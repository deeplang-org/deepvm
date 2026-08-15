(module
  (type $t0 (func (param f64 f64) (result f64)))
  (type $t1 (func (result i32)))
  (type $t2 (func (param f64) (result i32)))
  (import "env" "isnand" (func (type $t2)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t0) (param f64 f64) (result f64)
    (f64.div
      (local.get 0)
      (local.get 1)))
  (func $f1 (type $t1) (result i32)
    (call 0
      (call 1
        (f64.const 0.0)
        (f64.const 0.0))))
  (export "memory" (memory 0))
  (export "div" (func 1))
  (export "main" (func 2))
)
