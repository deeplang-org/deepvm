(module
  (type $t0 (func (param i32 i32) (result i32)))
  (type $t1 (func (result i32)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t0) (param i32 i32) (result i32)
    (i32.sub
      (local.get 0)
      (local.get 1)))
  (func $f1 (type $t1) (result i32)
    (call 0
      (i32.const -10)
      (i32.const -8)))
  (export "memory" (memory 0))
  (export "sub" (func 0))
  (export "main" (func 1))
)
