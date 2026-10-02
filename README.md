# MADLANG

**一门故意难读难写的编程语言。** 纯语法地狱，不是加密，但比加密更难读。

## 当前版本：v7.0.0

- **解释器**：C++（单二进制，无依赖）
- **生成器**：C++（不再依赖 Python）
- **命令数**：101 条
- **语法层**：13 层地狱（哈希链 + 2D 纸带 + 6 语言融合）

## v7 特性

### 101 条命令，9 大类

| 类 | 数量 | 例子 |
|----|------|------|
| 基础 | 30 | PRINT / SET / JUMP / ADD / CALL |
| 纸带 | 2 | TREAD / TWRITE（Brainfuck 风格）|
| 流程 | 8 | COMEFROM / CMP / JE / JL / JNZ / NOP |
| 变量注册 | 8 | DIM / DECL / BIND / SALT / CHK / COMMIT / REFRESH / UNSET |
| 字符串 | 13 | STR_LEN / STR_AT / STR_SUB / STR_FIND / STR_REPL / CHR / ORD |
| 列表 | 8 | LST_NEW / LST_PUSH / LST_POP / LST_GET / LST_SET / LST_LEN |
| 数学 | 13 | SIN / COS / SQRT / POW / LOG / ABS / FLOOR / MIN / MAX |
| 绘图 | 10 | GOTO / COLOR / DRAW_HLINE / DRAW_BOX / CLR_SCREEN |
| 文件 | 10 | F_OPEN / F_READ / F_WRITE / DIR_LIST / FILE_DEL |
| 时间 | 4 | TIME_MS / TIME_NS / TIME_FMT / SLEEP_MS |

### 融合的语言

| 语言 | 借鉴 |
|------|------|
| Brainfuck | 纸带 + 指针（TREAD/TWRITE）|
| Befunge | 2D 格子 + 方向 |
| Whitespace | 缩进有语义（tab 深度 0-3）|
| Malbolge | 自修改代码（MUT=NO/OK）|
| INTERCAL | PLEASE 密度 + COMEFROM |
| Unlambda | 组合子前缀（.$X）|
| Shakespeare | 场景声明（SCENE/WHO/WHERE）|

### 命名变量系统

不用 `A`-`Z` 了。可以定义任意名字，但**每个新变量要 6 步注册**：

```

DIM 1
DECL 1 T
BIND 1 YzJOdmNtVT0=
SALT 1
CHK 1 1D
COMMIT 1

```

- `DIM` — 占槽位（必须连续）
- `DECL` — 声明类型 T/N/L
- `BIND` — 绑定名字（名字 base64 后长度必须质数）
- `SALT` — 加盐（槽位²）
- `CHK` — 校验和（名字 ASCII 累加 + 槽位，mod 256）
- `COMMIT` — 提交

**少一步 = 报错。** 用 100 次后过期，要 `REFRESH` 续期。

### 哈希链

每块尾部有 `@H`（本块 FNV-1a 32 位哈希），下一块头部有 `@P`（上一块的哈希）。

**改一个字符 → 哈希不符 → 编译失败。**

## 快速开始

    ./madctl compile        # 编译解释器 + 生成器
    ./madctl do v7test.spec # 一键：生成 + 显示 + 运行
    ./madctl run v7test.m6  # 直接运行

## 工具链

| 工具 | 语言 | 作用 |
|------|------|------|
| `madlangv7` | C++ | 解释器（101 命令）|
| `madgen` | C++ | 生成器（spec → .m6）|
| `madctl` | Bash | 一键脚本 |
| `madlangv7.cpp` | C++ | 解释器源码 |
| `madgen.cpp` | C++ | 生成器源码 |

**全部 C++。没有任何 Python 依赖。**

## spec 语法

每行一条命令，`<命令> <参数1> <参数2>...`

转义：`~s`=空格 `~t`=冒号 `~c`=分号 `~d`=短横 `~u`=下划线 `~n`=换行

参数规则：

- 单字符 `A`-`Z` → 变量名
- 纯数字 → 数值
- BF 路径（`<>^v+-@`）→ 纸带路径（v7 专用）
- `$X` → 变量引用（双重 base64）

## 编译链路

    spec.txt ──madgen──▶ .m6 ──madlangv7──▶ 输出

**没有任何加密。** 你能一行一行读，但读一行的前提是先把前面所有块读懂。

## 依赖

- g++ / clang++（C++11）
- 就这一个

## 许可

MIT

---

**写得越烂，越见功力。**
