(module
  (type $t0 (func (result i64)))
  (func $f0 (type $t0) (result i64)
    (i64.shr_s
      (i64.const -8)
      (i64.const 1)))
  (export "main" (func 0))
)
