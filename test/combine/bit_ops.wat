;; 组合用例：移位 + 位计数(clz/ctz/popcnt) + select + 比较 + i32 加法
;; 31 + 4 + 8 + 240 + 7 = 290
(module
  (func (export "main") (result i32)
    ;; clz(1) = 31
    i32.const 1
    i32.clz
    ;; + ctz(16) = 4
    i32.const 16
    i32.ctz
    i32.add
    ;; + popcnt(255) = 8
    i32.const 255
    i32.popcnt
    i32.add
    ;; + ((0xF0 << 4) >>u 4) = 240
    (i32.shr_u
      (i32.shl (i32.const 240) (i32.const 4))
      (i32.const 4))
    i32.add
    ;; + select(7, 3, 1<2) = 7
    (select
      (i32.const 7)
      (i32.const 3)
      (i32.lt_s (i32.const 1) (i32.const 2)))
    i32.add))
