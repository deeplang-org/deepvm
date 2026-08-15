(module
  (type $t0 (func (result i32)))
  (func $f0 (type $t0) (result i32)
    (i32.shr_s
      (i32.const -8)
      (i32.const 1)))
  (export "main" (func 0))
)
