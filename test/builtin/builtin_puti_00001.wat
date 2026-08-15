(module
  (type $t0 (func (param i32) (result i32)))
  (type $t1 (func (param i32 i32) (result i32)))
  (type $t2 (func (result i32)))
  (import "env" "puti" (func (type $t0)))
  (import "env" "puts" (func (type $t0)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t1) (param i32 i32) (result i32)
    (i32.add
      (local.get 1)
      (local.get 0)))
  (func $f1 (type $t2) (result i32)
    (local i32)
    (local.set 0
      (call 2
        (i32.const 7)
        (i32.const 8)))
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
