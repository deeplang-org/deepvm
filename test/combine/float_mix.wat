;; 组合用例：f32/f64 算术 + 转换(trunc) + 比较 + select + i32 加法
;; trunc(2.5*3.0+1.5)=9，trunc(3.5+4.5)=8，9+8+select(100,200,9.0>8.0)=117
(module
  (func (export "main") (result i32)
    ;; 9
    (i32.trunc_f64_s
      (f64.add
        (f64.mul (f64.const 2.5) (f64.const 3.0))
        (f64.const 1.5)))
    ;; 8
    (i32.trunc_f32_s
      (f32.add (f32.const 3.5) (f32.const 4.5)))
    i32.add
    ;; 100（9.0 > 8.0 为真）
    (select
      (i32.const 100)
      (i32.const 200)
      (f64.gt (f64.const 9.0) (f64.const 8.0)))
    i32.add))
