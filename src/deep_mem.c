
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "random.h"
#include "deep_log.h"
#include "deep_mem.h"

// #define REVERT_TO_DEFAULT_MEMORY_MANAGEMENT

#ifdef REVERT_TO_DEFAULT_MEMORY_MANAGEMENT
#include "stdlib.h"
#endif

mem_pool_t *pool;

/*
  Store the offset between payload and head of a block.
  Calculate at runtime (during deep_init()).
  This is just a patch; will prolly need better solutions.
*/
static uint32_t block_payload_offset;

static void *deep_malloc_fast_bins (uint32_t size);
static void *deep_malloc_sorted_bins (uint32_t size);
static void deep_free_fast_bins (void *ptr);
static void deep_free_sorted_bins (void *ptr);

/* helper functions for maintaining the sorted_block skiplist */
static sorted_block_t *
_split_into_two_sorted_blocks (sorted_block_t *block,
                               uint32_t aligned_size);
static void _merge_into_single_block (sorted_block_t *curr,
                                      sorted_block_t *next);
static sorted_block_t *
_allocate_block_from_skiplist (uint32_t aligned_size);
static inline bool _sorted_block_is_in_skiplist (sorted_block_t *block);
static sorted_block_t *
_find_sorted_block_by_size_on_index (sorted_block_t *node, uint32_t size,
                                     uint32_t index_level);
static sorted_block_t *
_find_sorted_block_by_size (sorted_block_t *node, uint32_t size);
static void _insert_sorted_block_to_skiplist (sorted_block_t *block);
static void _remove_sorted_block_from_skiplist (sorted_block_t *block);

static inline bool
block_is_allocated (block_head_t const *head)
{
  return (*head) & A_FLAG_MASK;
}

static inline void
block_set_A_flag (block_head_t *head, bool allocated)
{
  *head = allocated ? (*head | A_FLAG_MASK) : (*head & (~A_FLAG_MASK));
}

static inline bool
prev_block_is_allocated (block_head_t const *head)
{
  return (*head) & P_FLAG_MASK;
}

static inline void
block_set_P_flag (block_head_t *head, bool allocated)
{
  *head = allocated ? (*head | P_FLAG_MASK) : (*head & (~P_FLAG_MASK));
}

/**
 * The size of the payload.
 **/
static inline block_size_t
block_get_size (block_head_t const *head)
{
  return (*head) & BLOCK_SIZE_MASK;
}

/**
 * The 'size' passed here should be the size of PAYLOAD, NOT the size of the 
 * entire block!
 **/
static inline void
block_set_size (block_head_t *head, block_size_t size)
{
  *head = (*head & (~BLOCK_SIZE_MASK)) | size;
}

static inline void *
get_pointer_by_offset_in_bytes (void *p, int64_t offset)
{
  return (uint8_t *)p + offset;
}

static inline int64_t
get_offset_between_pointers_in_bytes (void *p, void *q)
{
  return (uint8_t *)p - (uint8_t *)q;
}

static inline struct sorted_block *
get_block_by_offset (struct sorted_block *node, int32_t offset)
{
  return (struct sorted_block *)(get_pointer_by_offset_in_bytes ((mem_t *)node,
                                                                 offset));
}

static inline int32_t
get_offset_between_blocks (struct sorted_block *origin,
                           struct sorted_block *target)
{
  return get_offset_between_pointers_in_bytes ((mem_t *)target,
                                               (mem_t *)origin);
}

static inline mem_size_t
get_remainder_size (struct mem_pool const *pool)
{
  return get_offset_between_pointers_in_bytes (pool->remainder_block_end,
                                               pool->remainder_block_head);
}

