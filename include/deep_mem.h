#ifndef _DEEP_MEM_ALLOC_H
#define _DEEP_MEM_ALLOC_H

#include <stdbool.h>

#define FAST_BIN_LENGTH (8) /* eight size options for fast bins */
#define FAST_BIN_MAX_SIZE (64) /* 8 * 8 bytes */
#define SORTED_BIN_MIN_SIZE (FAST_BIN_MAX_SIZE + 8) /* 64 + 8 bytes */

#define A_FLAG_OFFSET (0) /* is allocated */
#define A_FLAG_MASK (1 << A_FLAG_OFFSET)
#define P_FLAG_OFFSET (1) /* is previous block allocated */
#define P_FLAG_MASK (1 << P_FLAG_OFFSET)
#define BLOCK_SIZE_MASK (0xFFFFFFFF - A_FLAG_MASK - P_FLAG_MASK)
#define REMAINDER_SIZE_MASK ((0xffffffff << 32) & BLOCK_SIZE_MASK)

#define SORTED_BLOCK_INDICES_LEVEL (13)

/* Align the size up to a multiple of eight*/
#define ALIGN_MEM_SIZE(size) (((size + 0x7) >> 3) << 3)
/* Align the size down to a multiple of eight */
#define ALIGN_MEM_SIZE_TRUNC(size) ((size >> 3) << 3)

typedef void *mem_t;
typedef uint64_t mem_size_t;
typedef uint32_t block_head_t;
typedef uint32_t block_size_t;

/* For storing small blocks of memory */
typedef struct fast_block
{
  block_head_t head;
  union
  {
    struct fast_block *next;
    void *payload;
  } payload;
} fast_block_t;

typedef struct sorted_block
{
  block_head_t head;
  union
  {
    struct
    {
      int32_t pred_offset;
      int32_t succ_offset;
      uint32_t level_of_indices;
      // bigger array index corresponds to lower index in skip list,
      // i.e., skipping less nodes in the skip list
      // 0 means this node is the last one in this level of index.
      // offsets[SORTED_BLOCK_INDICES_LEVEL - 1] is the level where each node
      // is connected one by one consecutively.
      // The skip list is in ascending order by block's size.
      int32_t offsets[SORTED_BLOCK_INDICES_LEVEL];
      // padding
      // uint32_t footer;
    } info;
    void *payload;
  } payload;
} sorted_block_t;

typedef struct mem_pool
{
  uint64_t free_memory;
  union
  {
    uint64_t _padding;
    sorted_block_t *addr;
  } sorted_block;
  union
  {
    uint64_t _remainder_block_head_padding;
    block_head_t *remainder_block_head; /* The head of remainder */
  };
  union
  {
    uint64_t _remainder_block_end_padding;
    void *remainder_block_end; /* The address of the last byte in remainder */
  }; /* should not be dereferenced */
  union
  {
    uint64_t _padding;
    fast_block_t *addr;
  } fast_bins[FAST_BIN_LENGTH];
} mem_pool_t;

bool deep_mem_init (void *mem, uint32_t size);
void deep_mem_destroy (void);
void *deep_malloc (uint32_t size);
void *deep_realloc (void *ptr, uint32_t size);
void deep_free (void *ptr);
bool deep_mem_migrate (void *new_mem, uint32_t size);

#endif /* _DEEP_MEM_ALLOC_H */
