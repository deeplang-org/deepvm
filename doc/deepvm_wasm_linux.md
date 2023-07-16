webassembly转换工具

安装

```
sudo apt install wabt

```

将二进制的wasm文件转换为可读的wat文件

```
wasm2wat input.wasm -o output.wat
```

将wat文件转换回wasm文件

```
wat2wasm input.wat -o output.wasm
```