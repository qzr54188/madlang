#!/usr/bin/env python3
# mad5gen.py - 从简单描述生成 v5 源文件
# 用法: mad5gen.py <spec.txt> <out.m5>
import sys, os, subprocess

def load_ops(bin_path):
    out = subprocess.check_output([bin_path, '--map']).decode('utf-8', errors='replace')
    table = {}
    for line in out.splitlines():
        if not line.strip(): continue
        parts = line.split()
        if len(parts) < 2: continue
        cmd = parts[0]
        for p in parts[1:]:
            if len(p) < 4 or p[0] != '[' or p[2] != ']':
                continue
            try:
                mode = int(p[1])
            except: continue
            op = p[3:]
            table[(mode, cmd)] = op
    return table

SIGS = "@%&*"
SCHEMAS = [
    (None, None, None, None, '|'), (None, None, '[', ']', None),
    (None, None, '{', '}', None), ('(', ')', None, None, ' '),
    (None, None, '(', ')', ','), ('{', '}', None, None, ':'),
    (None, None, None, None, '~'), (None, None, None, None, '^'),
    ('[', ']', '<', '>', None), (None, None, None, None, '%'),
    (None, None, None, None, '#'), (None, None, None, None, '@'),
    (None, None, None, None, '$'), (None, None, None, None, '&'),
    (None, None, None, None, '*'), (None, None, None, None, '?'),
]

def build_body_tail(schema, op, args):
    oo, oc, ao, ac, sep = SCHEMAS[schema]
    if ao and ac:
        # 每个 arg 用一对括号包裹；0 个 arg 输出 op()
        if args:
            r = op + "".join(ao + a + ac for a in args)
        else:
            r = op + ao + ac
    else:
        r = (sep or '').join([op] + list(args))
    if oo and oc:
        r = oo + r + oc
    return r

def main():
    if len(sys.argv) < 3:
        print("用法: mad5gen.py <spec.txt> <out.m5>")
        sys.exit(1)
    spec_file = sys.argv[1]
    out_file = sys.argv[2]
    script_dir = os.path.dirname(os.path.abspath(__file__))
    bin5 = os.path.join(script_dir, "madlangv5")
    if not os.path.exists(bin5):
        print("找不到 " + bin5, file=sys.stderr); sys.exit(1)
    OP = load_ops(bin5)

    lines = []
    with open(spec_file) as f:
        for ln in f:
            ln = ln.strip()
            if not ln or ln.startswith('#'): continue
            lines.append(ln)

    output = []
    addr = 0x10
    last_mode = None
    last_sig_val = 0
    last_argc = 0
    for i, line in enumerate(lines):
        parts = line.split(' ')
        cmd = parts[0]
        args = parts[1:]
        if last_mode is None:
            mode = 0
        else:
            mode = (last_mode * 7 + last_sig_val + last_argc) % 10
        line_num = i + 1
        sq = line_num * line_num
        sig_idx = (sq + mode) % 4
        sig_char = SIGS[sig_idx]
        sig_val = sig_idx + 1
        op_sym = OP.get((mode, cmd))
        if op_sym is None:
            print("未知命令: " + cmd + " (mode " + str(mode) + ")", file=sys.stderr)
            sys.exit(1)
        if mode >= 5:
            op_sym = op_sym[::-1]
        schema = addr & 0xF
        body_tail = build_body_tail(schema, op_sym, args)
        pre = sig_char + str(sq) + "{" + str(mode) + "}"
        body = pre + body_tail
        blen = len(body)
        addr_end = addr + blen
        output.append("@SCHEMA=" + format(schema, 'X'))
        output.append("@MEM=" + format(addr, '04X') + "-" + format(addr_end, '04X'))
        output.append("@STACK=2")
        output.append("@TMP=1")
        output.append("@REG=N/N")
        output.append("@BODY")
        output.append(body)
        output.append("@END")
        output.append("---:---")
        output.append("")
        last_mode = mode
        last_sig_val = sig_val
        last_argc = len(args)
        addr = addr_end + 1

    with open(out_file, 'w') as f:
        f.write("\n".join(output))
    print("[mad5gen] 生成 " + out_file + " (" + str(len(lines)) + " 条命令)")

if __name__ == "__main__":
    main()
