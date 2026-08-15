(module
  (type $t0 (func (result i64)))
  (func $f0 (type $t0) (result i64)
    (i64.shr_u
      (i64.const 16)
      (i64.const 2)))
  (export "main" (func 0))
)
