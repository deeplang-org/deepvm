### block帧
<br />

目前有函数帧（`DEEPInterpFrame`），计划将其改造成函数和结构体通用的帧结构，并引入控制栈（`DEEPControlStack`）。方案如下：
1. `DEEPInterpFrame`结构新增一个`int`（或者`enum`）属性`type`，用于记录这是函数，`block`指令，`if`指令（区分`then`和`else`）还是`loop`指令。
2. 当解释器读取到`block`指令时，判断结构体的类型、本地变量及其数目，以及结构体开始的位置和大小（写一个额外函数`read_block(uint8_t *ip, uint8_t **start, uint32_t *offset, bool search_for_else)`，这个函数接收当前的指令地址，然后顺序读取找`end`指令，并将`start`和`offset`赋予正确的开始和结束的值。`search_for_else`为真代表在寻找一个`if`指令的`else`分支，此时将`start`设置为`else`指令的位置）。这样一来`block`结构也可以如同函数一般存储到`DEEPFunction`中。`if`指令详情见6。
3. 退出函数/block的时候，如果操作数栈顶上有额外的值，需要pop掉。比如说，一个接收2个参数返回一个值的函数，假设调用函数时栈上有3个值，那么函数执行完成后栈上应该有2个值，因为被pop了2个值（参数）并push了一个值（返回值）。如果函数提前结束，并且栈顶还是有3个值，那么最顶上的一个应被视为结果，然后再下面多余的一个应该被删除（按照张老师的书7.2.3所示）。这部分内容目前似乎还有待实现。
4. 引入控制栈（`DEEPControlStack`），是一个预设大小（深度）的`DEEPInterpFrame`数组，同时也记录当前帧的位置（index）。
5. 控制指令（`br_table`指令除外）通过控制栈可以快速找到跳到的位置（如果是`loop`指令，跳回到开始；否则结尾（开始+长度））。
6. 读取到`if`指令时，记录好类型信息之后，首先判断条件，根据条件选择`then`分支或是`else`分支。这两个分支的结束位置都是`end`指令。为了防止`else`分支在条件为真时被无端执行，当`DEEPInterpFrame`的`type`属性为`then`时，若读取到`else`指令，直接跳到`end`指令。
7. `br_table`指令，再想想，应该也不会太复杂。我想先实现以上内容。

## 开发中笔记
1. 发现一个问题（#19）。总之暂时先切回了C原生内存管理。
2. 额外添加了drop和select指令代码，也就是参数指令。实现了select。
3. 准备实现一些目前没有的比较指令，因为在test中存在。已经实现了部分。
4. 目前`read_block`函数已经可以正确算出block开始和截止的位置。其他结构体暂未实现。
5. 解决了`exec_instruction`函数中`ip_end`比function的实际结尾（end指令）要大的bug。在`DEEPFunction`中存储function的body的大小，而不是整个函数的大小。这样`ip_end = ip + code_size`可以正确指向函数的结尾。
6. 目前可以识别和执行block指令，并且正确的操作控制栈（创建正确的block帧，推入/推出栈，释放帧，没有内存泄漏），同时会返回执行完成后的指令地址。
7. 实现的br和br_if指令。跳出多层也已实现，但是没有测试。
8. tri_if_001.wasm和tri_if_002.wasm通过。
9. loop指令已经实现，但是没有测试（测试样例中的loop_001不包含loop）。
10. return指令已经实现并通过测试。
