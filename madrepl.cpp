// madrepl.cpp - MADLANG REPL v2 (含文件/编译/打包)
// compile: g++ -std=c++11 -O2 -pthread -o madrepl madrepl.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <random>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>
#include <ctime>
#include <unistd.h>
#include <sys/wait.h>
#include <dirent.h>

// ================= base64 =================
static const char B64C[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int b64rev[256];
static bool b64init = false;
static void initB64() {
    if (b64init) return;
    for (int i = 0; i < 256; i++) b64rev[i] = -1;
    for (int i = 0; i < 64; i++) b64rev[(unsigned char)B64C[i]] = i;
    b64init = true;
}
static std::string b64decode(const std::string& in) {
    initB64();
    std::string out; uint32_t buf = 0; int bits = 0;
    for (size_t i = 0; i < in.size(); i++) {
        char c = in[i];
        if (c == '=') break;
        int d = b64rev[(unsigned char)c];
        if (d < 0) continue;
        buf = (buf << 6) | (uint32_t)d;
        bits += 6;
        if (bits >= 8) { bits -= 8; out += (char)((buf>>bits)&0xFF); buf &= (1u<<bits)-1u; }
    }
    return out;
}
static std::string b64encode(const std::string& in) {
    std::string out; size_t i = 0;
    while (i + 2 < in.size()) {
        uint32_t n = ((uint8_t)in[i]<<16)|((uint8_t)in[i+1]<<8)|(uint8_t)in[i+2];
        out += B64C[(n>>18)&63]; out += B64C[(n>>12)&63];
        out += B64C[(n>>6)&63];  out += B64C[n&63];
        i += 3;
    }
    if (i + 1 == in.size()) {
        uint32_t n = (uint8_t)in[i]<<16;
        out += B64C[(n>>18)&63]; out += B64C[(n>>12)&63]; out += "==";
    } else if (i + 2 == in.size()) {
        uint32_t n = ((uint8_t)in[i]<<16)|((uint8_t)in[i+1]<<8);
        out += B64C[(n>>18)&63]; out += B64C[(n>>12)&63]; out += B64C[(n>>6)&63]; out += '=';
    }
    return out;
}
static std::string b64x2(const std::string& in) { return b64encode(b64encode(in)); }

// ================= token =================
static const char OBF[] = "Il1O0S5Z2B8Qq";
static const int OBFN = 13;
static std::string genTok(std::mt19937& rng, int len) {
    std::uniform_int_distribution<int> d(0, OBFN-1);
    std::string s;
    for (int i = 0; i < len; i++) s += OBF[d(rng)];
    return s;
}
enum {
    CMD_PRINT=0, CMD_PRINTLN, CMD_SET, CMD_INPUT, CMD_JUMP, CMD_IFEQ,
    CMD_CLEAR, CMD_SLEEP, CMD_HALT, CMD_RAND, CMD_ADD, CMD_SUB,
    CMD_READFILE, CMD_TIME,
    CMD_MOV, CMD_CONCAT, CMD_LEN, CMD_MUL, CMD_DIV, CMD_MOD,
    CMD_GT, CMD_LT, CMD_NEQ, CMD_PUSH, CMD_POP, CMD_CALL, CMD_RET,
    CMD_WRITEFILE, CMD_ENV, CMD_ARGV,
    CMD_COUNT
};
static const char* CMD_NAMES[CMD_COUNT] = {
    "PRINT","PRINTLN","SET","INPUT","JUMP","IFEQ",
    "CLEAR","SLEEP","HALT","RAND","ADD","SUB","READFILE","TIME",
    "MOV","CONCAT","LEN","MUL","DIV","MOD",
    "GT","LT","NEQ","PUSH","POP","CALL","RET",
    "WRITEFILE","ENV","ARGV"
};
struct CmdMap { std::string primary, alias1, alias2; };
struct Mapping { CmdMap cmds[CMD_COUNT]; };
static Mapping generateMapping() {
    std::mt19937 rng(0x4D41444Cu);
    std::set<std::string> used;
    Mapping m;
    for (int i = 0; i < CMD_COUNT; i++)
        for (int j = 0; j < 3; j++) {
            std::string t;
            do { t = genTok(rng, 6); } while (used.count(t));
            used.insert(t);
            if (j == 0) m.cmds[i].primary = t;
            else if (j == 1) m.cmds[i].alias1 = t;
            else m.cmds[i].alias2 = t;
        }
    return m;
}
static int lookupCmd(const Mapping& m, const std::string& t) {
    for (int i = 0; i < CMD_COUNT; i++)
        if (t == m.cmds[i].primary || t == m.cmds[i].alias1 || t == m.cmds[i].alias2)
            return i;
    return -1;
}
static std::vector<std::string> splitBy(const std::string& s, char sep) {
    std::vector<std::string> out; std::string cur;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == sep) { out.push_back(cur); cur.clear(); }
        else cur += s[i];
    }
    out.push_back(cur);
    return out;
}

