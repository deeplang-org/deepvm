## wat testcase spec

覆盖 WAT（S 表达式）→ wasm 二进制的汇编与运行回环（round-trip）。
每个 `.wat` 先用 `deepvm -o <tmp>.wasm <file>.wat` 汇编，再 `deepvm <tmp>.wasm` 运行断言输出。

- `add_folded`：折叠写法 `(i32.add (i32.const 1) (i32.const 2))` → 3
- `mul_flat`：平铺写法 `i32.const 7 / i32.const 5 / i32.mul` → 35
- `puts_string`：import `env.puts` + data 段字符串 → 输出 `hello deeplang\n0`
- `if_else`：折叠 `if`/`then`/`else` → 100
- `global_get`：可变全局量读取 + 运算 → 42
- `memory_store_load`：线性内存 `i32.store`/`i32.load` → 99
- `call_indirect`：函数表 + `call_indirect` → 42
- `start_section`：`start` 段初始化全局量 → 5
- `loop_sum`：`block`/`loop`/`br`/`br_if` 求和 1..10 → 55
- `i64_const`：i64 常量与运算（-1 + 2）→ 1
