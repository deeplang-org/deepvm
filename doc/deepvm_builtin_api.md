## DeepVM Builtin API Proposal

### About

Generally, builtin API is a function with the body is built  in language VM and user can call function through reading builtin API SPEC or language demos.

Deeplang builtin APIs  are  designed for system io, IoT hardware interface, **ffi** and so on. 

deeplang builtin functions: 

```
fun puts(str: String);
fun puti(data: i32);
fun putf(data: f32);
```

IoT hardware devices: *uart, gpio, i2c, spi, pwm, timer, i2s, adc, dac*...
Some builtin API as follow:

```deeplang
fun uartOpen(port: i32, baud: i32) -> i32;
fun uartRead(port: i32, data: [Char; i32]) -> i32;
fun uartWrite(port: i32, data: [char; i32]) -> i32;
fun uartClose(port: i32);
fun gpioOpen(port: i32, state: i32, mode: i32) -> i32;
fun gpioRead(port: i32);
fun gpioHandler(port: i32, handle: () -> unit);
fun gpioClose(port: i32);
...
```

### How to call a builtin API

*print* as example.

deeplang:

```
fun main() -> i32 {
	let count: i32 = 100;
    print("hello deeplang ${count}");
    return 0;
}
```

deeplang desugar with builtin APIs :

```
fun main() -> i32 {
	let count: i32 = 100;
    puts("hello deeplang ");
    puti(count);
    return 0;
}
```

deeplang codegen wasm file (s-expression).

```wasm
(module
 (type $FUNCSIG$ii (func (param i32) (result i32)))
 (import "env" "puti" (func $puti (param i32) (result i32)))
 (import "env" "puts" (func $puts (param i32) (result i32)))
 (table 0 anyfunc)
 (memory $0 1)
 (data (i32.const 16) "hello deeplang \00")
 (export "memory" (memory $0))
 (export "main" (func $main))
 (func $main (; 2 ;) (result i32)
  (drop
   (call $puts
    (i32.const 16)
   )
  )
  (drop
   (call $puti
    (i32.const 100)
   )
  )
  (i32.const 0)
 )
)

builtin 
import map native function table

```

### What to do

1. import section loading in DeepVM loader. lichang
2. mapping deeplang import function with c functions in DeepVM interp. chen yitang

```c
typedef struct {
    const char *funcName;
    const char *funcProto;
    void *(func)(void);
}

const funcMap_t nativeFuncMapping[] = 
[
	{"puts","v(s)",native_puts},
	{"puti", "v(i)", native_puti},
	{"uartOpen","i(i,i)", native_uartOpen}
]
```



