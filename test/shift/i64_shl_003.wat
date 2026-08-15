(module
  (type $t0 (func (result i64)))
  (func $f0 (type $t0) (result i64)
    (i64.shl
      (i64.const 15)
      (i64.const 8)))
  (export "main" (func 0))
)
