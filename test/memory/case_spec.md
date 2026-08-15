## memory testcase spec

覆盖 wasm 1.0 的窄宽度 load/store 指令与 `memory.size`/`memory.grow`。
load/store 的 memarg（align、offset）均为 0；内存初值为 0，测试先 `store` 写入再 `load` 读出。

- `i32_load8_s_001/002/003`：store8 0x80/0xFF/0x7F 后符号扩展读出（-128/-1/127）
- `i32_load8_u_001/002/003`：store8 0x80/0xFF/0x00 后零扩展读出（128/255/0）
- `i32_load16_s_001/002/003`：store16 0x8000/0xFFFF/0x7FFF 后符号扩展读出
- `i32_load16_u_001/002/003`：store16 0x8000/0xFFFF/0x1234 后零扩展读出
- `i64_load8_s_001/002/003`：store8 0x80/0xFF/0x7F 后符号扩展读出（i64 打印 -128/-1/127）
- `i64_load8_u_001/002/003`：store8 0x80/0xFF/0x00 后零扩展读出
- `i64_load16_s_001/002/003`：store16 0x8000/0xFFFF/0x7FFF 后符号扩展读出
- `i64_load16_u_001/002/003`：store16 0x8000/0xFFFF/0x1234 后零扩展读出
- `i64_load32_s_001/002/003`：store32 0x80000000/0xFFFFFFFF/0x7FFFFFFF 后符号扩展读出
- `i64_load32_u_001/002/003`：store32 0x80000000/0xFFFFFFFF/0x12345678 后零扩展读出
- `i32_store8_001/002/003`：store8 0x12/0xFF/0x00 再 load 验证低字节截断
- `i32_store16_001/002/003`：store16 0x1234/0xFFFF/0x00 再 load 验证低 16 位截断
- `i64_store8_001/002/003`：store8 0x12/0xFF/0x00 再 load 验证低字节截断
- `i64_store16_001/002/003`：store16 0x1234/0xFFFF/0x00 再 load 验证低 16 位截断
- `i64_store32_001/002/003`：store32 0x12345678/0xFFFFFFFF/0x00 再 load 验证低 32 位截断
- `memory_size_001/002/003`：`memory.size` 返回初始页数（1/2/3 页）
- `memory_grow_001/002/003`：`memory.grow 0` 返回旧页数（realloc 桩未实现扩容）
- `memory_grow_pos_001`：`memory.grow 1` 因 realloc 桩未实现返回 -1（打印 4294967295）
