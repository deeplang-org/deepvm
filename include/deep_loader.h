/**
 * Filename:deep_loader.h
 * Author:megumin
 * Date:4/10/2021
 */

#ifndef _DEEP_LOADER_H
#define _DEEP_LOADER_H

//
#define MAGIC_NUMBER 0x6d736100
#define VERSION 1

//
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

//
//type item
typedef struct DEEPType
{
    int32_t param_count;
    int32_t ret_count;
    uint8_t type[1];
} DEEPType;

//local variables item
typedef struct LocalVars
{
    int32_t count;
    int16_t local_type;
} LocalVars;

//function item
typedef struct DEEPFunction
{
    DEEPType *func_type; // the type of function
    LocalVars **localvars;
    int32_t code_size;
    uint8_t *code_begin;
} DEEPFunction;

//
typedef struct DEEPExport
{
    char* name;
    int32_t index;
    char tag;
} DEEPExport;

//
typedef struct DEEPData
{
    int32_t offset;
    int32_t datasize;
    uint8_t *data;
} DEEPData;

/* Data structure of module, at present we only support 
two sections, which can make the program run*/
typedef struct DEEPModule
{
    int32_t type_count;
    int32_t function_count;
    int32_t export_count;
    int32_t data_count;
    DEEPType **type_section;
    DEEPFunction **func_section;
    DEEPExport **export_section;
    DEEPData **data_section;
} DEEPModule;

//the difinition of listnode
typedef struct section_listnode
{
    uint8_t section_type;
    int32_t section_size;
    uint8_t *section_begin;
    struct section_listnode *next;
} section_listnode;

DEEPModule* deep_load(uint8_t** p, int32_t size);
int32_t read_leb_u32(uint8_t** p);
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