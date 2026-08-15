;; 组合用例：内存 store/load（计算地址）+ loop + global.get/set + i32 算术
;; 把 1..6 的平方写入内存，再读回累加：1+4+9+16+25+36 = 91
(module
  (memory 1)
  (global $sum (mut i32) (i32.const 0))
  (func (export "main") (result i32)
    (local $i i32)
    ;; 写入阶段：mem[(i-1)*4] = i*i
    i32.const 1
    local.set $i
    (block $w
      (loop $l
        local.get $i
        i32.const 7
        i32.ge_s
        br_if $w
        (i32.store
          (i32.mul (i32.sub (local.get $i) (i32.const 1)) (i32.const 4))
          (i32.mul (local.get $i) (local.get $i)))
        local.get $i
        i32.const 1
        i32.add
        local.set $i
        br $l))
    ;; 读回阶段：sum += mem[i*4]
    i32.const 0
    local.set $i
    (block $w2
      (loop $l2
        local.get $i
        i32.const 6
        i32.ge_s
        br_if $w2
        (global.set $sum
          (i32.add (global.get $sum)
            (i32.load (i32.mul (local.get $i) (i32.const 4)))))
        local.get $i
        i32.const 1
        i32.add
        local.set $i
        br $l2))
    global.get $sum))
