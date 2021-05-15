## 功能介绍

my_loader.cpp实现了解析.wasm二进制文件，目前能够做到将类型段，函数段以及代码段解析出来，并存入相应的数据结构。

## 主要数据结构

`WASMType`

`LocalVars`

`WASMFuntion`

`WASMModule`

`section_listnode `

## 主要函数

`static int32_t read_leb_u32(uint8_t** p)`

`static bool check_magic_number_and_version(uint8_t** p)`

` static bool create_section_list(const uint8_t** p, int *size*, section_listnode* section_list) `

`static void load_type_section(const uint8_t* p, WASMModule* module)`

` static void load_func_section(const uint8_t* p, WASMModule* module, const uint8_t* p_code) `

` static void load_from_sections(WASMModule* module, section_listnode* section_list) `

`static WASMModule* load(uint8_t** p, int size)`

`int32_t main()`    函数入口

## 与之前实现的区别

+ 简化了module的数据结构，使之结构更加清晰，移除了结构体内不必要的成员变量，减少了内存消耗
+ 相当多的安全性验证没有实现
+ 内存泄漏问题(2021.3.31已修复）

## 接下来的目标

+ 实现对其他section的解析
+ 增加安全校验
+ 解决内存泄漏问题(2021.3.31已修复）
+ 完善代码注释
