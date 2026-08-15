(module
  (type $t0 (func (result i32)))
  (func $f0 (type $t0) (result i32)
    (i32.shr_s
      (i32.const 1073741824)
      (i32.const 30)))
  (export "main" (func 0))
)
