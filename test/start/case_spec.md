## start testcase spec

覆盖 wasm 1.0 的 Start section：起始函数在 main 之前执行。

- `set_global`：start 函数 `global.set 99`，main 再 `global.get` 读出 99
