# MADLANG

**一门故意难读难写的编程语言。** 纯语法地狱，不是加密，但比加密更难读。

## v7.0.0（稳定版）

- **解释器**：C++ 单二进制，无依赖
- **生成器**：C++ 单二进制，无依赖
- **命令数**：101 条
- **回归测试**：8/8 全绿

## 快速开始

    ./madctl compile        # 编译解释器 + 生成器
    ./madctl do v7test.spec # 一键：生成 + 显示 + 运行
    ./v7test.sh             # 回归测试（8 项）

## 101 条命令

| 类 | 数量 | 例子 |
|----|------|------|
| 基础 | 30 | PRINT / SET / JUMP / ADD / CALL / IFEQ / RAND |
| 纸带 | 2 | TREAD / TWRITE（Brainfuck 风格）|
| 流程 | 2 | COMEFROM / PLEASE |
| 变量注册 | 8 | DIM / DECL / BIND / SALT / CHK / COMMIT / REFRESH / UNSET |
| 字符串 | 13 | STR_LEN / STR_AT / STR_SUB / STR_FIND / STR_UPPER / CHR / ORD |
| 列表 | 8 | LST_NEW / LST_PUSH / LST_POP / LST_GET / LST_SET / LST_LEN |
| 数学 | 13 | SIN / COS / SQRT / POW / LOG / ABS / FLOOR / MIN / MAX |
| 绘图 | 10 | GOTO / COLOR / DRAW_HLINE / DRAW_BOX / CLR_SCREEN |
| 文件 | 10 | F_OPEN / F_READ / F_WRITE / FILE_DEL / FILE_REN |
| 时间 | 4 | TIME_MS / TIME_NS / TIME_FMT / SLEEP_MS |

## 融合的语言

| 语言 | 借鉴 |
|------|------|
| Brainfuck | 纸带 + 指针 |
| Befunge | 2D 网格 + 方向 |
| Whitespace | 缩进深度（0-3）|
| Malbolge | 自修改（MUT=NO/OK）|
| INTERCAL | PLEASE / COMEFROM |
| Unlambda | `.$X` 组合子前缀 |
| Shakespeare | SCENE / WHO / WHERE |

## 命名变量系统

不用 `A`-`Z`，可定义任意名字，**每个变量 6 步注册**：

    DIM 1          # 占槽位
    DECL 1 T       # 类型 T/N/L
    BIND 1 score   # 绑名字（长度必须质数）
    SALT 1         # 加盐（槽位²）
    CHK 1 1D       # 校验和（ASCII 累加 + 槽位 mod 256）
    COMMIT 1       # 提交

用 100 次后过期，要 `REFRESH` 续期。

## 哈希链

每块尾部 `@H`（FNV-1a 32 位哈希），下块头部 `@P`。

**改一个字符 → 哈希不符 → 编译失败。**

## spec 语法

每行一条命令，`<命令> <参数1> <参数2>...`

转义：`~s`=空格 `~t`=冒号 `~c`=分号 `~d`=短横 `~u`=下划线 `~n`=换行 `%X`=字面量大写字母

参数规则：

- 单字符 `A`-`Z` → 变量名
- 纯数字 → 数值
- BF 路径（`<>^v+-@`）→ 纸带路径
- `$X` → 变量引用（双重 base64）

## 工具链

| 工具 | 语言 | 作用 |
|------|------|------|
| `madlangv7` | C++ | 解释器 |
| `madgen` | C++ | 生成器 |
| `madctl` | Bash | 一键脚本 |
| `v7test.sh` | Bash | 回归测试 |

**全部 C++，零 Python 依赖。**

## 示例（v7test.spec）

    SET A hello
    STR_UPPER B A
    PRINTLN $B
    SQRT C 16
    PRINTLN $C
    POW D 2 10
    PRINTLN $D
    HALT

输出：

    HELLO
    4
    1024

## 许可

MIT

---

**写得越烂，越见功力。**
