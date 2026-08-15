(module
  (type $t0 (func (param i64) (result i32)))
  (type $t1 (func (param i64 i64) (result i64)))
  (type $t2 (func (result i32)))
  (type $t3 (func (param i32) (result i32)))
  (import "env" "putl" (func (type $t0)))
  (import "env" "puts" (func (type $t3)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t1) (param i64 i64) (result i64)
    (i64.add
      (local.get 1)
      (local.get 0)))
  (func $f1 (type $t2) (result i32)
    (local i64)
    (local.set 0
      (call 2
        (i64.const 7)
        (i64.const 8)))
    (drop
      (call 1
        (i32.const 16)))
    (drop
      (call 0
        (local.get 0)))
    (i32.const 0))
  (export "memory" (memory 0))
  (export "add" (func 2))
  (export "main" (func 3))
  (data (i32.const 16) "add(7,8)=\00")
)
