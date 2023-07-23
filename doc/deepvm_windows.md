## 关于如何在Windows下使用DeepVM的一些记录

如果你拥有WSL，那么使用WSL来完成相应的操作自然是最佳的。但如果你并么有WSL时，在纯Windows环境下，你又该如何运行DeepVM呢？

这里假设在你的Windows下已经安装了C语言的编译器，如MinGW等，并且已经成功安装了CMake。

```shell

mkdir build
cd build
cmake -G "MinGW Makefiles" ..
make

```

由于在Windows操作系统中生成的可执行程序会自带.exe后缀，因此，我们在测试时需求修改生成的可执行程序的路径以满足测试的需求。

你可以在test.py中添加如下代码：

```python

import platform

sys = platform.system()
BIN_PATH = '../bin/deepvm.exe' if sys == "Windows" else '../bin/deepvm'

```