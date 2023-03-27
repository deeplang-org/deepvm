## 关于如何在Windows下使用DeepVM的一些记录

如果你拥有WSL，那么使用WSL来完成相应的操作自然是最佳的。但如果你并么有WSL时，在纯Windows环境下，你又该如何运行DeepVM呢？

这里假设在你的Windows下已经安装了C语言的编译器，如MinGW等，并且已经成功安装了CMake。

```shell

mkdir build
cd build
cmake -G "MinGW Makefiles" ..
make

```

问题会不会是出在`deep_mem.c: 130`呢？将一个64位的指针赋值给了32位长度的uint？