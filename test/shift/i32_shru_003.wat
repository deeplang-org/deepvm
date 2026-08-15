(module
  (type $t0 (func (result i32)))
  (func $f0 (type $t0) (result i32)
    (i32.shr_u
      (i32.const 16)
      (i32.const 2)))
  (export "main" (func 0))
)
