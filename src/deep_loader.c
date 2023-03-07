/**
 * Filename:deep_loader.c
 * Author:megumin
 * Date:4/10/2021
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "deep_loader.h"
#include "deep_log.h"
#include "deep_mem.h"
#include "deep_opcode.h"

/**
 * @brief read a value of specified type
 *
 */
#define READ_VALUE(Type, p) \
    (p += sizeof(Type), *(Type*)(p - sizeof(Type)))
#define READ_UINT32(p)  READ_VALUE(uint32_t, p)
#define READ_BYTE(p) READ_VALUE(uint8_t, p)

#define WASM_MAGIC_SIZE 4
#define WASM_VERSION_SIZE 4

uint8_t wasm_type_size(uint8_t type) {
    switch (type)
    {
    case op_type_i32:
    case op_type_f32:
        return 4;
    case op_type_i64:
    case op_type_f64:
        return 8;
    case op_type_void:
        return 0;
    default:
        deep_error("Invalid type 0x%x!", type);
        exit(1);
    }
}

/**
 * @brief read unsigned int 32bits
 *
 * @param p
 * @return uint32_t
 */
uint32_t read_leb_u32(uint8_t** p) {
    uint8_t* buf = *p;
    uint32_t res = 0;
    for (int32_t i = 0; i < 5; i++) {
        res |= (buf[i] & 0x7f) << (i * 7);
        if ((buf[i] & 0x80) == 0) {
            *p += i + 1;
            return res;
        }
    }
    return 0;
}

/**
 * @brief read signed int 32bits
 *
 * @param p
 * @return uint32_t
 */
int32_t read_leb_i32(uint8_t** p) {
    uint8_t* buf = *p;
    int32_t res = 0;
    for (int32_t i = 0; i < 5; i++) {
        res |= (buf[i] & 0x7f) << (i * 7);
        if ((buf[i] & 0x80) == 0) {
            *p += i + 1;
            if((buf[i] & 0x40) != 0) {
                res = res | (-1 << ((i + 1) * 7));
            }
            return res;
        }
    }
    return 0;
}

/**
 * @brief read unsigned int 64bits
 *
 * @param p
 * @return uint32_t
 */
uint64_t read_leb_u64(uint8_t** p) {
    uint8_t* buf = *p;
    uint64_t res = 0;
    for (int64_t i = 0; i < 10; i++) {
        res |= (buf[i] & 0x7f) << (i * 7);
        if ((buf[i] & 0x80) == 0) {
            *p += i + 1;
            return res;
        }
    }
    return 0;
}

/**
 * @brief read signed int 64bits
 *
 * @param p
 * @return uint32_t
 */
int64_t read_leb_i64(uint8_t** p) {
    uint8_t* buf = *p;
    int64_t res = 0;
    for (int64_t i = 0; i < 10; i++) {
        res |= (buf[i] & 0x7f) << (i * 7);
        if ((buf[i] & 0x80) == 0) {
            *p += i + 1;
            if((buf[i] & 0x40) != 0) {
                res = res | (-1 << ((i + 1) * 7));
            }
            return res;
        }
    }
    return 0;
}

float read_ieee_32(uint8_t **p) {
    // TODO: Support big-endian machines.
    float res = *(float *)(*p);
    *p += 4;
    return res;
}

double read_ieee_64(uint8_t **p) {
    // TODO: Support big-endian machines.
    double res = *(double *)(*p);
    *p += 8;
    return res;
}

/**
 * @brief check magic number and version of wasm module
 *
 * @param p
 * @return true
 * @return false
 */
static bool check_magic_number_and_version(uint8_t** p) {
    uint32_t magic_number = 0, version = 0;
    magic_number = READ_UINT32(*p);
    version      = READ_UINT32(*p);
    if (magic_number == MAGIC_NUMBER && version == VERSION)
        return true;
    return false;
}

static char* str_gen(const char* p, int32_t len) {
    char* str = (char*)deep_malloc((len + 1) * sizeof(char));
    memcpy(str, p, len);
    str[len] = '\0';
    return str;
}

