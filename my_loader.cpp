#include <iostream>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define MAGIC_NUMBER        0x6d736100
#define VERSION             1
#define SECTION_TYPE_USER   0
#define SECTION_TYPE_TYPE   1
#define SECTION_TYPE_IMPORT 2
#define SECTION_TYPE_FUNC   3
#define SECTION_TYPE_TABLE  4
#define SECTION_TYPE_MEMORY 5
#define SECTION_TYPE_GLOBAL 6
#define SECTION_TYPE_EXPORT 7
#define SECTION_TYPE_START  8
#define SECTION_TYPE_ELEM   9
#define SECTION_TYPE_CODE   10
#define SECTION_TYPE_DATA   11

using namespace std;

//type item
typedef struct DEEPType {
	int  param_count;
	int  ret_count;
	char type[1];
} DEEPType;

//local variables item
typedef struct LocalVars {
	int   count;
	short local_type;
} LocalVars;

//function item
typedef struct DEEPFunction {
	DEEPType*   func_type; // the type of function
	LocalVars** localvars;
	int         code_size;
	char*       code_begin;
} DEEPFunction;

/* Data structure of module, at present we only support 
two sections, which can make the program run*/
typedef struct DEEPModule {
	int            type_count;
	int            function_count;
	DEEPType**     type_section;
	DEEPFunction** func_section;
} DEEPModule;

//the difinition of listnode
typedef struct section_listnode {
	char                     section_type;
	int                      section_size;
	char*                    section_begin;
	struct section_listnode* next;
} section_listnode;

//read a value of specified type
#define READ_VALUE(Type, p) \
	(p += sizeof(Type), *(Type*)(p - sizeof(Type)))
#define READ_INT(p)  READ_VALUE(int, p)
#define READ_CHAR(p) READ_VALUE(char, p)

//解码一个无符号32位或64整型，目前只会用到32位类型,在大多数实现中占5个字节
static int read_leb_u32(char** p) {
	char* buf = *p;
	int   res = 0;
	for (int i = 0; i < 7; i++) {
		res |= (buf[i] & 0x7f) << (i * 7); //出错了，忘记加‘|’
		if ((buf[i] & 0x80) == 0) {
			*p += i + 1;
			return res;
		}
	}
	return 0;
}

//检查魔数和版本号
static bool check_magic_number_and_version(char** p) {
	int magic_number = 0, version = 0;
	magic_number = READ_INT(*p);
	version      = READ_INT(*p);
	if (magic_number == MAGIC_NUMBER && version == VERSION)
		return true;
	return false;
}

//解析出每个section的开始和大小和类型，拉成一个链表， 从第二项开始存
static bool create_section_list(const char** p, int size, section_listnode* section_list) {
	const char *buf = *p, *buf_end = buf + size;
	while (buf < buf_end) {
		section_listnode* section = (section_listnode*)malloc(sizeof(section_listnode));
		memset(section, 0, sizeof(section_listnode));
		section->section_type  = READ_CHAR(buf);
		section->section_size  = read_leb_u32((char**)&buf);
		section->section_begin = (char*)buf;
		buf += section->section_size;
		section_list->next = section;
		section_list       = section;
	}
	if (buf == buf_end)
		return true;
	return false;
}

//读取类型段
static void decode_type_section(const char* p, DEEPModule* module) {

	const char* p_tmp;
	int         total_size = 0;
	int         type_count = 0, param_count = 0, ret_count = 0;
	module->type_count = type_count = read_leb_u32((char**)&p);
	module->type_section            = (DEEPType**)malloc(type_count * sizeof(DEEPType*));
	for (int i = 0; i < type_count; i++) {
		if (READ_CHAR(p) == 0x60) {
			param_count = read_leb_u32((char**)&p);
			p_tmp     = p;
			p += param_count;
			ret_count = read_leb_u32((char**)&p);
			p          = p_tmp;
			total_size = 8 + param_count + ret_count;

			module->type_section[i]             = (DEEPType*)malloc(total_size);
			module->type_section[i]->param_count  = param_count;
			module->type_section[i]->ret_count = ret_count;

			for (int j = 0; j < param_count; j++) {
				module->type_section[i]->type[j] = READ_CHAR(p);
			}
			read_leb_u32((char**)&p);
			for (int j = 0; j < ret_count; j++) {
				module->type_section[i]->type[param_count + j] = READ_CHAR(p);
			}
		}
	}
}

//读取函数段
static void decode_func_section(const char* p, DEEPModule* module, const char* p_code) {
	int           func_count      = read_leb_u32((char**)&p);
	int           code_func_count = read_leb_u32((char**)&p_code);
	int           type_index, code_size, local_set_count;
	DEEPFunction* func;
	LocalVars*    local_set;
	const char*   p_code_temp;
	if (func_count == code_func_count) {
		module->function_count = func_count;
		module->func_section   = (DEEPFunction**)malloc(func_count * sizeof(DEEPFunction*));

		for (int i = 0; i < func_count; i++) {
			func = module->func_section[i] = (DEEPFunction*)malloc(sizeof(DEEPFunction));
			memset(func, 0, sizeof(DEEPFunction));
			type_index  = read_leb_u32((char**)&p);
			code_size   = read_leb_u32((char**)&p_code);
			p_code_temp = p_code;
			func->func_type = module->type_section[type_index];
			func->code_size = code_size;
			local_set_count = read_leb_u32((char**)&p_code);
			if (local_set_count == 0) {
				func->localvars = NULL;
			} else {
				func->localvars = (LocalVars**)malloc(local_set_count * sizeof(LocalVars*));
				for (int j = 0; j < local_set_count; j++) {
					local_set = func->localvars[j] = (LocalVars*)malloc(sizeof(LocalVars));
					local_set->count                 = read_leb_u32((char**)&p_code);
					local_set->local_type          = READ_CHAR(p_code);
				}
			}
			func->code_begin = (char*)p_code;
			p_code           = p_code_temp + code_size;
		}
	}
}

//依次遍历每个section，遇到需要的就load进module
static void decode_each_sections(DEEPModule* module, section_listnode* section_list) {

	section_listnode* section = section_list;
	const char *      buf = NULL, *p_code = NULL;
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

			break;
		case SECTION_TYPE_START:

			break;
		case SECTION_TYPE_ELEM:

			break;
		case SECTION_TYPE_CODE:

			break;
		case SECTION_TYPE_DATA:

			break;
		default:

			break;
		}
		section_list = section_list->next;
	}
}

//load
static DEEPModule* deep_load(char** p, int size) {
	if (!check_magic_number_and_version(p))
		return NULL;
	section_listnode* section_list = (section_listnode*)malloc(sizeof(section_list));
	memset(section_list, 0, sizeof(section_list));
	size -= 8;
	create_section_list((const char**)p, size, section_list);

	DEEPModule* module = (DEEPModule*)malloc(sizeof(DEEPModule));
	memset(module, 0, sizeof(module));
	decode_each_sections(module, section_list->next);

	section_listnode* dummy = section_list, *q;
	while(dummy != NULL) {
		q = dummy->next;
		free(dummy);
		dummy = q;
	}
	section_list = NULL;
	return module;
}


int main() {
	const char* path = "program.wasm";
	char*       p    = (char*)malloc(1024);
	int         file;
	int         size;
	file = open(path, O_RDONLY);
	if (file < 0) {
		cout << "open fail" << endl;
	}
	size = (int)read(file, p, 1024);
	DEEPModule* module = deep_load(&p, size);
}
