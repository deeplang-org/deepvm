(module
  (type $t0 (func (param i32) (result i32)))
  (type $t1 (func (param f32) (result i32)))
  (type $t2 (func (param f32 f32) (result f32)))
  (type $t3 (func (result i32)))
  (import "env" "putf" (func (type $t1)))
  (import "env" "puts" (func (type $t0)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t2) (param f32 f32) (result f32)
    (f32.add
      (local.get 0)
      (local.get 1)))
  (func $f1 (type $t3) (result i32)
    (local f32)
    (local.set 0
      (call 2
        (f32.const 7.099999904632568)
        (f32.const 8.199999809265137)))
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
  (data (i32.const 16) "add(7.1,8.2)=\00")
)
