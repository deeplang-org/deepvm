(module
  (type $t0 (func (param f32 f32) (result f32)))
  (type $t1 (func (result i32)))
  (type $t2 (func (param f32) (result i32)))
  (import "env" "isinff" (func (type $t2)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t0) (param f32 f32) (result f32)
    (f32.div
      (local.get 0)
      (local.get 1)))
  (func $f1 (type $t1) (result i32)
    (call 0
      (call 1
        (f32.const 22.700000762939453)
        (f32.const 0.0))))
  (export "memory" (memory 0))
  (export "div" (func 1))
  (export "main" (func 2))
)
