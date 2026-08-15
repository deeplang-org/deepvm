## global testcase spec

覆盖 wasm 1.0 的 Global section 及 `global.get`/`global.set`（按类型区分 i32/i64）。
i32 全局量打印无符号，i64 全局量打印有符号。

- `global_get_001`：i32 全局量初始 42，`global.get` 读出 42
- `global_get_002`：i32 全局量初始 -1，`global.get` 读出 4294967295
- `global_get_003`：i64 全局量初始 4294967296，`global.get` 读出 4294967296
- `global_set_001`：可变 i32 全局量 `global.set 100` 再 `global.get` 得 100
- `global_set_002`：可变 i32 全局量 `global.set -1` 再 `global.get` 得 4294967295
- `global_set_003`：可变 i64 全局量 `global.set 4294967296` 再 `global.get` 得 4294967296
