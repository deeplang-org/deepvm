;; 组合用例：table/elem + call_indirect + 多函数 + i32 加减乘除
;; 通过函数表依次调用 add/sub/mul/div：15+5+50+2 = 72
(module
  (type $binop (func (param i32 i32) (result i32)))
  (table 4 funcref)
  (func $add (type $binop) (param i32 i32) (result i32)
    local.get 0 local.get 1 i32.add)
  (func $sub (type $binop) (param i32 i32) (result i32)
    local.get 0 local.get 1 i32.sub)
  (func $mul (type $binop) (param i32 i32) (result i32)
    local.get 0 local.get 1 i32.mul)
  (func $div (type $binop) (param i32 i32) (result i32)
    local.get 0 local.get 1 i32.div_s)
  (func (export "main") (result i32)
    (local $acc i32)
    i32.const 0
    local.set $acc
    (call_indirect (type $binop) (i32.const 10) (i32.const 5) (i32.const 0))
    local.set $acc
    (call_indirect (type $binop) (i32.const 10) (i32.const 5) (i32.const 1))
    local.get $acc
    i32.add
    local.set $acc
    (call_indirect (type $binop) (i32.const 10) (i32.const 5) (i32.const 2))
    local.get $acc
    i32.add
    local.set $acc
    (call_indirect (type $binop) (i32.const 10) (i32.const 5) (i32.const 3))
    local.get $acc
    i32.add)
  (elem (i32.const 0) 0 1 2 3))
