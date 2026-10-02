#!/usr/bin/env python3
# mad5gen.py - v5 生成器（v4 混淆 + 声明块关键字混淆）
import sys, os, subprocess, base64

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

def load_kw(bin_path):
    out = subprocess.check_output([bin_path, '--keywords']).decode('utf-8', errors='replace')
    kw = {}
    for line in out.splitlines():
        line = line.strip()
        if '=' not in line: continue
        k, v = line.split('=', 1)
        kw[k] = v
    return kw

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

def decode_spec_escape(s):
    out = []
    i = 0
    while i < len(s):
        if s[i] == '~' and i + 1 < len(s):
            n = s[i+1]
            out.append({'s':' ', 'c':';', 't':':', 'd':'-', 'u':'_', '~':'~', 'n':'\n'}.get(n, '~'+n))
            i += 2
        else:
            out.append(s[i]); i += 1
    return ''.join(out)

def encode_arg(arg):
    # 单字符大写字母 = 变量名，不编码
    if len(arg) == 1 and 'A' <= arg <= 'Z':
        return arg
    # 纯数字 = 数值参数，不编码
    if arg.isdigit():
        return arg
    # 变量引用 #$X，原样双重 base64
    if len(arg) == 3 and arg[0] == '#' and arg[1] == '$' and arg[2].isdigit():
        raw = arg
    else:
        raw = decode_spec_escape(arg)
    b1 = base64.b64encode(raw.encode('utf-8')).decode('ascii')
    b2 = base64.b64encode(b1.encode('ascii')).decode('ascii')
    return b2

def build_body_tail(schema, op, args):
    encoded = [encode_arg(a) for a in args]
    oo, oc, ao, ac, sep = SCHEMAS[schema]
    if ao and ac:
        if encoded:
            r = op + "".join(ao + a + ac for a in encoded)
        else:
            r = op + ao + ac
    else:
        r = (sep or '').join([op] + list(encoded))
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
    KW = load_kw(bin5)

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
        output.append(KW['SCHEMA'] + "=" + format(schema, 'X'))
        output.append(KW['MEM'] + "=" + format(addr, '04X') + "-" + format(addr_end, '04X'))
        output.append(KW['STACK'] + "=2")
        output.append(KW['TMP'] + "=1")
        output.append(KW['REG'] + "=N/N")
        output.append(KW['BODY'])
        output.append(body)
        output.append(KW['END'])
        output.append(KW['SEP'])
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
