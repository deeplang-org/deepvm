;; 内置函数 puts + data 段字符串
(module
  (import "env" "puts" (func $puts (param i32) (result i32)))
  (memory 1)
  (data (i32.const 0) "hello deeplang\n")
  (func (export "main") (result i32)
    (call $puts (i32.const 0))))
