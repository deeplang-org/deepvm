(module
  (type $t0 (func (param i32 i32) (result i32)))
  (type $t1 (func (result i32)))
  (table 0 funcref)
  (memory 1)
  (func $f0 (type $t0) (param i32 i32) (result i32)
    (i32.mul
      (i32.add
        (local.get 0)
        (i32.const 1))
      (local.get 1)))
  (func $f1 (type $t1) (result i32)
    (local i32 i32)
    (loop
      (local.set 0
        (i32.add
          (local.get 0)
          (i32.const 1)))
      (local.set 1
        (i32.add
          (local.get 1)
          (local.get 0)))
      (br_if 0
        (i32.lt_s
          (local.get 0)
          (i32.const 10))))
    (local.get 1))
  (export "memory" (memory 0))
  (export "getVal" (func 0))
  (export "main" (func 1))
)
