;; 组合用例：data 段（多段）+ 内置函数 puts/puti + call + drop + i32 乘法
;; 输出 "deepvm: 42\n0"
(module
  (import "env" "puts" (func $puts (param i32) (result i32)))
  (import "env" "puti" (func $puti (param i32) (result i32)))
  (memory 1)
  (data (i32.const 0) "deepvm: ")
  (data (i32.const 8) "\n")
  (func (export "main") (result i32)
    (drop (call $puts (i32.const 0)))                 ;; "deepvm: "
    (drop (call $puti (i32.mul (i32.const 6) (i32.const 7)))) ;; "42"
    (drop (call $puts (i32.const 8)))                 ;; "\n"
    (i32.const 0)))
