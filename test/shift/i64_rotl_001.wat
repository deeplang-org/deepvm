(module
  (type $t0 (func (result i64)))
  (func $f0 (type $t0) (result i64)
    (i64.rotl
      (i64.const 9223372036854775809)
      (i64.const 1)))
  (export "main" (func 0))
)
