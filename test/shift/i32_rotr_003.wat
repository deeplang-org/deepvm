(module
  (type $t0 (func (result i32)))
  (func $f0 (type $t0) (result i32)
    (i32.rotr
      (i32.const 305419896)
      (i32.const 8)))
  (export "main" (func 0))
)