bool deep_mem_init (void *mem, uint32_t size)
{
  // Offset calculation
  {
    fast_block_t block;
    block_payload_offset
      =(uint32_t)((uint8_t *)&block.payload - (uint8_t *)&block.head);
  }
  PRINT_ARG("Offset: %u\n", block_payload_offset);

  if (size < sizeof (mem_pool_t))
    {
      return false; /* given buffer is too small */
    }

  memset(mem, 0, size);
  mem_size_t aligned_size = ALIGN_MEM_SIZE_TRUNC(size);

  pool = (mem_pool_t *)mem;
  pool->free_memory = aligned_size - sizeof(mem_pool_t)
                      - sizeof(sorted_block_t) - sizeof(block_head_t);
  /* the first node in the list, to simplify implementation */
  pool->sorted_block.addr = 
      (sorted_block_t *)(get_pointer_by_offset_in_bytes(
        mem, sizeof(mem_pool_t)));
  /* all other fields are set as 0 */
  pool->sorted_block.addr->payload.info.level_of_indices = 
      SORTED_BLOCK_INDICES_LEVEL;
  pool->remainder_block_head =
      (block_head_t *)(get_pointer_by_offset_in_bytes(
        mem, sizeof(mem_pool_t) + sizeof(sorted_block_t)));
  pool->remainder_block_end = 
      (get_pointer_by_offset_in_bytes(mem, aligned_size - 8)); // -8 for safety
  for (int i = 0; i < FAST_BIN_LENGTH; ++i)
    {
      pool->fast_bins[i].addr = NULL;
    }
  // initialise remainder block's head
  block_set_A_flag (pool->remainder_block_head, false);
  block_set_P_flag (pool->remainder_block_head, true);
  PRINT_ARG("Free memory (start):     %llu\n", pool->free_memory);
  return true;
}

void
deep_mem_destroy (void)
{
  pool = NULL;
}

#ifdef REVERT_TO_DEFAULT_MEMORY_MANAGEMENT
void *
deep_malloc(uint32_t size)
{
  void *result = malloc(size);
  memset(result, 0, size);
  return result;
}
#else
void *
deep_malloc(uint32_t size)
{
  if (pool->free_memory < size || size == 0)
  {
    return NULL;
  }

  uint32_t aligned_size = ALIGN_MEM_SIZE(size + block_payload_offset);

  if (aligned_size <= FAST_BIN_MAX_SIZE)
  {
    return deep_malloc_fast_bins(aligned_size);
  }
  return deep_malloc_sorted_bins(aligned_size);
}
#endif

/* Note that aligning is done in deep_malloc, the size should already be 
 * aligned here.
 */
static void *
deep_malloc_fast_bins(block_size_t aligned_size)
{
  uint32_t offset = (aligned_size >> 3) - 1;
  bool P_flag = false;
  fast_block_t *ret = NULL;
  block_size_t payload_size;
  if (pool->fast_bins[offset].addr != NULL) 
  {
    ret = pool->fast_bins[offset].addr;
    pool->fast_bins[offset].addr = ret->payload.next;
    P_flag = prev_block_is_allocated(&ret->head);
    payload_size = block_get_size(&ret->head);

  }
  // When there are no available fast blocks, grab at the end of the remainder.
  else if (aligned_size <= get_remainder_size(pool)) 
  {
    ret = (fast_block_t *)(get_pointer_by_offset_in_bytes
        (pool->remainder_block_end,
        -(int64_t)aligned_size - sizeof(block_head_t)));
    pool->remainder_block_end = (void *)ret;

    payload_size = aligned_size - block_payload_offset;
    block_set_size (&ret->head, payload_size);
  }
  else 
  {
    return NULL;
  }

  memset (&ret->payload, 0, payload_size);
  block_set_A_flag (&ret->head, true);
  block_set_P_flag (&ret->head, P_flag);
  pool->free_memory -= payload_size;

  PRINT_ARG("Remainder start (after fast allocation): %p\n", pool->remainder_block_head);
  PRINT_ARG("Remainder end (after fast allocation):   %p\n", pool->remainder_block_end);
  PRINT_ARG("Payload size (after fast allocation):    %u\n", payload_size);
  PRINT_ARG("Free memory (after fast allocation):     %llu\n", pool->free_memory);
  return &ret->payload;
}

