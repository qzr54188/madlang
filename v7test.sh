#!/bin/bash
cd /data/data/com.termux/files/home/madproj
PASS=0
FAIL=0

# ============ 批 1: 基础命令 ============
cat > _t1.spec <<'S1'
SET A hello
SET B 100
SET C 3
PRINTLN $A
PRINTLN $B
ADD B 5
PRINTLN $B
SUB B 5
PRINTLN $B
MUL C 4
PRINTLN $C
DIV C 2
PRINTLN $C
MOD C 3
PRINTLN $C
MOV D A
PRINTLN $D
CONCAT D B
PRINTLN $D
LEN E D
PRINTLN $E
GT B C 24
PRINTLN never
JUMP 26
PRINTLN reached
NEQ A x 28
PRINTLN never2
JUMP 30
PRINTLN reached2
PUSH A
POP F
PRINTLN $F
CALL 35
HALT
PRINTLN sub_ok
RET
S1
cat > _t1.expect <<'E1'
hello
100
105
100
12
6
0
hello
hello100
8
hello
sub_ok
E1

# ============ 批 2: 字符串 ============
cat > _t2.spec <<'S2'
SET A Hello~sWorld
STR_LEN B A
PRINTLN $B
STR_UPPER C A
PRINTLN $C
STR_LOWER D C
PRINTLN $D
STR_AT E A 0
PRINTLN $E
STR_SUB F A 6 5
PRINTLN $F
STR_FIND G A o
PRINTLN $G
STR_STARTS H A Hel
PRINTLN $H
STR_ENDS I A rld
PRINTLN $I
STR_TRIM J A
PRINTLN $J
STR_REPL K A l %L
PRINTLN $K
CHR L 65
PRINTLN $L
ORD M A
PRINTLN $M
HALT
S2
cat > _t2.expect <<'E2'
11
HELLO WORLD
hello world
H
World
4
1
1
Hello World
HeLLo WorLd
A
72
E2

# ============ 批 3: 列表 ============
cat > _t3.spec <<'S3'
LST_NEW L
SET X apple
LST_PUSH L X
SET X banana
LST_PUSH L X
SET X cherry
LST_PUSH L X
LST_LEN N L
PRINTLN $N
LST_GET A L 0
PRINTLN $A
LST_GET A L 2
PRINTLN $A
LST_POP B L
PRINTLN $B
LST_LEN N L
PRINTLN $N
LST_INS L 1 X
LST_LEN N L
PRINTLN $N
LST_GET A L 1
PRINTLN $A
LST_DEL L 0
LST_LEN N L
PRINTLN $N
HALT
S3
cat > _t3.expect <<'E3'
3
apple
cherry
cherry
2
3
cherry
2
E3

# ============ 批 4: 数学 ============
cat > _t4.spec <<'S4'
SQRT A 16
PRINTLN $A
POW B 2 10
PRINTLN $B
ABS C -5
PRINTLN $C
FLOOR D 3.7
PRINTLN $D
CEIL E 3.2
PRINTLN $E
ROUND F 3.5
PRINTLN $F
MIN G 3 7
PRINTLN $G
MAX H 3 7
PRINTLN $H
SIN I 0
PRINTLN $I
COS J 0
PRINTLN $J
LOG K 1
PRINTLN $K
EXP L 0
PRINTLN $L
HALT
S4
cat > _t4.expect <<'E4'
4
1024
5
3
4
4
3
7
0
1
0
1
E4

# ============ 批 5: 命名变量 ============
cat > _t5.spec <<'S5'
DIM 1
DECL 1 T
BIND 1 score
SALT 1
CHK 1 1D
COMMIT 1
SET :score 42
PRINTLN :score
ADD :score 8
PRINTLN :score
DIM 2
DECL 2 N
BIND 2 title
SALT 2
CHK 2 24
COMMIT 2
SET :title Alice
PRINTLN :title
HALT
S5
cat > _t5.expect <<'E5'
42
50
Alice
E5

run_batch() {
  local name="$1"
  local m6="_${name}.m6"
  ./madgen v6 "_${name}.spec" "$m6" > /dev/null 2>&1
  if [ ! -f "$m6" ]; then
    echo "  ❌ 批 $name  生成失败"
    FAIL=$((FAIL+1))
    return
  fi
  ./madlangv7 "$m6" < /dev/null > "_${name}.out" 2>&1
  if diff -q "_${name}.expect" "_${name}.out" > /dev/null; then
    echo "  ✅ 批 $name  ($(wc -l < "_${name}.out") 行)"
    PASS=$((PASS+1))
  else
    echo "  ❌ 批 $name"
    diff "_${name}.expect" "_${name}.out" | head -20 | sed 's/^/     /'
    FAIL=$((FAIL+1))
  fi
}

echo ""
echo "═══════════════════════════════════════"
echo "  MADLANG v7 回归测试"
echo "═══════════════════════════════════════"
echo ""
echo "[1/6] 基础命令"
run_batch t1
echo "[2/6] 字符串"
run_batch t2
echo "[3/6] 列表"
run_batch t3
echo "[4/6] 数学"
run_batch t4
echo "[5/6] 命名变量"
run_batch t5

echo "[6/6] 错误处理"
printf 'SET A 5\nSET A\nHALT\n' > _t6a.spec
./madgen v6 _t6a.spec _t6a.m6 > /dev/null 2>&1
O6A=$(./madlangv7 _t6a.m6 < /dev/null 2>&1)
if echo "$O6A" | grep -q "SET"; then
  echo "  ✅ 6.1  缺参数报错"
  PASS=$((PASS+1))
else
  echo "  ❌ 6.1  期望报错，实际: $O6A"
  FAIL=$((FAIL+1))
fi

printf 'LST_GET A L 0\nHALT\n' > _t6b.spec
./madgen v6 _t6b.spec _t6b.m6 > /dev/null 2>&1
O6B=$(./madlangv7 _t6b.m6 < /dev/null 2>&1)
if echo "$O6B" | grep -q "oob"; then
  echo "  ✅ 6.2  越界报错"
  PASS=$((PASS+1))
else
  echo "  ❌ 6.2  期望 oob，实际: $O6B"
  FAIL=$((FAIL+1))
fi

sed 's/%1/%2/' _t1.m6 > _t6c.m6
O6C=$(./madlangv7 _t6c.m6 < /dev/null 2>&1)
if echo "$O6C" | grep -q "哈希不符"; then
  echo "  ✅ 6.3  哈希校验"
  PASS=$((PASS+1))
else
  echo "  ❌ 6.3  期望哈希不符，实际: $O6C"
  FAIL=$((FAIL+1))
fi

echo ""
echo "═══════════════════════════════════════"
echo "  通过: $PASS / 8"
echo "  失败: $FAIL / 8"
echo "═══════════════════════════════════════"
echo ""

rm -f _t*.spec _t*.m6 _t*.expect _t*.out
