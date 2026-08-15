(module
  (type $t0 (func (result i64)))
  (memory 1)
  (func $f0 (type $t0) (result i64)
    (i32.store16
      (i32.const 0)
      (i32.const 32768))
    (i64.load16_s
      (i32.const 0)))
  (export "main" (func 0))
)
