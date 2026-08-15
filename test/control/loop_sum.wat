;; 控制流：loop + block + br（1..10 求和 = 55）
(module
  (func (export "main") (result i32)
    (local $i i32)
    (local $sum i32)
    i32.const 0
    local.set $sum
    i32.const 1
    local.set $i
    (block $done
      (loop $top
        local.get $i
        i32.const 11
        i32.ge_s
        br_if $done
        local.get $sum
        local.get $i
        i32.add
        local.set $sum
        local.get $i
        i32.const 1
        i32.add
        local.set $i
        br $top))
    local.get $sum))
