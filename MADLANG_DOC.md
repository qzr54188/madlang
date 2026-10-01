# MADLANG doc v4.0

## 混淆清单（主 token）

| op | token | args |
|----|-------|------|
| PRINT     | 8S0Q2Z | <b64x2> |
| PRINTLN   | QQqI52 | <b64x2> |
| SET       | SlQqB1 | <var>|<b64x2> |
| INPUT     | 1SOIZB | <var> |
| JUMP      | ZIZ012 | <line> |
| IFEQ      | 5OS08Q | <var>|<b64x2>|<line> |
| CLEAR     | q1SOSB | - |
| SLEEP     | O0Il51 | <ms> |
| HALT      | qQO50S | - |
| RAND      | l1lO21 | <var>|<max> |
| ADD       | 0I820B | <var>|<int> |
| SUB       | QqQO2I | <var>|<int> |
| READFILE  | 0BO1OB | <var>|<b64x2路径> |
| TIME      | 22202Z | <var> |
| MOV       | 2Zl255 | <var>|<var> |
| CONCAT    | S0q0S8 | <var>|<var> |
| LEN       | 1q0IIq | <var>|<var> |
| MUL       | SOB5l5 | <var>|<int> |
| DIV       | Z1QBll | <var>|<int> |
| MOD       | Z2I155 | <var>|<int> |
| GT        | IlZlSl | <var>|<var>|<line> |
| LT        | QI8Q5l | <var>|<var>|<line> |
| NEQ       | 0Q85ZI | <var>|<b64x2>|<line> |
| PUSH      | 522qO2 | <var> |
| POP       | 1SSQl1 | <var> |
| CALL      | 2IB1l1 | <line> |
| RET       | IQSS2S | - |
| WRITEFILE | 5Q10lB | <varPath>|<varContent> |
| ENV       | B22ZSO | <var>|<b64x2name> |
| ARGV      | 1OIS88 | <var>|<idx> |

## 源码格式

<lineNo>|<token>|<arg1>|<arg2>|...
行首 # 为注释，整行忽略。

## 规则

1. 源码不允许出现空格
2. 参数值双重 base64：b64(b64(text))
3. 行号 = 逻辑行号；执行按行号升序，与物理顺序无关
4. 变量名为单个大写字母 A-Z
5. 参数解码两次得 $X 时取变量 X 的值
6. JUMP/IFEQ/GT/LT/NEQ/CALL 目标为逻辑行号

## 运行

madlang <program.mgl> [args...]
madlang --serve [port]
madlang --mapping
madlang --regen
