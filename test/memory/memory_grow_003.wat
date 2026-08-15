(module
  (type $t0 (func (result i32)))
  (memory 3)
  (func $f0 (type $t0) (result i32)
    (memory.grow
      (i32.const 0)))
  (export "main" (func 0))
)
