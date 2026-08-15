## table/call_indirect testcase spec

覆盖 wasm 1.0 的 Table/Elem section 与 `call_indirect`。

- `call_indirect_001`：表 [func0->42, func1->100, func2->7]，`call_indirect 0` 得 42
- `call_indirect_002`：`call_indirect 1` 得 100
- `call_indirect_003`：`call_indirect 2` 得 7
- `call_indirect_oob_001`：索引越界，触发运行时错误（退出码 1）
