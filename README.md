# MADLANG

**一门故意难读难写的编程语言。** 纯语法地狱，不是加密，但比加密更难读。

## 设计哲学

> 高级语言要忍受"繁琐 + 转义地狱"
> 低级语言要忍受"转义地狱"
> MADLANG 是"繁琐 + 转义 + 混淆 + 逻辑错乱 + 连锁雪崩"

## 当前版本：v5.0.0

v5 是**语法层**地狱。同一个字面值在不同位置含义完全不同，一条命令要读它必须**先模拟前面所有命令**。

### 十层叠加

| 层 | 内容 |
|----|------|
| L1 | 参数双重 base64 |
| L2 | op 符号从 `Il1O0S5Z2B8Qq` 生成（`I`/`l`/`1` 分不清）|
| L3 | 行号用**平方数**（逻辑行 5 → 25）|
| L4 | sigil 公式：`(平方行号 + 模式) mod 4` |
| L5 | 10 模式 × 30 命令 = 300 条 op 映射表 |
| L6 | 模式链：下一块模式 = `(M×7 + sigil值 + 参数个数) mod 10` |
| L7 | 16 种 schema（地址末位十六进制决定分隔格式）|
| L8 | 声明块 6 行（SCHEMA/MEM/STACK/TMP/REG）|
| L9 | `@STACK` 必须质数，`@TMP` 必须 2 的幂 |
| L10 | `@MEM` 长度必须精确匹配正文，改一个字符全部重写 |

**加上声明块关键字混淆**：`@SCHEMA` 显示为 `2Q05q`，`@MEM` 显示为 `IS8Oq`。

## Hello World 对照

**你写的**（spec，人类可读）：

    PRINTLN Hello~sWorld
    HALT

**生成的**（.m5，地狱）：

    2Q05q=0
    IS8Oq=0010-0031
    q0SqI=2
    8I18S=1
    BB00O=N/N
    OOQS2
    %1{0}258|U0dWc2JHOGdWMjl5YkdRPQ==
    ISq21
    SqlBq55

    2Q05q=2
    IS8Oq=0032-003C
    q0SqI=2
    8I18S=1
    BB00O=N/N
    OOQS2
    *4{3}Zqq{}
    ISq21
    SqlBq55

**Python 对照**：`print("Hello World")`

## 快速开始

### 编译

    cd madproj
    ./madctl compile

### 生成并运行

    ./madctl gen hello5.spec          # spec → .m5
    ./madctl run hello5.m5            # 运行
    ./madctl do hello5.spec           # 一键：生成+显示+运行

### 查看内部表

    ./madctl map                      # 10 模式 op 表
    ./madctl kw                       # 混淆关键字

## 工具链

| 工具 | 作用 |
|------|------|
| `madlangv5` | v5 解释器（单二进制，无依赖）|
| `mad5gen.py` | spec → .m5 生成器 |
| `madctl` | 一键脚本 |

## spec 语法

每行一条命令，`<命令> <参数1> <参数2>...`

支持的转义：`~s`=空格 `~t`=冒号 `~c`=分号 `~d`=短横 `~u`=下划线 `~n`=换行

命令列表：PRINT / PRINTLN / SET / INPUT / JUMP / IFEQ / CLEAR / SLEEP / HALT
RAND / ADD / SUB / MUL / DIV / MOD / TIME
MOV / CONCAT / LEN / GT / LT / NEQ / PUSH / POP
CALL / RET / READFILE / WRITEFILE / ENV / ARGV

## 编译链路

    spec.txt ──mad5gen.py──▶ .m5 ──madlangv5──▶ 输出

**没有任何加密**。你能一行一行读，但读一行的前提是先把前面所有块读懂。

## 依赖

- g++ / clang++（C++11）
- python3（生成器）

## 许可

MIT

---

**写得越烂，越见功力。**