/* Note that aligning is done in deep_malloc, the size should already be 
 * aligned here.
 */
static void *
deep_malloc_sorted_bins (block_size_t aligned_size)
{
  sorted_block_t *ret = NULL;
  block_size_t payload_size = aligned_size - block_payload_offset;
  if ((pool->sorted_block.addr != NULL)
      && ((ret = _allocate_block_from_skiplist(aligned_size)) != NULL))
  {
    // PRINT_ARG("%s", "Allocate from skiplist\n");
    /* pass */
  }
  else if (aligned_size <= get_remainder_size (pool))
  {
    // PRINT_ARG("%s", "Allocate not from skiplist (start)\n");
    /* no suitable sorted_block */
    ret = (sorted_block_t *)pool->remainder_block_head;
    block_set_size(
        &ret->head, get_remainder_size(pool) - block_payload_offset);
    pool->remainder_block_head
        = (block_head_t *)_split_into_two_sorted_blocks(ret, aligned_size);
    // PRINT_ARG("%s", "Allocate not from skiplist (finish)\n");
  }
  else
  {
    return NULL;
  }

  memset (&ret->payload, 0, payload_size);
  block_set_A_flag (&ret->head, true);
  block_set_P_flag (&get_block_by_offset (ret, aligned_size)->head, true);
  pool->free_memory -= payload_size;

  PRINT_ARG("Remainder start (after allocation): %p\n", pool->remainder_block_head);
  PRINT_ARG("Remainder end (after allocation):   %p\n", pool->remainder_block_end);
  PRINT_ARG("Payload size (after allocation):    %u\n", payload_size);
  PRINT_ARG("Free memory (after allocation):     %llu\n", pool->free_memory);

  return &ret->payload;
}

void *
deep_realloc (void *ptr, uint32_t size)
{
  return NULL;
}

#ifdef REVERT_TO_DEFAULT_MEMORY_MANAGEMENT
void
deep_free (void *ptr)
{
  free(ptr);
}
#else
void
deep_free (void *ptr)
{
  if (ptr == NULL)
  {
    return;
  }

  void *head = 
      get_pointer_by_offset_in_bytes(ptr, -(int64_t)block_payload_offset);
  if (!block_is_allocated((block_head_t *)head))
  {
    // PRINT_ARG("Double Free: %u\n", ptr == NULL);
    return;
  }
  block_size_t block_size = 
      block_get_size((block_head_t *)head) + block_payload_offset;
  if (block_size <= FAST_BIN_MAX_SIZE)
  {
    deep_free_fast_bins(head);
  }
  else
  {
    deep_free_sorted_bins(head);
  }
}
#endif

static void
deep_free_fast_bins(void *ptr)
{
  fast_block_t *block = ptr;
  // block size is payload size according to spec.
  block_size_t payload_size = block_get_size(&block->head);
  uint32_t offset = ((payload_size + block_payload_offset) >> 3) - 1;
  memset (&block->payload, 0, payload_size);
  block_set_A_flag (&block->head, false);
  pool->free_memory += payload_size;
  if (pool->fast_bins[offset].addr != NULL) 
  {
    block->payload.next = pool->fast_bins[offset].addr->payload.next;
  }
  pool->fast_bins[offset].addr = block;

  PRINT_ARG("Remainder start (after free fast): %p\n", pool->remainder_block_head);
  PRINT_ARG("Remainder end (after free fast):   %p\n", pool->remainder_block_end);
  PRINT_ARG("Payload size (after free fast):    %u\n", payload_size);
  PRINT_ARG("Free memory (after free fast):     %llu\n", pool->free_memory);
}

