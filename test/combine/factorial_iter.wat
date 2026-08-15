;; 组合用例：多参数函数调用 + loop/block + br_if/br + 局部变量 + i32 乘法
;; 迭代计算 5! = 120
(module
  ;; 辅助函数：mul(a, b) = a * b
  (func $mul (param i32 i32) (result i32)
    local.get 0
    local.get 1
    i32.mul)
  (func (export "main") (result i32)
    (local $i i32)
    (local $acc i32)
    i32.const 1
    local.set $acc
    i32.const 1
    local.set $i
    (block $done
      (loop $top
        local.get $i
        i32.const 6
        i32.ge_s
        br_if $done
        ;; acc = mul(acc, i)
        (call $mul (local.get $acc) (local.get $i))
        local.set $acc
        local.get $i
        i32.const 1
        i32.add
        local.set $i
        br $top))
    local.get $acc))
