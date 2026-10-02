# MADLANG

**一门故意难读难写的编程语言。** 纯语法地狱，不是加密，但比加密更难读。

## 版本

| 版本 | 定位 | 难度 |
|------|------|------|
| v5 | 10 层语法地狱 | ★★★★ |
| **v6** | **13 层 + 6 语言融合** | ★★★★★ |

## v6：集大成者

融合 **Brainfuck / Befunge / Whitespace / Malbolge / INTERCAL / Unlambda / Shakespeare**。

### 6 语言借鉴

| 语言 | 借鉴 | 实现 |
|------|------|------|
| Brainfuck | 纸带 + 指针 | `TREAD` / `TWRITE` + `>>v+` 路径 |
| Befunge | 2D + 方向 | 纸带 16×16，路径有 `<>^v` |
| Whitespace | 缩进有语义 | 每块前导 tab 数 = 深度（0-3）|
| Malbolge | 自修改 | `MUT=NO/OK` 声明 |
| INTERCAL | PLEASE + COME FROM | 每 4 块至少 1 个 PLEASE |
| Unlambda | 组合子前缀 | `.$X` 前缀访问变量 |
| Shakespeare | 场景 + 人物 | `SCENE:I` / `WHO:main` / `WHERE:0,0` |

### 每块 14 行结构

```

0OZSZ:I              ← SCENE
Bl0IS:main           ← WHO
I5SI2:0,0            ← WHERE
01Bq5=NO             ← MUT
2Q05q=?              ← SCHEMA
IS8Oq=0010-0036      ← MEM
q0SqI=2              ← STACK
8I18S=1              ← TMP
BB00O=N/N            ← REG
OOQS2=0858F950       ← @H 本块哈希（v6 新增）
ISq21                ← BODY
%1{0}l0|00A|0AU0...  ← 正文
SqlBq                ← END
55BBl=00000000       ← @P 上一块哈希（v6 新增）
QQ1IBq2              ← SEP

```

**哈希链**：`@H` 是本块 FNV-1a 32 位哈希，`@P` 是上一块哈希。改一个字符 → 哈希不符 → 编译失败。

## 快速开始

    # 编译 v5
    g++ -std=c++11 -O2 -pthread -o madlangv5 madlangv5.cpp
    # 编译 v6
    g++ -std=c++11 -O2 -pthread -o madlangv6 madlangv6.cpp
    # 编译 TUI
    g++ -std=c++11 -O2 -o mad6 mad6.cpp

    # v5 用法
    python3 mad5gen.py hello5.spec hello5.m5
    ./madlangv5 hello5.m5

    # v6 用法
    python3 mad6gen.py tape.spec tape.m6
    ./madlangv6 tape.m6

## 示例

| 文件 | 说明 | 版本 |
|------|------|------|
| `hello5.spec` | Hello World | v5 |
| `pingpong.spec` | 乒乓动画 | v5 |
| `hitmouse.spec` | 打地鼠 | v5 |
| `tape.spec` | 纸带读写测试 | v6 |

## spec 语法

每行一条命令，`<命令> <参数1> <参数2>...`

转义：`~s`=空格 `~t`=冒号 `~c`=分号 `~d`=短横 `~u`=下划线 `~n`=换行

参数规则：

- 单字符 `A`-`Z` → 变量名（不编码）
- 纯数字 → 数值参数（不编码）
- BF 路径（`<>^v+-`）→ 原样（v6）
- `$X` → 变量引用（双重 base64）

命令（35 条）：PRINT / PRINTLN / SET / INPUT / JUMP / IFEQ / CLEAR / SLEEP / HALT
RAND / ADD / SUB / MUL / DIV / MOD / TIME
MOV / CONCAT / LEN / GT / LT / NEQ / PUSH / POP
CALL / RET / READFILE / WRITEFILE / ENV / ARGV
TREAD / TWRITE / COMEFROM / PLEASE

## 工具链

| 工具 | 作用 |
|------|------|
| `madlangv5` | v5 解释器 |
| `madlangv6` | v6 解释器 |
| `mad5gen.py` | v5 生成器（spec → .m5）|
| `mad6gen.py` | v6 生成器（spec → .m6）|
| `madctl` | 一键脚本 |
| `mad6.cpp` | TUI 启动器（简单/困难/终端/工具）|

## Hello World 对照

**spec（人类可读）**：
```

PRINTLN Hello~sWorld
HALT

```

**v5 生成（.m5，20 行）**：10 层地狱

**v6 生成（.m6，127 行）**：13 层地狱 + 哈希链 + 场景声明

**Python**：`print("Hello World")`

## 依赖

- g++ / clang++（C++11）
- python3（生成器）

## 许可

MIT

---

**写得越烂，越见功力。**
