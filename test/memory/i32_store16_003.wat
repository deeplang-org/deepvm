(module
  (type $t0 (func (result i32)))
  (memory 1)
  (func $f0 (type $t0) (result i32)
    (i32.store16
      (i32.const 0)
      (i32.const 65536))
    (i32.load16_u
      (i32.const 0)))
  (export "main" (func 0))
)
