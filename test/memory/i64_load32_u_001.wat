(module
  (type $t0 (func (result i64)))
  (memory 1)
  (func $f0 (type $t0) (result i64)
    (i64.store32
      (i32.const 0)
      (i64.const 2147483648))
    (i64.load32_u
      (i32.const 0)))
  (export "main" (func 0))
)