/**
 * @brief Create a section list object with linklist
 *
 * @param p
 * @param size
 * @return section_listnode*
 */
static section_listnode* create_section_list(const uint8_t** p, int32_t size) {
    const uint8_t *buf = *p, *buf_end = buf + size;
    section_listnode* section_list = NULL;
    section_listnode* current_section = NULL;
    while (buf < buf_end) {
        section_listnode* section = (section_listnode*)deep_malloc(sizeof(section_listnode));
        if (section_list == NULL) {
            section_list = section;
            current_section = section;
        }
        section->section_type  = READ_BYTE(buf);
        section->section_size  = read_leb_u32((uint8_t**)&buf);
        section->section_begin = (uint8_t*)buf;
        buf += section->section_size;
        current_section->next = section;
        current_section       = section;
    }
    if (buf == buf_end)
        return section_list;
    return NULL;
}

/**
 * @brief read type section
 *
 * @param p
 * @param module
 */
static void decode_type_section(const uint8_t* p, DEEPModule* module) {

    const uint8_t* p_tmp;
    uint32_t total_size = 0;
    uint32_t type_count = 0, param_count = 0, ret_count = 0;
    module->type_count = type_count = read_leb_u32((uint8_t**)&p);
    module->type_section = (DEEPType**)deep_malloc(type_count * sizeof(DEEPType*));
    for (int32_t i = 0; i < type_count; i++) {
        if (READ_BYTE(p) == 0x60) {
            param_count = read_leb_u32((uint8_t**)&p);
            p_tmp = p;
            p += param_count;
            ret_count = read_leb_u32((uint8_t**)&p);
            p = p_tmp;
            total_size = 8 + param_count + ret_count;

            module->type_section[i] = (DEEPType*)deep_malloc(sizeof(DEEPType));
            module->type_section[i]->type = (uint8_t*)deep_malloc(total_size);
            module->type_section[i]->param_count = param_count;
            module->type_section[i]->ret_count = ret_count;

            for (uint32_t j = 0; j < param_count; j++) {
                module->type_section[i]->type[j] = READ_BYTE(p);
            }
            read_leb_u32((uint8_t**)&p);
            for (uint32_t j = 0; j < ret_count; j++) {
                module->type_section[i]->type[param_count + j] = READ_BYTE(p);
            }
        }
    }
}

/**
 * @brief read funciton section
 *
 * @param p
 * @param module
 * @param p_code
 */
