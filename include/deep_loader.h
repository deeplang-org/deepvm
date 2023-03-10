/**
 * Filename:deep_loader.h
 * Author:megumin
 * Date:4/10/2021
 */

#ifndef _DEEP_LOADER_H
#define _DEEP_LOADER_H

/**
 * @brief head of wasm module
 *
 */
#define MAGIC_NUMBER 0x6d736100
#define VERSION 1

/**
 * @brief section NO
 *
 */
#define SECTION_TYPE_USER 0
#define SECTION_TYPE_TYPE 1
#define SECTION_TYPE_IMPORT 2
#define SECTION_TYPE_FUNC 3
#define SECTION_TYPE_TABLE 4
#define SECTION_TYPE_MEMORY 5
#define SECTION_TYPE_GLOBAL 6
#define SECTION_TYPE_EXPORT 7
#define SECTION_TYPE_START 8
#define SECTION_TYPE_ELEM 9
#define SECTION_TYPE_CODE 10
#define SECTION_TYPE_DATA 11

/**
 * @brief memory manage parameters
 *
 */
#define PAGESIZE 65536

enum ImportAndExportTagType
{
    FUNC_TAG_TYPE = 0,
    TAB_TAG_TYPE,
    MEM_TAG_TYPE,
    GLOBAL_TAG_TYPE
};


/**
 * @brief items of function type section
 *
 */
typedef struct DEEPType
{
    uint32_t param_count;
    uint32_t ret_count;
    uint8_t *type;
} DEEPType;

/**
 * @brief local variable information in DEEPFunction
 *
 */
typedef struct LocalVarCluster
{
    uint32_t count;
    uint8_t local_type;
} LocalVarCluster;

/**
 * @brief items of functoin section
 *
 */
typedef struct DEEPFunction
{
    DEEPType *func_type; // the type of function
    LocalVarCluster *local_var_types; // Local variables' type informations
    uint32_t code_size;
    uint8_t *code_begin;
    uint8_t local_var_count; // Including function parameters
    uint32_t *local_var_offsets;
    bool is_import;
} DEEPFunction;

/**
 * @brief items of export section
 *
 */
typedef struct DEEPExport
{
    char* name;
    uint32_t index;
    char tag;
} DEEPExport;

/**
 * @brief items of import section
 *
 */
typedef struct DEEPImport
{
    char* module_name;
    char* member_name;
    uint32_t index;
    char tag;
} DEEPImport;

/**
 * @brief items of data section
 *
 */
typedef struct DEEPData
{
    uint32_t offset;
    uint32_t datasize;
    uint8_t *data;
} DEEPData;

/**
 * @brief Data structure of module. We only support 5 sections which can make the program run.
 *
 */
typedef struct DEEPModule
{
    uint32_t type_count;
    uint32_t function_count;
    uint32_t export_count;
    uint32_t import_count;
    /* share index with local sections */
    uint32_t import_function_count; /* share function indexs with function section */
    uint32_t import_table_count;
    uint32_t import_memory_count;
    uint32_t import_global_count;
    uint32_t data_count;
    DEEPType **type_section;
    DEEPFunction **func_section;
    DEEPExport **export_section;
    DEEPImport **import_section;
    DEEPData **data_section;
} DEEPModule;

/**
 * @brief the definition of listnode
 *
 */
typedef struct section_listnode
{
    uint8_t section_type;
    int32_t section_size;
    uint8_t *section_begin;
    struct section_listnode *next;
} section_listnode;

DEEPModule* deep_load(uint8_t** p, int32_t size);
void module_free(DEEPModule *module);
uint32_t read_leb_u32(uint8_t** p);
int32_t read_leb_i32(uint8_t** p);
uint64_t read_leb_u64(uint8_t** p);
int64_t read_leb_i64(uint8_t** p);

/**
 * @brief Read a little-endian 32-bit floating point number.
 *
 * @param p
 * @return float
 */
float read_ieee_32(uint8_t **p);
double read_ieee_64(uint8_t **p);

uint8_t wasm_type_size(uint8_t type);
uint8_t* init_memory(uint32_t min_page);
uint8_t read_mem8(uint8_t* mem, uint32_t offset);
uint16_t read_mem16(uint8_t* mem, uint32_t offset);
uint32_t read_mem32(uint8_t* mem, uint32_t offset);
uint64_t read_mem64(uint8_t* mem, uint32_t offset);
void write_mem8(uint8_t* mem, uint8_t val, uint32_t offset);
void write_mem16(uint8_t* mem, uint16_t val, uint32_t offset);
void write_mem32(uint8_t* mem, uint32_t val, uint32_t offset);
void write_mem64(uint8_t* mem, uint64_t val, uint32_t offset);
#endif /* _DEEP_LOADER_H */
