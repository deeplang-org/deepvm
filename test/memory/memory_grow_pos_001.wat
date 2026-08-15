(module
  (type $t0 (func (result i32)))
  (memory 1)
  (func $f0 (type $t0) (result i32)
    (memory.grow
      (i32.const 1)))
  (export "main" (func 0))
)
