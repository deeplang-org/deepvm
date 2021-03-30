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
typedef struct WASMType {
	int  param_num;
	int  return_num;
	char type[1];
} WASMType;

//local variables item
typedef struct LocalVars {
	int   num;
	short local_type;
} LocalVars;

//function item
typedef struct WASMFunction {
	WASMType*   func_type; // the type of function
	LocalVars** localvars;
	int         code_size;
	char*       code_begin;
} WASMFunction;

/* Data structure of module, at present we only support 
two sections, which can make the program run*/
typedef struct WASMModule {
	int            type_count;
	int            function_count;
	WASMType**     type_section;
	WASMFunction** func_section;
} WASMModule;

//the difinition of listnode
typedef struct section_listnode {
	char                     section_type;
	int                      section_size;
	char*                    section_begin;
	struct section_listnode* next;
} section_listnode;

//read a value of specified type
#define TEMPLATE_READ_VALUE(Type, p) \
	(p += sizeof(Type), *(Type*)(p - sizeof(Type)))
#define READ_INT(p)  TEMPLATE_READ_VALUE(int, p)
#define READ_CHAR(p) TEMPLATE_READ_VALUE(char, p)

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
static void load_type_section(const char* p, WASMModule* module) {

	const char* p_tmp;
	int         total_size = 0;
	int         type_count = 0, param_num = 0, return_num = 0;
	module->type_count = type_count = read_leb_u32((char**)&p);
	module->type_section            = (WASMType**)malloc(type_count * sizeof(WASMType*));
	for (int i = 0; i < type_count; i++) {
		if (READ_CHAR(p) == 0x60) {
			param_num = read_leb_u32((char**)&p);
			p_tmp     = p;
			p += param_num;
			return_num = read_leb_u32((char**)&p);
			p          = p_tmp;
			total_size = 8 + param_num + return_num;

			module->type_section[i]             = (WASMType*)malloc(total_size);
			module->type_section[i]->param_num  = param_num;
			module->type_section[i]->return_num = return_num;

			for (int j = 0; j < param_num; j++) {
				module->type_section[i]->type[j] = READ_CHAR(p);
			}
			read_leb_u32((char**)&p);
			for (int j = 0; j < return_num; j++) {
				module->type_section[i]->type[param_num + j] = READ_CHAR(p);
			}
		}
	}
}

//读取函数段
static void load_func_section(const char* p, WASMModule* module, const char* p_code) {
	int           func_count      = read_leb_u32((char**)&p);
	int           code_func_count = read_leb_u32((char**)&p_code);
	int           type_index, code_size, local_set_count;
	WASMFunction* func;
	LocalVars*    local_set;
	const char*   p_code_temp;
	if (func_count == code_func_count) {
		module->function_count = func_count;
		module->func_section   = (WASMFunction**)malloc(func_count * sizeof(WASMFunction*));

		for (int i = 0; i < func_count; i++) {
			func = module->func_section[i] = (WASMFunction*)malloc(sizeof(WASMFunction));
			memset(func, 0, sizeof(WASMFunction));
			type_index  = read_leb_u32((char**)&p);
			code_size   = read_leb_u32((char**)&p_code);
			p_code_temp = p_code;
			cout << code_size << endl;
			func->func_type = module->type_section[type_index];
			func->code_size = code_size;
			local_set_count = read_leb_u32((char**)&p_code);
			cout << local_set_count << endl;
			if (local_set_count == 0) {
				func->localvars = NULL;
			} else {
				func->localvars = (LocalVars**)malloc(local_set_count * sizeof(LocalVars*));
				for (int j = 0; j < local_set_count; j++) {
					local_set = func->localvars[j] = (LocalVars*)malloc(sizeof(LocalVars));
					local_set->num                 = read_leb_u32((char**)&p_code);
					local_set->local_type          = READ_CHAR(p_code);
				}
			}
			func->code_begin = (char*)p_code;
			p_code           = p_code_temp + code_size;
		}
	}
}

//依次遍历每个section，遇到需要的就load进module
static void load_from_sections(WASMModule* module, section_listnode* section_list) {

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
			load_type_section(buf, module);

			break;
		case SECTION_TYPE_IMPORT:

			break;
		case SECTION_TYPE_FUNC:

			load_func_section(buf, module, p_code);

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
static WASMModule* load(char** p, int size) {
	if (!check_magic_number_and_version(p))
		return NULL;
	section_listnode* section_list = (section_listnode*)malloc(sizeof(section_list));
	memset(section_list, 0, sizeof(section_list));
	size -= 8;
	create_section_list((const char**)p, size, section_list);

	WASMModule* module = (WASMModule*)malloc(sizeof(WASMModule));
	memset(module, 0, sizeof(module));
	load_from_sections(module, section_list->next);

	free(section_list); //链表的free要自己实现，目前还未实现
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
	WASMModule* module = load(&p, size);
}
