(module
  (type $t0 (func (result i64)))
  (global $g0 (mut i64) (i64.const 0))
  (func $f0 (type $t0) (result i64)
    (global.set 0
      (i64.const 4294967296))
    (global.get 0))
  (export "main" (func 0))
)
