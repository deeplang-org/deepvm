(module
  (type $t0 (func (param i64 i64) (result i64)))
  (type $t1 (func (result i64)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t0) (param i64 i64) (result i64)
    (i64.rem_s
      (local.get 0)
      (local.get 1)))
  (func $f1 (type $t1) (result i64)
    (call 0
      (i64.const 2147483648)
      (i64.const 1000)))
  (export "memory" (memory 0))
  (export "rem" (func 0))
  (export "main" (func 1))
)
