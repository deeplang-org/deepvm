(module
  (type $t0 (func (param i32 i32) (result i32)))
  (type $t1 (func (result i32)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t1) (result i32)
    (local i32 i32)
    (local.set 0
      (i32.const 5))
    (local.set 1
      (i32.const 10))
    (if (result i32)
      (i32.lt_s
        (local.get 0)
        (local.get 1))
    (then
      (i32.const 10))
    (else
      (i32.const 20))))
  (export "memory" (memory 0))
  (export "main" (func 0))
)
