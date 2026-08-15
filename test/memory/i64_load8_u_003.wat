(module
  (type $t0 (func (result i64)))
  (memory 1)
  (func $f0 (type $t0) (result i64)
    (i32.store8
      (i32.const 0)
      (i32.const 0))
    (i64.load8_u
      (i32.const 0)))
  (export "main" (func 0))
)
