/**
 * Filename:deep_loader.c
 * Author:megumin
 * Date:4/10/2021
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include "deep_loader.h"
#include "deep_log.h"

//read a value of specified type
#define READ_VALUE(Type, p) \
	(p += sizeof(Type), *(Type*)(p - sizeof(Type)))
#define READ_UINT32(p)  READ_VALUE(uint32_t, p)
#define READ_BYTE(p) READ_VALUE(uint8_t, p)

//解码一个无符号32位或64整型，目前只会用到32位类型,在大多数实现中占5个字节
int32_t read_leb_u32(uint8_t** p) {
	uint8_t* buf = *p;
	int32_t   res = 0;
	for (int32_t i = 0; i < 7; i++) {
		res |= (buf[i] & 0x7f) << (i * 7); //出错了，忘记加‘|’
		if ((buf[i] & 0x80) == 0) {
			*p += i + 1;
			return res;
		}
	}
	return 0;
}

//检查魔数和版本号
static bool check_magic_number_and_version(uint8_t** p) {
	uint32_t magic_number = 0, version = 0;
	magic_number = READ_UINT32(*p);
	version      = READ_UINT32(*p);
	if (magic_number == MAGIC_NUMBER && version == VERSION)
		return true;
	return false;
}

static char* str_gen(const char* p, int32_t len) {
	char* str = (char*)malloc((len + 1) * sizeof(char));
	memcpy(str, p, len);
	str[len] = '\0';
	return str;
}

//解析出每个section的开始和大小和类型，拉成一个链表， 从第二项开始存
static section_listnode* create_section_list(const uint8_t** p, int32_t size) {
	const uint8_t *buf = *p, *buf_end = buf + size;
	section_listnode* section_list = NULL;
	section_listnode* current_section = NULL;
	while (buf < buf_end) {
		section_listnode* section = (section_listnode*)malloc(sizeof(section_listnode));
		memset(section, 0, sizeof(section_listnode));
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

//读取类型段
static void decode_type_section(const uint8_t* p, DEEPModule* module) {

	const uint8_t* p_tmp;
	int32_t         total_size = 0;
	int32_t         type_count = 0, param_count = 0, ret_count = 0;
	module->type_count = type_count = read_leb_u32((uint8_t**)&p);
	module->type_section            = (DEEPType**)malloc(type_count * sizeof(DEEPType*));
	for (int32_t i = 0; i < type_count; i++) {
		if (READ_BYTE(p) == 0x60) {
			param_count = read_leb_u32((uint8_t**)&p);
			p_tmp     = p;
			p += param_count;
			ret_count = read_leb_u32((uint8_t**)&p);
			p          = p_tmp;
			total_size = 8 + param_count + ret_count;

			module->type_section[i]             = (DEEPType*)malloc(total_size);
			module->type_section[i]->param_count  = param_count;
			module->type_section[i]->ret_count = ret_count;

			for (int32_t j = 0; j < param_count; j++) {
				module->type_section[i]->type[j] = READ_BYTE(p);
			}
			read_leb_u32((uint8_t**)&p);
			for (int32_t j = 0; j < ret_count; j++) {
				module->type_section[i]->type[param_count + j] = READ_BYTE(p);
			}
		}
	}
}

//读取函数段
static void decode_func_section(const uint8_t* p, DEEPModule* module,const uint8_t* p_code) {
	int32_t           func_count      = read_leb_u32((uint8_t**)&p);
	int32_t           code_func_count = read_leb_u32((uint8_t**)&p_code);
	int32_t           type_index, code_size, local_set_count;
	DEEPFunction* func;
	LocalVars*    local_set;
	const uint8_t*   p_code_temp;
	if (func_count == code_func_count) {
		module->function_count = func_count;
		module->func_section   = (DEEPFunction**)malloc(func_count * sizeof(DEEPFunction*));

		for (int32_t i = 0; i < func_count; i++) {
			func = module->func_section[i] = (DEEPFunction*)malloc(sizeof(DEEPFunction));
			memset(func, 0, sizeof(DEEPFunction));
			type_index  = read_leb_u32((uint8_t**)&p);
			code_size   = read_leb_u32((uint8_t**)&p_code);
			p_code_temp = p_code;
			func->func_type = module->type_section[type_index];
			func->code_size = code_size;
			local_set_count = read_leb_u32((uint8_t**)&p_code);
			if (local_set_count == 0) {
				func->localvars = NULL;
			} else {
				func->localvars = (LocalVars**)malloc(local_set_count * sizeof(LocalVars*));
				for (int32_t j = 0; j < local_set_count; j++) {
					local_set = func->localvars[j] = (LocalVars*)malloc(sizeof(LocalVars));
					local_set->count                 = read_leb_u32((uint8_t**)&p_code);
					local_set->local_type          = READ_BYTE(p_code);
				}
			}
			func->code_begin = (uint8_t*)p_code;
			p_code           = p_code_temp + code_size;
		}
	}
}

//读取导出段
static void decode_export_section(const uint8_t* p, DEEPModule* module) {
	int32_t export_count = read_leb_u32((uint8_t**)&p);
	int32_t name_len;
	DEEPExport* Export;
	module->export_count = export_count;
	module->export_section = (DEEPExport**)malloc(export_count * sizeof(DEEPExport*));

	for (int32_t i = 0; i < export_count; i ++) {
		Export = module->export_section[i] = (DEEPExport*)malloc(sizeof(DEEPExport));
		name_len = read_leb_u32((uint8_t**)&p);
		Export->name = str_gen(p, name_len);
		p += name_len;
		Export->tag = READ_BYTE(p);
		Export->index = read_leb_u32((uint8_t**)&p);
	}
}

//读取数据段
static void decode_data_section(const uint8_t* p, DEEPModule* module) {
	int32_t data_count = read_leb_u32((uint8_t**)&p);
	DEEPData* Data;
	module->data_count = data_count;
	module->data_section = (DEEPData**)malloc(data_count * sizeof(DEEPData*));
	
	for (int32_t i = 0; i < data_count; i ++) {
		Data = module->data_section[i] = (DEEPData*)malloc(sizeof(DEEPData));
		READ_BYTE(p);
		p ++;
		Data->offset = read_leb_u32((uint8_t**)&p);
		p ++;
		Data->datasize = read_leb_u32((uint8_t**)&p);
		Data->data = (uint8_t*)p;
	}
}

//依次遍历每个section，遇到需要的就load进module
static void decode_each_sections(DEEPModule* module, section_listnode* section_list) {

	section_listnode* section = section_list;
	const uint8_t *      buf = NULL, *p_code = NULL;
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
		error("magic number error");
		return NULL;
	}
	section_listnode* section_list = create_section_list((const uint8_t**)p, size);
	if(section_list == NULL) {
		error("create section list fail");
		return NULL;
	}
	size -= 8;
	DEEPModule* module = (DEEPModule*)malloc(sizeof(DEEPModule));
	if(module == NULL) {
		error("module malloc fail");
		return NULL;
	}
	memset(module, 0, sizeof(module));
	decode_each_sections(module, section_list);
	section_listnode* dummy = section_list, *q;
	while(dummy != NULL) {
		q = dummy->next;
		free(dummy);
		dummy = q;
	}
	section_list = NULL;
	return module;
}

//内存模块实现
#define PAGESIZE 65536

uint8_t* init_memory(uint32_t min_page) {
    uint8_t* mem = (uint8_t*)malloc(min_page * PAGESIZE);
    memset(mem, 0, min_page * PAGESIZE);
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
    read_memory(mem, buf, offset, 2);
    return buf[0];
}

uint32_t read_mem32(uint8_t* mem, uint32_t offset) {
    uint32_t buf[1];
    read_memory(mem, buf, offset, 4);
    return buf[0];
}

uint64_t read_mem64(uint8_t* mem, uint32_t offset) {
    uint64_t buf[1];
    read_memory(mem, buf, offset, 8);
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
	write_memory(mem, buf, offset, 2);
}

void write_mem32(uint8_t* mem, uint32_t val, uint32_t offset) {
	uint32_t buf[1];
	buf[0] = val;
	write_memory(mem, buf, offset, 4);
}

void write_mem64(uint8_t* mem, uint64_t val, uint32_t offset) {
	uint64_t buf[1];
	buf[0] = val;
	write_memory(mem, buf, offset, 8);
}

// DEEPModule* module = deep_load(&p, size);
//DOING：开发dump指令，方便后续调试程序

// void print_type(uint8_t t) {
// 	switch (t)
// 	{
// 	case :
		
// 		break;
	
// 	default:
// 		error("not correct parameter type");
// 		break;
// 	}
// }

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