static void
deep_free_sorted_bins (void *ptr)
{
  sorted_block_t *block = ptr;
  sorted_block_t *the_other = NULL;
  // block size is payload size according to spec.
  block_size_t payload_size = block_get_size (&block->head);

  memset (&block->payload, 0, payload_size);

  block_set_A_flag (&block->head, false);

  /* try to merge */
  /* merge above */
  if (!prev_block_is_allocated (&block->head))
    {
      // PRINT_ARG("%s", "Merge above\n");
      block_size_t prev_size
          = block_get_size ((block_head_t *)(block - sizeof (block_head_t)));
      
      the_other = get_block_by_offset (block, -((int32_t)prev_size));
      
      _merge_into_single_block (the_other, block);
      block = the_other;
    }

  /* merge below */
  the_other = get_block_by_offset(
      block, block_get_size(&block->head) + block_payload_offset);
  if (!block_is_allocated (&the_other->head))
    {
      //  PRINT_ARG("%s", "Merge below\n");
      _merge_into_single_block (block, the_other);
    }
  /* update remainder_block if it is involved */
  if (the_other == (sorted_block_t *)pool->remainder_block_head)
    {
      // PRINT_ARG("%s", "Merge into remainder\n");
      pool->remainder_block_head = (block_head_t *)block;
    }

  if (!_sorted_block_is_in_skiplist (block))
    {
      _insert_sorted_block_to_skiplist (block);
    }

  pool->free_memory += payload_size;

  PRINT_ARG("Remainder start (after free): %p\n", pool->remainder_block_head);
  PRINT_ARG("Remainder end (after free):   %p\n", pool->remainder_block_end);
  PRINT_ARG("Payload size (after free):    %u\n", payload_size);
  PRINT_ARG("Free memory (after free):     %llu\n", pool->free_memory);
}

bool
deep_mem_migrate (void *new_mem, uint32_t size)
{
  return false;
}

/* helper functions for maintaining the sorted_block skiplist.
 * aligned_size is the total size of the first block (head + payload).
*/
static sorted_block_t *
_split_into_two_sorted_blocks (sorted_block_t *block,
                               uint32_t aligned_size)
{
  sorted_block_t *new_block = get_block_by_offset(block, aligned_size);
  // new block size = old block size - space used (aligned_size) - head size.
  block_size_t new_block_size
      = block_get_size(&block->head) - aligned_size - block_payload_offset;

  // Do we really need this memset?
  // Maybe setting only the head of new block is suffice?
  memset (new_block, 0, new_block_size + block_payload_offset);
  block_set_size (&new_block->head, new_block_size);
  block_set_A_flag (&new_block->head, false);
  block_set_P_flag (&new_block->head, false); /* by default */
  block_set_size (&block->head, aligned_size - block_payload_offset);
  // pool->free_memory -= sizeof (block_head_t);

  return new_block;
}

/**
 * Assuming `curr` and `next` are contiguous in memory address,
 * where curr < next
 * Attempt to remove both nodes from list, merge them, and insert the merged.
 * NOTE:
 *   - will update `free_memory` of releasing `sizeof (block_head_t)` to pool
 **/
static void
_merge_into_single_block (sorted_block_t *curr, sorted_block_t *next)
{
  _remove_sorted_block_from_skiplist (curr);
  _remove_sorted_block_from_skiplist (next);

  block_size_t new_size
      = block_get_size (&curr->head) + block_get_size (&next->head);

  block_set_size (&curr->head, new_size);
  memset (&curr->payload, 0, new_size - sizeof (curr->head));
  // copy over new head info to footer
  *(block_head_t *)get_pointer_by_offset_in_bytes (
      &curr->head, new_size - sizeof (block_head_t))
      = curr->head;

  _insert_sorted_block_to_skiplist (curr);
}

