;; start 段：先执行初始化函数
(module
  (global $g (mut i32) (i32.const 0))
  (func $init
    i32.const 5
    global.set $g)
  (start $init)
  (func (export "main") (result i32)
    global.get $g))
