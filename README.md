# MADLANG

**一门故意难读难写的编程语言。** 纯语法地狱，不是加密，但比加密更难读。

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

**声明块关键字也混淆**：`@SCHEMA` → `2Q05q`，`@MEM` → `IS8Oq`

## 快速开始

    ./madctl compile        # 编译解释器
    ./mad6                  # 启动 TUI（简单/困难/终端/工具）
    ./madctl do hello5.spec # 一键：生成+显示+运行

## 示例

| 文件 | 说明 |
|------|------|
| `hello5.spec` | Hello World |
| `pingpong.spec` | 乒乓动画（清屏 + 睡 + 循环）|
| `hitmouse.spec` | 打地鼠（随机 + 输入 + 判断）|

跑法：

    ./madctl do pingpong.spec
    ./madctl do hitmouse.spec

## Hello World 对照

**spec（人类可读）**：
```

PRINTLN Hello~sWorld
HALT

```

**生成的 .m5（地狱）**：
```

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

```

**Python 对照**：`print("Hello World")`

## 工具链

| 工具 | 作用 |
|------|------|
| `madlangv5` | 解释器（单二进制，无依赖）|
| `mad5gen.py` | spec → .m5 生成器 |
| `madctl` | 一键脚本 |
| `mad6` | TUI 启动器（简单/困难/终端/工具）|

## spec 语法

每行一条命令，`<命令> <参数1> <参数2>...`

转义：`~s`=空格 `~t`=冒号 `~c`=分号 `~d`=短横 `~u`=下划线 `~n`=换行

参数规则：

- 单字符 `A`-`Z` → 变量名（不编码）
- 纯数字 → 数值参数（不编码）
- `#$X` → 变量引用（双重 base64）
- 其他 → 文本（双重 base64）

命令：PRINT / PRINTLN / SET / INPUT / JUMP / IFEQ / CLEAR / SLEEP / HALT
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