/** Obtain a most apporiate block from sorted_list if possible.
 *
 * - Obtain one with exact same size.
 * - Obtain one with bigger size, but split into two sorted blocks
 *   - returns the part with exactly same size
 *   - insert the rest into sorted_block skiplist
 *   - NOTE: this requires the block found be at least
 *           (`aligned_size + SORTED_BIN_MIN_SIZE`) big
 * - NULL
 *
 * NOTE: The obtained block will be **removed** from the skiplist.
 **/
static sorted_block_t *
_allocate_block_from_skiplist (uint32_t aligned_size)
{
  sorted_block_t *ret = NULL;

  if ((pool->sorted_block.addr == NULL)
      || ((ret = _find_sorted_block_by_size (pool->sorted_block.addr,
                                             aligned_size))
          == NULL))
    {
      return NULL;
    }
  if (block_get_size (&ret->head) != aligned_size)
    {
      if ((block_get_size (&ret->head) < aligned_size + SORTED_BIN_MIN_SIZE)
          && (ret = _find_sorted_block_by_size (
                  ret, aligned_size + SORTED_BIN_MIN_SIZE))
                 == NULL)
        {
          return NULL;
        }
      sorted_block_t *remainder = _split_into_two_sorted_blocks (ret, aligned_size);
      _insert_sorted_block_to_skiplist (remainder);
    }
  _remove_sorted_block_from_skiplist (ret);

  return ret;
}

static inline bool
_sorted_block_is_in_skiplist (sorted_block_t *block)
{
  return (block->payload.info.pred_offset != 0 || block->payload.info.level_of_indices != 0);
}

/**
 *  returns a block with desired size on the list of given index level;
 * if not possible, the greatest one that is smaller than desired.
 *
 * NOTE:
 *   - there will always be an infimum due to the existence of head
 *   - this function will not check nodes on other index level
 *   - this function will not check if there are any predecessor in the chain
 *     with same key. It assumes the `node` given has embedded all indices.
 **/
static sorted_block_t *
_find_sorted_block_by_size_on_index (sorted_block_t *node, uint32_t size,
                                     uint32_t index_level)
{

  sorted_block_t *curr = node;
  sorted_block_t *prev = curr;

  while (block_get_size (&curr->head) < size)
    {
      prev = curr; /* curr is the candidate of infimum. */
      /* reached the end of the skiplist or the biggest smaller sorted block */
      if (index_level >= SORTED_BLOCK_INDICES_LEVEL
          || curr->payload.info.offsets[index_level] == 0)
        {
          break;
        }
      curr = get_block_by_offset (curr, curr->payload.info.offsets[index_level]);
    }

  /* return a node with no indices to avoid copying indices. */
  if (block_get_size (&curr->head) == size && curr->payload.info.succ_offset != 0)
    {
      return get_block_by_offset (curr, curr->payload.info.succ_offset);
    }

  return prev;
}

/**
 *  returns a block with desired size; if not possible, the least greater one
 *
 * NOTE:
 *   - returns NULL when supremum is not in the list
 **/
static sorted_block_t *
_find_sorted_block_by_size (sorted_block_t *node, uint32_t size)
{

  sorted_block_t *curr = node;

  /* indices should only exists on first node in each sub-list. */
  while (curr->payload.info.pred_offset != 0)
    {
      curr = get_block_by_offset (curr, curr->payload.info.pred_offset);
    }

  while (block_get_size (&curr->head) < size)
    {
      uint32_t index_level
          = SORTED_BLOCK_INDICES_LEVEL - curr->payload.info.level_of_indices;

      /* skip non-existing indices, to node with size <= than desired */
      while (index_level < SORTED_BLOCK_INDICES_LEVEL
             || curr->payload.info.offsets[index_level] == 0
             || (block_get_size (&get_block_by_offset (curr, curr->payload.info.offsets[index_level])->head) > size))
        {
          index_level++;
        }

      /* reached the end of the skiplist or the biggest smaller sorted block */
      if (index_level >= SORTED_BLOCK_INDICES_LEVEL
          || curr->payload.info.offsets[index_level] == 0)
        {
          break;
        }

      /* will not be NULL as curr's size is smaller than size */
      curr = _find_sorted_block_by_size_on_index (curr, size, index_level);
    }

  /* all nodes are smaller than required. */
  if (block_get_size (&curr->head) < size)
    {
      return NULL;
    }

  /* return a node with no indices to avoid copying indices. */
  if (curr->payload.info.succ_offset != 0)
    {
      curr = get_block_by_offset (curr, curr->payload.info.succ_offset);
    }

  return curr;
}

