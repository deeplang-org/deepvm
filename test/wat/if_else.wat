;; 控制流：if / then / else
(module
  (func (export "main") (result i32)
    (if (result i32) (i32.const 1)
      (then (i32.const 100))
      (else (i32.const 200)))))