// ================= global =================
static std::string SELF_DIR = ".";
static void detectSelfDir() {
    char p[4096];
    ssize_t n = readlink("/proc/self/exe", p, sizeof(p)-1);
    if (n > 0) { p[n] = 0; std::string s = p; size_t k = s.rfind('/'); if (k != std::string::npos) SELF_DIR = s.substr(0, k); }
}

static int runShell(const std::string& cmd) {
    int r = system(cmd.c_str());
    if (r == -1) return -1;
    if (WIFEXITED(r)) return WEXITSTATUS(r);
    return -1;
}

struct VMState {
    std::map<char, std::string> vars;
    std::vector<std::string> stack;
};
struct Repl {
    Mapping m;
    VMState st;
    std::mt19937 rng;
    std::vector<std::string> buf;
    std::string bufName;
};

static void printErr(const std::string& s) {
    std::cout << "[\033[31m错误\033[0m] " << s << "\n";
}
static void printInfo(const std::string& s) {
    std::cout << "[\033[36m·\033[0m] " << s << "\n";
}
static void printOk(const std::string& s) {
    std::cout << "[\033[32m✓\033[0m] " << s << "\n";
}

static std::string resolveValue(const std::string& arg, VMState& st, bool& ok) {
    std::string twice = b64decode(b64decode(arg));
    if (twice.size() == 2 && twice[0] == '$' && twice[1] >= 'A' && twice[1] <= 'Z') {
        char v = twice[1];
        if (!st.vars.count(v)) { ok = false; return ""; }
        return st.vars[v];
    }
    return twice;
}