static void decode_func_section(const uint8_t* p, DEEPModule* module,const uint8_t* p_code) {
    uint32_t func_count = read_leb_u32((uint8_t**)&p);
    uint32_t code_func_count = read_leb_u32((uint8_t**)&p_code);
    uint32_t type_index, code_size, local_set_count, p_head;
    uint8_t local_var_count;
    DEEPFunction *func;
    const uint8_t *p_code_temp;
    if (func_count != code_func_count) {
        deep_error("function count is not equal to function body count");
        return;
    }
    uint32_t import_func_count = module->import_function_count;
    uint32_t all_func_count = import_func_count + func_count;
    module->function_count = all_func_count;
    module->func_section = (DEEPFunction**)deep_malloc(all_func_count * sizeof(DEEPFunction*));

    /* functions from import section */
    int32_t import_func_index = 0;
    DEEPImport *import = NULL;
    for(int32_t i = 0; i < module->import_count; i++) {
        if (import_func_index >= import_func_count) {
            deep_error("import function index >= function count in import section");
            return;
        }
        import = module->import_section[i];
        if(import->tag == FUNC_TAG_TYPE) {
            func = module->func_section[import_func_index] = (DEEPFunction*)deep_malloc(sizeof(DEEPFunction));
            memset(func, 0, sizeof(DEEPFunction));
            func->is_import = true;
            func->func_type = module->type_section[import->index];
            func->local_var_count = func->func_type->param_count;

            func->local_var_offsets = (uint32_t *)deep_malloc(sizeof(uint32_t) * (func->local_var_count + 1));
            u_int32_t offset = 0;
            for (uint32_t j = 0; j < func->local_var_count; j++) {
                func->local_var_offsets[j] = offset;
            }
            func->local_var_offsets[func->local_var_count] = offset;
            import_func_index++;
        }
    }

    /* functions belong to function section self */
    for (uint32_t i = import_func_count; i < all_func_count; i++) {
        func = module->func_section[i] = (DEEPFunction*)deep_malloc(sizeof(DEEPFunction));
        memset(func, 0, sizeof(DEEPFunction));
        type_index = read_leb_u32((uint8_t **)&p);
        code_size = read_leb_u32((uint8_t **)&p_code);
        p_code_temp = p_code;
        func->is_import = false;
        func->func_type = module->type_section[type_index];
        // The local variables also include the parameters
        local_var_count = func->func_type->param_count;
        local_set_count = local_var_count + read_leb_u32((uint8_t**)&p_code);
        if (local_set_count == 0) {
            func->local_var_types = NULL;
        } else {
            func->local_var_types = (LocalVarCluster *)deep_malloc(local_set_count * sizeof(LocalVarCluster));
            // For parameters, the count is 1, and the type
            // is the type of the corresponding parameter
            for (uint32_t j = 0; j < func->func_type->param_count; j++) {
                func->local_var_types[j].count = 1;
                func->local_var_types[j].local_type = func->func_type->type[j];
            }
            for (uint32_t j = func->func_type->param_count; j < local_set_count; j++) {
                func->local_var_types[j].count = read_leb_u32((uint8_t **)&p_code);
                local_var_count += func->local_var_types[j].count;
                func->local_var_types[j].local_type = READ_BYTE(p_code);
            }
        }

        func->code_begin = (uint8_t*)p_code;
        func->local_var_count = local_var_count;
        func->code_size = code_size - (uint32_t)(p_code - p_code_temp);
        p_code = p_code_temp + code_size;

        func->local_var_offsets = (uint32_t *)deep_malloc(sizeof(uint32_t) * (func->local_var_count + 1));
        u_int32_t offset = 0;
        uint32_t j = 0;
        for (uint32_t k = 0; k < local_set_count; k++) {
            for (uint32_t c = 0; c < func->local_var_types[k].count; c++) {
                func->local_var_offsets[j] = offset;
                offset += wasm_type_size(func->local_var_types[k].local_type);
                j += 1;
            }
        }
        assert(j == func->local_var_count);
        func->local_var_offsets[j] = offset;
        import_func_index++;
    }
}

/**
 * @brief read export section
 *
 * @param p
 * @param module
 */
static void decode_export_section(const uint8_t* p, DEEPModule* module) {
    uint32_t export_count = read_leb_u32((uint8_t**)&p);
    uint32_t name_len;
    DEEPExport* Export;
    module->export_count = export_count;
    module->export_section = (DEEPExport**)deep_malloc(export_count * sizeof(DEEPExport*));

    for (uint32_t i = 0; i < export_count; i ++) {
        Export = module->export_section[i] = (DEEPExport*)deep_malloc(sizeof(DEEPExport));
        name_len = read_leb_u32((uint8_t**)&p);
        Export->name = str_gen((char *)p, name_len);
        p += name_len;
        Export->tag = READ_BYTE(p);
        Export->index = read_leb_u32((uint8_t**)&p);
    }
}

/**
 * @brief load import section
 *
 * @param p
 * @param module
 */