static void
_insert_sorted_block_to_skiplist (sorted_block_t *block)
{
  block_size_t size = block_get_size (&block->head);
  sorted_block_t *pos
      = _find_sorted_block_by_size (pool->sorted_block.addr, size);

  /* insert into the chain with same size. */
  if (pos != NULL && block_get_size (&pos->head) == size)
    {
      block->payload.info.pred_offset = get_offset_between_blocks (block, pos);
      if (pos->payload.info.succ_offset != 0)
        {
          block->payload.info.succ_offset
              = pos->payload.info.succ_offset - get_offset_between_blocks (pos, block);
        }
      else
        {
          block->payload.info.succ_offset = 0; /* end of chain */
        }
      pos->payload.info.succ_offset = get_offset_between_blocks (pos, block);

      return;
    }

  block->payload.info.level_of_indices
      = ((uint32_t) (next () >> 32)) % SORTED_BLOCK_INDICES_LEVEL + 1;

  for (uint32_t index_level
       = SORTED_BLOCK_INDICES_LEVEL - block->payload.info.level_of_indices;
       index_level < SORTED_BLOCK_INDICES_LEVEL; ++index_level)
    {
      pos = _find_sorted_block_by_size_on_index (pool->sorted_block.addr, size,
                                                 index_level);
      if (pos->payload.info.offsets[index_level] != 0)
        {
          block->payload.info.offsets[index_level]
              = pos->payload.info.offsets[index_level]
                - get_offset_between_blocks (pos, block);
        }
      else
        {
          block->payload.info.offsets[index_level] = 0;
        }
      pos->payload.info.offsets[index_level] = get_offset_between_blocks (pos, block);
    }
}

/**
 * Remove the node and update all indices / offsets.
 * May traverse the list multiple times
 *
 * NOTE: never removes a node with offsets when it has children in the chain
 **/
static void _remove_sorted_block_from_skiplist (sorted_block_t *block)
{

  /* considering a node which is not in the skiplist */
  sorted_block_t *prev = NULL;
  block_size_t size = block_get_size (&block->head);

  for (uint32_t index_level
       = SORTED_BLOCK_INDICES_LEVEL - block->payload.info.level_of_indices;
       index_level < SORTED_BLOCK_INDICES_LEVEL; ++index_level)
    {
      /* -1 to find the strictly smaller node. */
      prev = _find_sorted_block_by_size_on_index (pool->sorted_block.addr,
                                                  size - 1, index_level);
      if (block->payload.info.offsets[index_level] != 0)
        {
          prev->payload.info.offsets[index_level] += block->payload.info.offsets[index_level];
        }
      else
        {
          prev->payload.info.offsets[index_level] = 0;
        }
    }

  if (block->payload.info.pred_offset != 0)
    {
      if (block->payload.info.succ_offset != 0)
        {
          get_block_by_offset (block, block->payload.info.pred_offset)->payload.info.succ_offset
              += block->payload.info.succ_offset;
          get_block_by_offset (block, block->payload.info.succ_offset)->payload.info.pred_offset
              += block->payload.info.pred_offset;
        }
      else
        {
          get_block_by_offset (block, block->payload.info.pred_offset)->payload.info.succ_offset = 0;
        }
    }
  /* no other cases, as if it is the first node, it should be the only node. */
}

uint64_t deep_get_free_memory(void) {
  return pool->free_memory;
}