// ================= 执行单行 =================
static void execOne(int cmd, const std::vector<std::string>& args,
                    Repl& R) {
    bool ok = true;
    VMState& st = R.st;
    switch (cmd) {
    case CMD_PRINT: case CMD_PRINTLN: {
        if (args.empty()) { printErr("缺参数"); return; }
        std::string v = resolveValue(args[0], st, ok);
        if (!ok) { printErr("未定义变量"); return; }
        std::cout.write(v.data(), (std::streamsize)v.size());
        if (cmd == CMD_PRINTLN) std::cout << "\n";
        std::cout.flush(); return; }
    case CMD_SET: {
        if (args.size() < 2 || args[0].size() != 1) { printErr("变量必须单字母 A-Z"); return; }
        std::string v = resolveValue(args[1], st, ok);
        if (!ok) { printErr("未定义变量"); return; }
        st.vars[args[0][0]] = v;
        printInfo(std::string("") + args[0][0] + " = \"" + v + "\""); return; }
    case CMD_INPUT: {
        if (args.empty() || args[0].size() != 1) { printErr("变量必须单字母 A-Z"); return; }
        std::cout << "? "; std::cout.flush();
        std::string s;
        if (!std::getline(std::cin, s)) s = "";
        st.vars[args[0][0]] = s;
        printInfo(std::string("") + args[0][0] + " = \"" + s + "\""); return; }
    case CMD_CLEAR:
        std::cout << "\x1b[2J\x1b[H"; std::cout.flush(); return;
    case CMD_SLEEP:
        if (args.empty()) { printErr("缺参数"); return; }
        std::this_thread::sleep_for(std::chrono::milliseconds(std::atoi(args[0].c_str())));
        return;
    case CMD_HALT: printInfo("HALT (REPL 忽略)"); return;
    case CMD_RAND: {
        if (args.size() < 2) { printErr("缺参数"); return; }
        int max = std::atoi(args[1].c_str()); if (max <= 0) max = 1;
        std::uniform_int_distribution<int> d(0, max-1);
        int v = d(R.rng);
        st.vars[args[0][0]] = std::to_string(v);
        printInfo(std::string("") + args[0][0] + " = " + std::to_string(v)); return; }
    case CMD_ADD: case CMD_SUB: case CMD_MUL: case CMD_DIV: case CMD_MOD: {
        if (args.size() < 2) { printErr("缺参数"); return; }
        char v = args[0][0]; int n = std::atoi(args[1].c_str());
        int cur = (st.vars.count(v) && !st.vars[v].empty()) ? std::atoi(st.vars[v].c_str()) : 0;
        if (cmd == CMD_ADD) cur += n;
        else if (cmd == CMD_SUB) cur -= n;
        else if (cmd == CMD_MUL) cur *= n;
        else if (cmd == CMD_DIV) { if (n == 0) { printErr("除零"); return; } cur /= n; }
        else { if (n == 0) { printErr("模零"); return; } cur %= n; }
        st.vars[v] = std::to_string(cur);
        printInfo(std::string("") + v + " = " + std::to_string(cur)); return; }
    case CMD_TIME: {
        if (args.empty()) { printErr("缺参数"); return; }
        long long t = (long long)std::time(nullptr);
        st.vars[args[0][0]] = std::to_string(t);
        printInfo(std::string("") + args[0][0] + " = " + std::to_string(t)); return; }
    case CMD_MOV: {
        if (args.size() < 2) { printErr("缺参数"); return; }
        char d = args[0][0], s = args[1][0];
        if (!st.vars.count(s)) { printErr("未定义变量"); return; }
        st.vars[d] = st.vars[s];
        printInfo(std::string("") + d + " = " + s); return; }
    case CMD_CONCAT: {
        if (args.size() < 2) { printErr("缺参数"); return; }
        char d = args[0][0], s = args[1][0];
        if (!st.vars.count(s)) { printErr("未定义变量"); return; }
        st.vars[d] += st.vars[s];
        printInfo(std::string("") + d + " = \"" + st.vars[d] + "\""); return; }
    case CMD_LEN: {
        if (args.size() < 2) { printErr("缺参数"); return; }
        char d = args[0][0], s = args[1][0];
        if (!st.vars.count(s)) { printErr("未定义变量"); return; }
        st.vars[d] = std::to_string(st.vars[s].size());
        printInfo(std::string("") + d + " = " + st.vars[d]); return; }
    case CMD_PUSH: {
        if (args.empty()) { printErr("缺参数"); return; }
        char v = args[0][0];
        if (!st.vars.count(v)) { printErr("未定义变量"); return; }
        st.stack.push_back(st.vars[v]);
        printInfo("push, size=" + std::to_string(st.stack.size())); return; }
    case CMD_POP: {
        if (args.empty()) { printErr("缺参数"); return; }
        if (st.stack.empty()) { printErr("栈空"); return; }
        char v = args[0][0];
        st.vars[v] = st.stack.back(); st.stack.pop_back();
        printInfo(std::string("") + v + " <- \"" + st.vars[v] + "\""); return; }
    case CMD_ENV: {
        if (args.size() < 2) { printErr("缺参数"); return; }
        std::string name = resolveValue(args[1], st, ok);
        if (!ok) { printErr("未定义变量"); return; }
        const char* val = std::getenv(name.c_str());
        st.vars[args[0][0]] = val ? val : "";
        printInfo(std::string("") + args[0][0] + " = \"" + st.vars[args[0][0]] + "\""); return; }
    case CMD_READFILE: {
        if (args.size() < 2) { printErr("缺参数"); return; }
        std::string path = resolveValue(args[1], st, ok);
        if (!ok) { printErr("未定义变量"); return; }
        std::ifstream f(path.c_str(), std::ios::binary);
        if (!f.good()) { printErr("打不开 " + path); return; }
        std::stringstream ss; ss << f.rdbuf();
        st.vars[args[0][0]] = ss.str();
        printInfo(std::string("") + args[0][0] + " <- file (" + std::to_string(st.vars[args[0][0]].size()) + "B)"); return; }
    case CMD_WRITEFILE: {
        if (args.size() < 2) { printErr("缺参数"); return; }
        char pv = args[0][0], cv = args[1][0];
        if (!st.vars.count(pv) || !st.vars.count(cv)) { printErr("未定义变量"); return; }
        std::ofstream of(st.vars[pv].c_str(), std::ios::binary);
        if (!of.good()) { printErr("写不了"); return; }
        of.write(st.vars[cv].data(), (std::streamsize)st.vars[cv].size());
        printInfo("写了 " + std::to_string(st.vars[cv].size()) + "B → " + st.vars[pv]); return; }
    case CMD_JUMP: case CMD_IFEQ: case CMD_NEQ:
    case CMD_GT: case CMD_LT: case CMD_CALL: case CMD_RET:
        printErr(std::string(CMD_NAMES[cmd]) + " 在 REPL 单行模式不可用（用 :run 跑文件）"); return;
    case CMD_ARGV:
        printErr("ARGV 在 REPL 中无意义"); return;
    }
}

