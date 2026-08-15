(module
  (type $t0 (func (param i32) (result i32)))
  (type $t1 (func (result i32)))
  (import "env" "puts" (func (type $t0)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t1) (result i32)
    (drop
      (call 0
        (i32.const 16)))
    (i32.const 0))
  (export "memory" (memory 0))
  (export "main" (func 1))
  (data (i32.const 16) "hello deeplang\r\n\00")
)
