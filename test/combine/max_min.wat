;; 组合用例：内存 store/load + loop + if/else(result) + select + 比较 + local.tee
;; 数组 [7,2,9,4,11,3] 求 max=11、min=2，返回 max*100+min = 1102
(module
  (memory 1)
  (func (export "main") (result i32)
    (local $i i32)
    (local $v i32)
    (local $max i32)
    (local $min i32)
    i32.const 0 i32.const 7 i32.store
    i32.const 4 i32.const 2 i32.store
    i32.const 8 i32.const 9 i32.store
    i32.const 12 i32.const 4 i32.store
    i32.const 16 i32.const 11 i32.store
    i32.const 20 i32.const 3 i32.store
    ;; max = min = mem[0]
    i32.const 0
    i32.load
    local.tee $max
    local.set $min
    i32.const 1
    local.set $i
    (block $done
      (loop $top
        local.get $i
        i32.const 6
        i32.ge_s
        br_if $done
        ;; v = mem[i*4]
        local.get $i
        i32.const 4
        i32.mul
        i32.load
        local.set $v
        ;; max = (v > max) ? v : max   （if/else 折叠）
        (if (result i32)
          (i32.gt_s (local.get $v) (local.get $max))
          (then (local.get $v))
          (else (local.get $max)))
        local.set $max
        ;; min = (v < min) ? v : min   （select）
        (select
          (local.get $v)
          (local.get $min)
          (i32.lt_s (local.get $v) (local.get $min)))
        local.set $min
        local.get $i
        i32.const 1
        i32.add
        local.set $i
        br $top))
    local.get $max
    i32.const 100
    i32.mul
    local.get $min
    i32.add))
