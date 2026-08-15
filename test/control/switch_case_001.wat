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
    (local.set 1
      (i32.const 10))
    (block
      (block
        (block
          (block
            (block
              (br_if 0
                (i32.gt_u
                  (local.tee 0
                    (i32.add
                      (call 0
                        (i32.const 4)
                        (i32.const 10))
                      (i32.const -10)))
                  (i32.const 60)))
              (block
                (br_table 2 1 1 1 1 1 1 1 1 1 3 1 1 1 1 1 1 1 1 1 5 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 0 1 1 1 1 1 1 1 1 1 4 2
                  (local.get 0)))
              (return
                (i32.const 65)))
            (local.set 1
              (i32.const 80)))
          (return
            (local.get 1)))
        (return
          (i32.const 21)))
      (return
        (i32.const 79)))
    (i32.const 32))
  (export "memory" (memory 0))
  (export "getVal" (func 0))
  (export "main" (func 1))
)
