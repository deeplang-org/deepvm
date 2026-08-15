(module
  (type $t0 (func (param f32 f32) (result f32)))
  (type $t1 (func (result i32)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t0) (param f32 f32) (result f32)
    (f32.sub
      (local.get 0)
      (local.get 1)))
  (func $f1 (type $t1) (result i32)
    (i32.trunc_f32_s
      (call 0
        (f32.const 99.01000213623047)
        (f32.const 22.299999237060547))))
  (export "memory" (memory 0))
  (export "sub" (func 0))
  (export "main" (func 1))
)
