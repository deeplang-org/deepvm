;; 全局变量读取
(module
  (global $g (mut i32) (i32.const 10))
  (func (export "main") (result i32)
    global.get $g
    i32.const 32
    i32.add))
