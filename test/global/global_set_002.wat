(module
  (type $t0 (func (result i32)))
  (global $g0 (mut i32) (i32.const 0))
  (func $f0 (type $t0) (result i32)
    (global.set 0
      (i32.const -1))
    (global.get 0))
  (export "main" (func 0))
)
