(module
  (type $t0 (func (param f64 f64) (result f64)))
  (type $t1 (func (result f64)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t0) (param f64 f64) (result f64)
    (f64.sub
      (local.get 0)
      (local.get 1)))
  (func $f1 (type $t1) (result f64)
    (call 0
      (f64.const 2023.7)
      (f64.const 0.0)))
  (export "memory" (memory 0))
  (export "add" (func 0))
  (export "main" (func 1))
)