static void decode_import_section(const uint8_t* p, DEEPModule* module)
{
    uint32_t import_count = read_leb_u32((uint8_t**)&p);
    uint32_t module_name_len = 0;
    uint32_t member_name_len = 0;
    DEEPImport* Import = NULL;
    module->import_count = import_count;
    module->import_section = (DEEPImport**)deep_malloc(import_count * sizeof(DEEPImport*));
    for (uint32_t i = 0; i < import_count; i ++) {
        Import = module->import_section[i] = (DEEPImport *)deep_malloc(sizeof(DEEPImport));
        /* read import module name */
        module_name_len = read_leb_u32((uint8_t**)&p);
        Import->module_name = str_gen((char *)p, module_name_len);
        p += module_name_len;
        /* read member name */
        member_name_len = read_leb_u32((uint8_t**)&p);
        Import->member_name = str_gen((char *)p, member_name_len);
        p += member_name_len;
        /* read tag(type of this record) and index */
        Import->tag = READ_BYTE(p);
        Import->index = read_leb_u32((uint8_t**)&p);
        switch (Import->tag)
        {
        case FUNC_TAG_TYPE:
            module->import_function_count += 1;
            break;
        case TAB_TAG_TYPE:
            module->import_table_count += 1;
            break;
        case MEM_TAG_TYPE:
            module->import_memory_count += 1;
            break;
        case GLOBAL_TAG_TYPE:
            module->import_global_count += 1;
            break;
        default:
            deep_error("mismatch tag type");
            break;
        }
    }
}

/**
 * @brief read data section
 *
 * @param p
 * @param module
 */
static void decode_data_section(const uint8_t* p, DEEPModule* module) {
    uint32_t data_count = read_leb_u32((uint8_t**)&p);
    DEEPData* Data;
    module->data_count = data_count;
    module->data_section = (DEEPData**)deep_malloc(data_count * sizeof(DEEPData*));

    for (uint32_t i = 0; i < data_count; i ++) {
        Data = module->data_section[i] = (DEEPData*)deep_malloc(sizeof(DEEPData));
        READ_BYTE(p);
        p ++;
        Data->offset = read_leb_u32((uint8_t**)&p);
        p ++;
        Data->datasize = read_leb_u32((uint8_t**)&p);
        Data->data = (uint8_t*)p;
    }
}

/**
 * @brief load sections one by one.
 *
 * @param module
 * @param section_list
 */
static void decode_each_sections(DEEPModule* module, section_listnode* section_list) {

    section_listnode* section = section_list;
    const uint8_t *buf = NULL, *p_code = NULL;
    while (section->section_type != SECTION_TYPE_CODE) {
        section = section->next;
    }
    p_code = section->section_begin;
    while (section_list) {
        buf = section_list->section_begin;
        switch (section_list->section_type) {
        case SECTION_TYPE_USER:

            break;
        case SECTION_TYPE_TYPE:
            decode_type_section(buf, module);
            break;
        case SECTION_TYPE_IMPORT:
            decode_import_section(buf, module);
            break;
        case SECTION_TYPE_FUNC:
            decode_func_section(buf, module, p_code);
            break;
        case SECTION_TYPE_TABLE:

            break;
        case SECTION_TYPE_MEMORY:

            break;
        case SECTION_TYPE_GLOBAL:

            break;
        case SECTION_TYPE_EXPORT:
            decode_export_section(buf, module);
            break;
        case SECTION_TYPE_START:

            break;
        case SECTION_TYPE_ELEM:

            break;
        case SECTION_TYPE_CODE:

            break;
        case SECTION_TYPE_DATA:
            decode_data_section(buf, module);

            break;
        default:

            break;
        }
        section_list = section_list->next;
    }
}

DEEPModule* deep_load(uint8_t** p, int size) {
    if (!check_magic_number_and_version(p)) {
        deep_error("magic number error");
        return NULL;
    }
    section_listnode* section_list = create_section_list((const uint8_t**)p, size - WASM_MAGIC_SIZE - WASM_VERSION_SIZE);
    if(section_list == NULL) {
        deep_error("create section list fail");
        return NULL;
    }
    size -= 8;
    DEEPModule* module = (DEEPModule*)deep_malloc(sizeof(DEEPModule));
    if(module == NULL) {
        deep_error("module malloc fail");
        return NULL;
    }
    memset(module, 0, sizeof(DEEPModule));
    decode_each_sections(module, section_list);
    section_listnode* dummy = section_list, *q;
    while(dummy != NULL) {
        q = dummy->next;
        deep_free(dummy);
        dummy = q;
    }
    section_list = NULL;
    return module;
}

