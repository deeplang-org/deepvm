;; 组合用例：start 段 + global + 内存初始化 + main 读回累加
;; start 把 base=10、mem=[1,2,3,4]；main 求 1+2+3+4+10 = 20
(module
  (memory 1)
  (global $base (mut i32) (i32.const 0))
  (func $init
    i32.const 10
    global.set $base
    i32.const 0 i32.const 1 i32.store
    i32.const 4 i32.const 2 i32.store
    i32.const 8 i32.const 3 i32.store
    i32.const 12 i32.const 4 i32.store)
  (start $init)
  (func (export "main") (result i32)
    i32.const 0 i32.load
    i32.const 4 i32.load
    i32.add
    i32.const 8 i32.load
    i32.add
    i32.const 12 i32.load
    i32.add
    global.get $base
    i32.add))
