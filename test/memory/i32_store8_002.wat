(module
  (type $t0 (func (result i32)))
  (memory 1)
  (func $f0 (type $t0) (result i32)
    (i32.store8
      (i32.const 0)
      (i32.const 511))
    (i32.load8_u
      (i32.const 0)))
  (export "main" (func 0))
)
