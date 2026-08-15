/**
 * Filename:deep_wat.h
 * Description:WAT（S 表达式）文本汇编为 wasm 二进制的公开接口
 */

#ifndef _DEEP_WAT_H
#define _DEEP_WAT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 判断路径是否为 .wat 文本（忽略大小写）
 */
bool deep_wat_is_path(const char *path);

/**
 * @brief 把 WAT 文本文件汇编为 .wasm 二进制文件
 *
 * @param input_path  .wat 文本路径
 * @param output_path .wasm 输出路径
 * @return 0 成功，非 0 失败
 */
int32_t deep_wat_compile_file(const char *input_path, const char *output_path);

#ifdef __cplusplus
}
#endif

#endif /* _DEEP_WAT_H */
