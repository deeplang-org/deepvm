(module
  (type $t0 (func (result i64)))
  (func $f0 (type $t0) (result i64)
    (i64.shr_s
      (i64.const 4611686018427387904)
      (i64.const 62)))
  (export "main" (func 0))
)
