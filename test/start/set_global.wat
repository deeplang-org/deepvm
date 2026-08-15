(module
  (type $t0 (func))
  (type $t1 (func (result i32)))
  (global $g0 (mut i32) (i32.const 0))
  (func $f0 (type $t0)
    (global.set 0
      (i32.const 99)))
  (func $f1 (type $t1) (result i32)
    (global.get 0))
  (export "main" (func 1))
  (start 0)
)
