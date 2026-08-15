(module
  (type $t0 (func (result i64)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t0) (result i64)
    (local i32)
    (local.set 0
      (i32.const 0))
    (i64.store offset=16
      (local.get 0)
      (i64.const 2147483648))
    (i64.store offset=8
      (local.get 0)
      (i64.const -2147483649))
    (i64.store
      (local.get 0)
      (i64.add
        (i64.load offset=16
          (local.get 0))
        (i64.const -2147483649)))
    (i64.load
      (local.get 0)))
  (export "memory" (memory 0))
  (export "main" (func 0))
)