// ================= 缓冲区 → .mgl 文件 =================
static bool bufHasLineNumbers(const std::vector<std::string>& buf) {
    for (size_t i = 0; i < buf.size(); i++) {
        if (buf[i].empty() || buf[i][0] == '#') continue;
        size_t p = buf[i].find('|');
        if (p == std::string::npos) return false;
        std::string first = buf[i].substr(0, p);
        bool allDigit = !first.empty();
        for (size_t j = 0; j < first.size(); j++)
            if (first[j] < '0' || first[j] > '9') { allDigit = false; break; }
        return allDigit;
    }
    return false;
}
static std::string bufToMgl(const std::vector<std::string>& buf) {
    bool hasNo = bufHasLineNumbers(buf);
    std::ostringstream o;
    for (size_t i = 0; i < buf.size(); i++) {
        if (buf[i].empty()) continue;
        if (hasNo) o << buf[i] << "\n";
        else o << (i + 1) << "|" << buf[i] << "\n";
    }
    return o.str();
}

// ================= 元命令 =================
static void banner() {
    std::cout << "\n";
    std::cout << "  \033[33m╔════════════════════════════════════════════╗\033[0m\n";
    std::cout << "  \033[33m║\033[0m  \033[1mMADLANG REPL\033[0m  \033[36mv4.0 IDE\033[0m                    \033[33m║\033[0m\n";
    std::cout << "  \033[33m║\033[0m  \033[32m:help\033[0m 看帮助   \033[32m:edit\033[0m 写程序   \033[32m:run\033[0m 运行   \033[33m║\033[0m\n";
    std::cout << "  \033[33m╚════════════════════════════════════════════╝\033[0m\n\n";
}
static void help() {
    std::cout << "\n  \033[36m▎直接输入\033[0m\n";
    std::cout << "    <token>|<b64x2参数>      执行一行（禁空格，无行号）\n\n";
    std::cout << "  \033[36m▎伪命令\033[0m\n";
    std::cout << "    :help  :h          帮助\n";
    std::cout << "    :map   :m          token 表\n";
    std::cout << "    :vars  :v          变量\n";
    std::cout << "    :stack             栈\n";
    std::cout << "    :clear             清空变量和栈\n";
    std::cout << "    :quit  :q          退出\n\n";
    std::cout << "  \033[36m▎文件 / 缓冲区\033[0m\n";
    std::cout << "    :ls                列出 examples/*.mgl\n";
    std::cout << "    :new               清空缓冲区\n";
    std::cout << "    :edit              进入多行编辑（:end 结束 / :abort 取消）\n";
    std::cout << "    :show              显示缓冲区\n";
    std::cout << "    :del <n>           删除缓冲区第 n 行\n";
    std::cout << "    :load <file>       载入文件到缓冲区\n";
    std::cout << "    :save <file>       保存缓冲区到 examples/<file>\n\n";
    std::cout << "  \033[36m▎执行 / 编译 / 打包\033[0m\n";
    std::cout << "    :run               运行缓冲区（解释）\n";
    std::cout << "    :runf <file>       运行 examples/<file>（解释）\n";
    std::cout << "    :cc [name]         编译缓冲区 → 原生二进制\n";
    std::cout << "    :bundle [name]     打包缓冲区 → .bundle\n\n";
    std::cout << "  \033[36m▎工具\033[0m\n";
    std::cout << "    :b64 <text>        双重 base64 编码\n";
    std::cout << "    :dec <text>        双重 base64 解码\n";
    std::cout << "    :sh <cmd>          执行 shell 命令\n\n";
}
static void showMap(const Mapping& m) {
    std::cout << "\n  \033[33m命令 token 表\033[0m\n\n";
    for (int i = 0; i < CMD_COUNT; i++) {
        std::cout << "    " << CMD_NAMES[i];
        for (size_t j = strlen(CMD_NAMES[i]); j < 12; j++) std::cout << ' ';
        std::cout << "\033[32m" << m.cmds[i].primary << "\033[0m\n";
    }
    std::cout << "\n";
}
static void showVars(const VMState& st) {
    std::cout << "\n";
    if (st.vars.empty()) std::cout << "  (无)\n";
    else for (std::map<char,std::string>::const_iterator it = st.vars.begin();
              it != st.vars.end(); ++it)
        std::cout << "    \033[32m" << it->first << "\033[0m = \"\033[36m" << it->second << "\033[0m\"\n";
    std::cout << "\n";
}
static void showStack(const VMState& st) {
    std::cout << "\n";
    if (st.stack.empty()) std::cout << "  (空)\n";
    else for (int i = (int)st.stack.size()-1; i >= 0; i--)
        std::cout << "    [" << i << "] \"" << st.stack[i] << "\"\n";
    std::cout << "\n";
}
static void showBuf(const Repl& R) {
    std::cout << "\n  \033[33m缓冲区";
    if (!R.bufName.empty()) std::cout << " [" << R.bufName << "]";
    std::cout << "\033[0m (" << R.buf.size() << " 行)\n\n";
    if (R.buf.empty()) std::cout << "  (空，用 :edit 或 :load 填充)\n";
    else for (size_t i = 0; i < R.buf.size(); i++)
        std::cout << "    \033[90m" << (i+1) << "\033[0m  " << R.buf[i] << "\n";
    std::cout << "\n";
}
static void cmdLs() {
    std::string dir = SELF_DIR + "/examples";
    DIR* d = opendir(dir.c_str());
    if (!d) { printErr("打不开 " + dir); return; }
    std::cout << "\n  \033[33mexamples/\033[0m\n\n";
    struct dirent* e;
    int n = 0;
    while ((e = readdir(d)) != NULL) {
        std::string name = e->d_name;
        if (name.size() > 4 && name.substr(name.size()-4) == ".mgl") {
            std::cout << "    " << name << "\n";
            n++;
        }
    }
    closedir(d);
    if (n == 0) std::cout << "    (无)\n";
    std::cout << "\n";
}
static void cmdLoad(Repl& R, const std::string& name) {
    std::string path = SELF_DIR + "/examples/" + name;
    std::ifstream f(path.c_str());
    if (!f.good()) { printErr("打不开 examples/" + name); return; }
    R.buf.clear();
    std::string ln;
    while (std::getline(f, ln)) {
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        R.buf.push_back(ln);
    }
    R.bufName = name;
    printOk("载入 examples/" + name + " (" + std::to_string(R.buf.size()) + " 行)");
}
static void cmdEdit(Repl& R) {
    printInfo("多行编辑（每行一条，无行号）");
    printInfo("  :end 保存  |  :abort 取消");
    std::vector<std::string> tmp;
    while (true) {
        std::cout << "\033[90m|\033[0m " << std::flush;
        std::string ln;
        if (!std::getline(std::cin, ln)) break;
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (ln == ":end") break;
        if (ln == ":abort") { printInfo("已取消"); return; }
        tmp.push_back(ln);
    }
    R.buf = tmp;
    R.bufName = "";
    printOk("缓冲区 " + std::to_string(R.buf.size()) + " 行");
}
static void cmdSave(Repl& R, const std::string& name) {
    if (R.buf.empty()) { printErr("缓冲区为空"); return; }
    std::string fn = name;
    if (fn.size() < 4 || fn.substr(fn.size()-4) != ".mgl") fn += ".mgl";
    std::string path = SELF_DIR + "/examples/" + fn;
    std::ofstream f(path.c_str());
    f << bufToMgl(R.buf);
    R.bufName = fn;
    printOk("已保存 examples/" + fn);
}
static void cmdRunBuf(Repl& R) {
    if (R.buf.empty()) { printErr("缓冲区为空"); return; }
    std::string tmp = SELF_DIR + "/.madrepl_buf.mgl";
    std::ofstream f(tmp.c_str());
    f << bufToMgl(R.buf);
    f.close();
    std::string cmd = "\"" + SELF_DIR + "/madlang\" \"" + tmp + "\" < /dev/null";
    printInfo("运行缓冲区...");
    std::cout << "\033[90m────\033[0m\n";
    std::cout.flush();
    int rc = runShell(cmd);
    std::cout << "\033[90m────\033[0m";
    if (rc == 0) std::cout << " \033[32m[exit 0]\033[0m\n";
    else std::cout << " \033[31m[exit " << rc << "]\033[0m\n";
}
static void cmdRunFile(const std::string& name) {
    std::string path = SELF_DIR + "/examples/" + name;
    std::ifstream f(path.c_str());
    if (!f.good()) { printErr("打不开 examples/" + name); return; }
    std::string cmd = "\"" + SELF_DIR + "/madlang\" \"" + path + "\" < /dev/null";
    std::cout << "\033[90m────\033[0m\n";
    int rc = runShell(cmd);
    std::cout << "\033[90m────\033[0m";
    if (rc == 0) std::cout << " \033[32m[exit 0]\033[0m\n";
    else std::cout << " \033[31m[exit " << rc << "]\033[0m\n";
}
static void cmdCompile(Repl& R, const std::string& name) {
    if (R.buf.empty()) { printErr("缓冲区为空"); return; }
    std::string base = name.empty() ? "madrepl_out" : name;
    std::string mgl = SELF_DIR + "/" + base + ".mgl";
    std::string cpp = SELF_DIR + "/" + base + ".cpp";
    std::string exe = SELF_DIR + "/" + base;
    {
        std::ofstream f(mgl.c_str()); f << bufToMgl(R.buf);
    }
    printInfo("madlangc " + base + ".mgl → " + base + ".cpp");
    int rc = runShell("\"" + SELF_DIR + "/madlangc\" \"" + mgl + "\" \"" + cpp + "\"");
    if (rc != 0) { printErr("madlangc 失败 rc=" + std::to_string(rc)); return; }
    printInfo("g++ " + base + ".cpp → " + base);
    std::cout.flush();
    rc = runShell("g++ -std=c++11 -O2 -o \"" + exe + "\" \"" + cpp + "\"");
    if (rc != 0) { printErr("g++ 失败 rc=" + std::to_string(rc)); return; }
    printOk("生成 " + base + "（可执行）");
    printInfo("  源码: " + base + ".mgl   中间: " + base + ".cpp");
    printInfo("  运行: ./" + base);
}
static void cmdBundle(Repl& R, const std::string& name) {
    if (R.buf.empty()) { printErr("缓冲区为空"); return; }
    std::string base = name.empty() ? "madrepl_bundle" : name;
    std::string mgl = SELF_DIR + "/.madrepl_tmp.mgl";
    {
        std::ofstream f(mgl.c_str()); f << bufToMgl(R.buf);
    }
    printInfo("madctl build → " + base + ".bundle");
    int rc = runShell("\"" + SELF_DIR + "/madctl\" build \"" + mgl + "\" \"" + base + "\"");
    if (rc != 0) { printErr("打包失败 rc=" + std::to_string(rc)); return; }
    printOk("生成 " + base + ".bundle");
}
static void cmdSh(const std::string& cmd) {
    if (cmd.empty()) { printErr(":sh 需要参数"); return; }
    std::cout << "\033[90m$ " << cmd << "\033[0m\n";
    int rc = runShell(cmd);
    std::cout << "\033[90m[exit " << rc << "]\033[0m\n";
}

