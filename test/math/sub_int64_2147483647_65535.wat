(module
  (type (;0;) (func (param i64 i64) (result i64)))
  (type (;1;) (func (result i64)))
  (func (;0;) (type 0) (param i64 i64) (result i64)
    local.get 0
    local.get 1
    i64.sub)
  (func (;1;) (type 1) (result i64)
    i64.const 2147483647
    i64.const 65535
    call 0)
  (table (;0;) 0 funcref)
  (memory (;0;) 1)
  (export "memory" (memory 0))
  (export "sub" (func 0))
  (export "main" (func 1)))
