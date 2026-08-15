### 主要结构体

---

```
DEEPStack(操作数栈)
DEEPInterpFrame(block帧，用于存储函数和block帧的帧)
DEEPExecEnv(运行环境)
DEEPModule(从loader得到的module)
```

### 主要函数

---

```
exec_instructions(执行代码块内指令)
call_main(为main函数创建帧，执行main函数)
```

### 下一步计划

---

1. 与loader合作完成更多段的解析，实现更复杂函数的解释

2. 丰富结构体内容，使其支持更多的指令集操作

3. 在现有基础上实现部分异常检查

### 工具网址

---

1. [在线c2wat](https://mbebenita.github.io/WasmExplorer/)

2. [在线wasm2wat](https://webassembly.github.io/wabt/demo/wasm2wat/)

   
##### Example中为实现的简单计算实例，example.cpp为源c语言程序，经由c2wat可得到example.wasm
