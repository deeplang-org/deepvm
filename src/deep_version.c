/**
 * Filename:deep_version.c
 * Description:DeepVM 版本号输出
 */

#include <stdio.h>
#include "deep_version.h"

void deep_print_version(void)
{
    printf("DeepVM %s\n", DEEP_VERSION);
    printf("WebAssembly %s\n", DEEP_WASM_VERSION);
}
