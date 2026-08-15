(module
  (type $t0 (func (param f32 f32) (result f32)))
  (type $t1 (func (result i32)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t0) (param f32 f32) (result f32)
    (f32.mul
      (local.get 0)
      (local.get 1)))
  (func $f1 (type $t1) (result i32)
    (i32.trunc_f32_s
      (call 0
        (f32.const -100.87999725341797)
        (f32.const -0.0))))
  (export "memory" (memory 0))
  (export "mult" (func 0))
  (export "main" (func 1))
)
