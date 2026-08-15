/**
 * Filename:deep_version.h
 * Description:DeepVM 版本号定义与版本输出
 */

#ifndef _DEEP_VERSION_H
#define _DEEP_VERSION_H

#ifdef __cplusplus
extern "C" {
#endif

/* DeepVM 自身版本号（语义化版本） */
#define DEEP_VERSION_MAJOR 1
#define DEEP_VERSION_MINOR 0
#define DEEP_VERSION_PATCH 0
#define DEEP_VERSION "1.0.0"

/* 支持的 WebAssembly 规范版本 */
#define DEEP_WASM_VERSION_MAJOR 1
#define DEEP_WASM_VERSION_MINOR 0
#define DEEP_WASM_VERSION "1.0"

/**
 * @brief 打印 DeepVM 版本号及支持的 WebAssembly 版本
 */
void deep_print_version(void);

#ifdef __cplusplus
}
#endif

#endif /* _DEEP_VERSION_H */
