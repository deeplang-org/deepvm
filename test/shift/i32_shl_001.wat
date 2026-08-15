(module
  (type $t0 (func (result i32)))
  (func $f0 (type $t0) (result i32)
    (i32.shl
      (i32.const 1)
      (i32.const 4)))
  (export "main" (func 0))
)
