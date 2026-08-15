;; 函数表 + call_indirect
(module
  (type $bin (func (param i32) (result i32)))
  (table 1 funcref)
  (func $double (param i32) (result i32)
    local.get 0
    i32.const 2
    i32.mul)
  (elem (i32.const 0) $double)
  (func (export "main") (result i32)
    i32.const 21
    (call_indirect (type $bin) (i32.const 0))))