// ================= main =================
int main(int argc, char** argv) {
    detectSelfDir();
    Repl R;
    R.m = generateMapping();
    R.rng.seed((unsigned)std::time(nullptr));

    if (argc >= 2 && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
        std::cout << "madrepl - MADLANG REPL IDE\n";
        std::cout << "  madrepl                    交互模式\n";
        std::cout << "  madrepl -e \"<token>|<arg>\" 执行一行后退出\n";
        return 0;
    }
    if (argc >= 3 && std::string(argv[1]) == "-e") {
        std::vector<std::string> parts = splitBy(argv[2], '|');
        if (parts.empty() || parts[0].empty()) return 1;
        int cmd = lookupCmd(R.m, parts[0]);
        if (cmd < 0) { printErr("未知 token"); return 1; }
        std::vector<std::string> a(parts.begin()+1, parts.end());
        execOne(cmd, a, R);
        return 0;
    }

    banner();
    std::string line;
    while (true) {
        std::cout << "\033[32mmgl>\033[0m " << std::flush;
        if (!std::getline(std::cin, line)) { std::cout << "\n"; break; }
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;

        if (line[0] == ':') {
            size_t sp = line.find(' ');
            std::string c = (sp == std::string::npos) ? line.substr(1) : line.substr(1, sp-1);
            std::string arg = (sp == std::string::npos) ? "" : line.substr(sp+1);
            // 去首尾空格
            while (!arg.empty() && arg[0] == ' ') arg.erase(0, 1);
            while (!arg.empty() && arg.back() == ' ') arg.pop_back();

            if (c == "help" || c == "h") help();
            else if (c == "map" || c == "m") showMap(R.m);
            else if (c == "vars" || c == "v") showVars(R.st);
            else if (c == "stack") showStack(R.st);
            else if (c == "clear") { R.st.vars.clear(); R.st.stack.clear(); printInfo("已清空"); }
            else if (c == "quit" || c == "q") break;
            else if (c == "ls") cmdLs();
            else if (c == "new") { R.buf.clear(); R.bufName = ""; printInfo("缓冲区已清空"); }
            else if (c == "edit") cmdEdit(R);
            else if (c == "show") showBuf(R);
            else if (c == "del") {
                int n = std::atoi(arg.c_str());
                if (n < 1 || n > (int)R.buf.size()) { printErr("行号越界"); }
                else { R.buf.erase(R.buf.begin() + n - 1); printInfo("删除第 " + std::to_string(n) + " 行"); }
            }
            else if (c == "load") {
                if (arg.empty()) { printErr(":load <file>"); }
                else cmdLoad(R, arg);
            }
            else if (c == "save") {
                if (arg.empty()) { printErr(":save <file>"); }
                else cmdSave(R, arg);
            }
            else if (c == "run") cmdRunBuf(R);
            else if (c == "runf") {
                if (arg.empty()) { printErr(":runf <file>"); }
                else cmdRunFile(arg);
            }
            else if (c == "cc" || c == "compile") cmdCompile(R, arg);
            else if (c == "bundle") cmdBundle(R, arg);
            else if (c == "b64") {
                if (arg.empty()) { printErr(":b64 <text>"); }
                else std::cout << "  " << b64x2(arg) << "\n";
            }
            else if (c == "dec") {
                if (arg.empty()) { printErr(":dec <text>"); }
                else std::cout << "  " << b64decode(b64decode(arg)) << "\n";
            }
            else if (c == "sh") cmdSh(arg);
            else printErr("未知 :" + c + "（试 :help）");
            continue;
        }

        if (line.find(' ') != std::string::npos) { printErr("不允许空格"); continue; }
        std::vector<std::string> parts = splitBy(line, '|');
        if (parts.empty() || parts[0].empty()) { printErr("空命令"); continue; }
        int cmd = lookupCmd(R.m, parts[0]);
        if (cmd < 0) { printErr("未知 token: " + parts[0] + "（试 :map）"); continue; }
        std::vector<std::string> a(parts.begin()+1, parts.end());
        execOne(cmd, a, R);
    }
    std::cout << "Bye.\n";
    return 0;
}