void module_free(DEEPModule *module) {
    uint32_t i;

    for (i = 0; i < module->data_count; i++) {
        deep_free(module->data_section[i]);
    }
    deep_free(module->data_section);

    for (i = 0; i < module->type_count; i++) {
        deep_free(module->type_section[i]->type);
        deep_free(module->type_section[i]);
    }
    deep_free(module->type_section);

    for (i = 0; i < module->function_count; i++) {
        deep_free(module->func_section[i]->local_var_types);
        deep_free(module->func_section[i]->local_var_offsets);
        deep_free(module->func_section[i]);
    }
    deep_free(module->func_section);

    for (i = 0; i < module->export_count; i++) {
        deep_free(module->export_section[i]->name);
        deep_free(module->export_section[i]);
    }
    deep_free(module->export_section);

    for (i = 0; i < module->import_count; i++) {
        deep_free(module->import_section[i]->module_name);
        deep_free(module->import_section[i]->member_name);
        deep_free(module->import_section[i]);
    }
    deep_free(module->import_section);

    deep_free(module);
}

uint8_t* init_memory(uint32_t min_page) {
    uint8_t* mem = (uint8_t*)deep_malloc(min_page * PAGESIZE);
    return mem;
}

static void read_memory(uint8_t* mem, uint8_t* dst, uint32_t offset, uint32_t size) {
    memcpy(dst, mem + offset, size);
}

static void write_memory(uint8_t* mem, uint8_t* src, uint32_t offset, uint32_t size) {
    memcpy(mem + offset, src, size);
}

uint8_t read_mem8(uint8_t* mem, uint32_t offset) {
    uint8_t buf[1];
    read_memory(mem, buf, offset, 1);
    return buf[0];
}

uint16_t read_mem16(uint8_t* mem, uint32_t offset) {
    uint16_t buf[1];
    read_memory(mem, (uint8_t *)buf, offset, 2);
    return buf[0];
}

uint32_t read_mem32(uint8_t* mem, uint32_t offset) {
    uint32_t buf[1];
    read_memory(mem, (uint8_t *)buf, offset, 4);
    return buf[0];
}

uint64_t read_mem64(uint8_t* mem, uint32_t offset) {
    uint64_t buf[1];
    read_memory(mem, (uint8_t *)buf, offset, 8);
    return buf[0];
}

void write_mem8(uint8_t* mem, uint8_t val, uint32_t offset) {
    uint8_t buf[1];
    buf[0] = val;
    write_memory(mem, buf, offset, 1);
}

void write_mem16(uint8_t* mem, uint16_t val, uint32_t offset) {
    uint16_t buf[1];
    buf[0] = val;
    write_memory(mem, (uint8_t *)buf, offset, 2);
}

void write_mem32(uint8_t* mem, uint32_t val, uint32_t offset) {
    uint32_t buf[1];
    buf[0] = val;
    write_memory(mem, (uint8_t *)buf, offset, 4);
}

void write_mem64(uint8_t* mem, uint64_t val, uint32_t offset) {
    uint64_t buf[1];
    buf[0] = val;
    write_memory(mem, (uint8_t *)buf, offset, 8);
}

void type_section_dump(DEEPModule* module) {
    uint32_t i, j = 0;
    printf("%s\n", "========================================================");
    printf("%s\t%d\n", "TYPE_SECTION", module->type_count);
    for (i = 0; i < module->type_count; i ++) {
        printf("%s%d%c\t", "   - type[", i, ']');
        printf("%s", "( ");
        for (j = 0; j < module->type_section[i]->param_count; j ++) {
            printf("%s", "int ");
        }
        printf("%s", ") -> ( ");
        for (j = 0; j < module->type_section[i]->ret_count; j ++) {
            printf("%s", "int ");
        }
        printf("%c", ')');
        printf("\n");
    }
    printf("%s\n", "========================================================");
}
