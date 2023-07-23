(module
  (type (;0;) (func (param i32) (result i32)))
  (type (;1;) (func (param f64) (result i32)))
  (type (;2;) (func (param f64 f64) (result f64)))
  (type (;3;) (func (result i32)))
  (import "env" "putd" (func (;0;) (type 1)))
  (import "env" "puts" (func (;1;) (type 0)))
  (func (;2;) (type 2) (param f64 f64) (result f64)
    local.get 0
    local.get 1
    f64.add)
  (func (;3;) (type 3) (result i32)
    (local f64)
    f64.const 0x1.c66666p+2 (;=7.1;)
    f64.const 0x1.066666p+3 (;=8.2;)
    call 2
    local.set 0
    i32.const 16
    call 1
    drop
    local.get 0
    call 0
    drop
    i32.const 0)
  (table (;0;) 0 funcref)
  (memory (;0;) 1)
  (export "memory" (memory 0))
  (export "add" (func 2))
  (export "main" (func 3))
  (data (;0;) (i32.const 16) "add(7.1,8.2)=\00"))
