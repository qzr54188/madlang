# MADLANG

**一门故意难读难写的编程语言。** 不是玩具，是完整工具链：解释器、编译器、REPL、Web IDE、打包器。

## 设计哲学

> 高级语言要忍受"繁琐 + 转义地狱"
> 低级语言要忍受"转义地狱"
> MADLANG 要忍受"繁琐 + 转义 + 混淆 + 逻辑错乱"

一个"能跑但写起来极其痛苦"的语言实验。它有 30 条命令、10 个语法模式、16 种分隔格式、独立的编译器和图形 IDE —— 但**写一行要算三遍**。

## 特性

- **双重 base64** —— 所有文本参数编码两次
- **混淆 token** —— 命令名是从 `Il1O0S5Z2B8Qq` 生成的 6 位随机串
- **乱序逻辑行号** —— 执行按行号升序，物理顺序无关
- **单字母变量** —— 只有 A-Z，26 个
- **禁空格** —— 源码里一个空格都不能有
- **v5 语法层地狱** —— 声明块 + 16 schema + 10 模式 + 模式链 + sigil 公式 + 平方行号

## 两个版本

| 版本 | 定位 | 难度 |
|------|------|------|
| v4 | 词法混淆（能读、难写）| ★★★ |
| v5 | 语法地狱（读都读不懂）| ★★★★★ |

## 工具链

| 工具 | 作用 |
|------|------|
| `madlang` | v4 解释器 |
| `madlangc` | v4 编译器（.mgl → C++ → 原生）|
| `madrepl` | v4 交互式 IDE |
| `madweb.py` | v4 Web Studio |
| `madlangv5` | v5 解释器 |
| `mad5gen.py` | v5 源码生成器（从简单 spec 生成 v5 文件）|
| `madctl` | 一键脚本 |

## 快速开始

### 编译

    cd madproj
    ./madctl compile         # 编译 v4 解释器
    ./madctl ccompile        # 编译 v4 编译器
    ./madctl rcompile        # 编译 REPL
    g++ -std=c++11 -O2 -pthread -o madlangv5 madlangv5.cpp   # 编译 v5

### 运行

    ./madctl run examples/hello.mgl        # 解释运行
    ./madctl cc examples/hello.mgl         # 编译成原生二进制
    ./madctl build examples/hello.mgl      # 打包成自解压 .bundle
    ./madctl repl                          # 交互式 IDE
    ./madctl web                           # 浏览器 Studio

### v5

    ./madlangv5 --map                      # 查看 10 模式 op 表
    python3 mad5gen.py spec.txt out.m5     # 生成 v5 源码
    ./madlangv5 out.m5                     # 运行

## v4 语法

    <逻辑行号>|<token>|<base64 双重编码的参数>

示例（打印 Hello World）：

    1|QQqI52|SGVsbG8gV29ybGQ=
    2|qQO50S

## v5 语法

每块 8 行声明 + 1 行正文 + 1 行分隔符：

    @SCHEMA=0
    @MEM=0010-0024
    @STACK=2
    @TMP=1
    @REG=N/N
    @BODY
    %1{0}>*|Hello~sWorld
    @END
    ---:---

其中：
- `@SCHEMA` 必须 = 地址末位十六进制
- `@MEM` 长度必须 = 正文实际字符数
- `@STACK` 必须是质数
- `@TMP` 必须是 2 的幂
- 正文的 sigil 由 `(平方行号 + 模式) mod 4` 决定
- 模式链由上块 `(M×7 + sigil值 + 参数个数) mod 10` 决定

**改一个字符 → 长度变 → 地址变 → schema 变 → 全部重写。**

## 30 条命令

PRINT / PRINTLN / SET / INPUT / JUMP / IFEQ / CLEAR / SLEEP / HALT
RAND / ADD / SUB / MUL / DIV / MOD / TIME
MOV / CONCAT / LEN / GT / LT / NEQ / PUSH / POP
CALL / RET / READFILE / WRITEFILE / ENV / ARGV

## 编译链路

    foo.mgl ──madlang──▶ 解释执行
            │
            └─madlangc─▶ foo.cpp ──g++──▶ foo（原生 ELF）

    madctl build ──▶ foo.bundle（自解压 bash）

三种运行方式，输出一致。

## 依赖

- g++ / clang++（C++11）
- bash
- python3（可选，用于 Web Studio 和 v5 生成器）

## 许可

MIT
