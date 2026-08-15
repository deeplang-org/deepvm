;; 线性内存 store / load
(module
  (memory 1)
  (func (export "main") (result i32)
    i32.const 0
    i32.const 99
    i32.store
    i32.const 0
    i32.load))
