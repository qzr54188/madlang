// madlang.cpp - MADLANG v4.0
// compile: g++ -std=c++11 -O2 -pthread -o madlang madlang.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <random>
#include <functional>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <thread>
#include <chrono>
#include <ctime>
#include <sys/stat.h>

#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
typedef SOCKET sock_t;
#define MADCLOSE closesocket
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
typedef int sock_t;
#define MADCLOSE close
#define INVALID_SOCKET (-1)
#endif

static const char B64C[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int b64rev[256];
static bool b64init = false;
static void initB64() {
    if (b64init) return;
    for (int i = 0; i < 256; i++) b64rev[i] = -1;
    for (int i = 0; i < 64; i++) b64rev[(unsigned char)B64C[i]] = i;
    b64init = true;
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
static std::string b64decode(const std::string& in) {
    initB64();
    std::string out;
    uint32_t buf = 0; int bits = 0;
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
static std::string b64x2(const std::string& in) { return b64encode(b64encode(in)); }

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

static void writeDoc(const std::string& path, const Mapping& m) {
    std::ofstream f(path.c_str());
    if (!f.good()) return;
    f << "# MADLANG doc v4.0\n\n";
    f << "## 混淆清单（主 token）\n\n";
    f << "| op | token | args |\n|----|-------|------|\n";
    f << "| PRINT     | " << m.cmds[CMD_PRINT].primary     << " | <b64x2> |\n";
    f << "| PRINTLN   | " << m.cmds[CMD_PRINTLN].primary   << " | <b64x2> |\n";
    f << "| SET       | " << m.cmds[CMD_SET].primary       << " | <var>|<b64x2> |\n";
    f << "| INPUT     | " << m.cmds[CMD_INPUT].primary     << " | <var> |\n";
    f << "| JUMP      | " << m.cmds[CMD_JUMP].primary      << " | <line> |\n";
    f << "| IFEQ      | " << m.cmds[CMD_IFEQ].primary      << " | <var>|<b64x2>|<line> |\n";
    f << "| CLEAR     | " << m.cmds[CMD_CLEAR].primary     << " | - |\n";
    f << "| SLEEP     | " << m.cmds[CMD_SLEEP].primary     << " | <ms> |\n";
    f << "| HALT      | " << m.cmds[CMD_HALT].primary      << " | - |\n";
    f << "| RAND      | " << m.cmds[CMD_RAND].primary      << " | <var>|<max> |\n";
    f << "| ADD       | " << m.cmds[CMD_ADD].primary       << " | <var>|<int> |\n";
    f << "| SUB       | " << m.cmds[CMD_SUB].primary       << " | <var>|<int> |\n";
    f << "| READFILE  | " << m.cmds[CMD_READFILE].primary  << " | <var>|<b64x2路径> |\n";
    f << "| TIME      | " << m.cmds[CMD_TIME].primary      << " | <var> |\n";
    f << "| MOV       | " << m.cmds[CMD_MOV].primary       << " | <var>|<var> |\n";
    f << "| CONCAT    | " << m.cmds[CMD_CONCAT].primary    << " | <var>|<var> |\n";
    f << "| LEN       | " << m.cmds[CMD_LEN].primary       << " | <var>|<var> |\n";
    f << "| MUL       | " << m.cmds[CMD_MUL].primary       << " | <var>|<int> |\n";
    f << "| DIV       | " << m.cmds[CMD_DIV].primary       << " | <var>|<int> |\n";
    f << "| MOD       | " << m.cmds[CMD_MOD].primary       << " | <var>|<int> |\n";
    f << "| GT        | " << m.cmds[CMD_GT].primary        << " | <var>|<var>|<line> |\n";
    f << "| LT        | " << m.cmds[CMD_LT].primary        << " | <var>|<var>|<line> |\n";
    f << "| NEQ       | " << m.cmds[CMD_NEQ].primary       << " | <var>|<b64x2>|<line> |\n";
    f << "| PUSH      | " << m.cmds[CMD_PUSH].primary      << " | <var> |\n";
    f << "| POP       | " << m.cmds[CMD_POP].primary       << " | <var> |\n";
    f << "| CALL      | " << m.cmds[CMD_CALL].primary      << " | <line> |\n";
    f << "| RET       | " << m.cmds[CMD_RET].primary       << " | - |\n";
    f << "| WRITEFILE | " << m.cmds[CMD_WRITEFILE].primary << " | <varPath>|<varContent> |\n";
    f << "| ENV       | " << m.cmds[CMD_ENV].primary       << " | <var>|<b64x2name> |\n";
    f << "| ARGV      | " << m.cmds[CMD_ARGV].primary      << " | <var>|<idx> |\n";
    f << "\n## 源码格式\n\n";
    f << "<lineNo>|<token>|<arg1>|<arg2>|...\n";
    f << "行首 # 为注释，整行忽略。\n\n";
    f << "## 规则\n\n";
    f << "1. 源码不允许出现空格\n";
    f << "2. 参数值双重 base64：b64(b64(text))\n";
    f << "3. 行号 = 逻辑行号；执行按行号升序，与物理顺序无关\n";
    f << "4. 变量名为单个大写字母 A-Z\n";
    f << "5. 参数解码两次得 $X 时取变量 X 的值\n";
    f << "6. JUMP/IFEQ/GT/LT/NEQ/CALL 目标为逻辑行号\n\n";
    f << "## 运行\n\n";
    f << "madlang <program.mgl> [args...]\n";
    f << "madlang --serve [port]\n";
    f << "madlang --mapping\n";
    f << "madlang --regen\n";
    f.close();
}

static void writeFile(const std::string& path, const std::string& content) {
    std::ofstream f(path.c_str(), std::ios::binary);
    if (f.good()) f.write(content.data(), (std::streamsize)content.size());
}
static void writeExamples(const Mapping& m) {
#ifdef _WIN32
    _mkdir("examples");
#else
    mkdir("examples", 0755);
#endif
    { std::string s;
      s += "1|" + m.cmds[CMD_PRINTLN].primary + "|" + b64x2("Hello from MADLANG v4") + "\n";
      s += "2|" + m.cmds[CMD_HALT].primary + "\n";
      writeFile("examples/hello.mgl", s); }
    { std::string s;
      s += "1|" + m.cmds[CMD_SET].primary + "|A|" + b64x2("0") + "\n";
      s += "2|" + m.cmds[CMD_ADD].primary + "|A|1\n";
      s += "3|" + m.cmds[CMD_PRINTLN].primary + "|" + b64x2("$A") + "\n";
      s += "4|" + m.cmds[CMD_IFEQ].primary + "|A|" + b64x2("5") + "|6\n";
      s += "5|" + m.cmds[CMD_JUMP].primary + "|2\n";
      s += "6|" + m.cmds[CMD_HALT].primary + "\n";
      writeFile("examples/count.mgl", s); }
    { std::string s;
      s += "1|" + m.cmds[CMD_RAND].primary + "|A|10\n";
      s += "2|" + m.cmds[CMD_PRINTLN].primary + "|" + b64x2("Guess 0-9: ") + "\n";
      s += "3|" + m.cmds[CMD_INPUT].primary + "|B\n";
      s += "4|" + m.cmds[CMD_IFEQ].primary + "|B|" + b64x2("$A") + "|8\n";
      s += "5|" + m.cmds[CMD_PRINTLN].primary + "|" + b64x2("Wrong") + "\n";
      s += "6|" + m.cmds[CMD_HALT].primary + "\n";
      s += "8|" + m.cmds[CMD_PRINTLN].primary + "|" + b64x2("Correct!") + "\n";
      s += "9|" + m.cmds[CMD_HALT].primary + "\n";
      writeFile("examples/guess.mgl", s); }
    { std::string s;
      s += "1|" + m.cmds[CMD_SET].primary + "|A|" + b64x2("Hello, ") + "\n";
      s += "2|" + m.cmds[CMD_SET].primary + "|B|" + b64x2("world") + "\n";
      s += "3|" + m.cmds[CMD_CONCAT].primary + "|A|B\n";
      s += "4|" + m.cmds[CMD_CALL].primary + "|100\n";
      s += "5|" + m.cmds[CMD_PRINTLN].primary + "|" + b64x2("back in main") + "\n";
      s += "6|" + m.cmds[CMD_HALT].primary + "\n";
      s += "100|" + m.cmds[CMD_PRINTLN].primary + "|" + b64x2("$A") + "\n";
      s += "101|" + m.cmds[CMD_RET].primary + "\n";
      writeFile("examples/fun.mgl", s); }
}

static void ensureInitialized(const Mapping& m, bool force) {
    bool need = force;
    if (!need) {
        std::ifstream chk("MADLANG_DOC.md");
        if (!chk.good()) need = true;
        else {
            std::stringstream ss; ss << chk.rdbuf();
            if (ss.str().find("MADLANG doc v4.0") == std::string::npos) need = true;
        }
    }
    if (need) {
        writeDoc("MADLANG_DOC.md", m);
        writeExamples(m);
        std::cout << "[MADLANG] 初始化完成 v4.0\n";
        std::cout << "  文档:    MADLANG_DOC.md\n";
        std::cout << "  示例:    examples/{hello,count,guess,fun}.mgl\n";
        std::cout << "  命令数:  " << CMD_COUNT << "\n\n";
    }
}

struct Line { int lineNo; int cmd; std::vector<std::string> args; };
struct RunResult { int exit_code; std::string out; std::string err; };
typedef std::function<bool(std::string&)> LineReader;

static std::vector<std::string> splitBy(const std::string& s, char sep) {
    std::vector<std::string> out; std::string cur;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == sep) { out.push_back(cur); cur.clear(); }
        else cur += s[i];
    }
    out.push_back(cur);
    return out;
}
static std::string resolveValue(const std::string& arg,
                                std::map<char,std::string>& vars, bool& ok) {
    std::string twice = b64decode(b64decode(arg));
    if (twice.size() == 2 && twice[0] == '$' && twice[1] >= 'A' && twice[1] <= 'Z') {
        char v = twice[1];
        if (!vars.count(v)) { ok = false; return ""; }
        return vars[v];
    }
    return twice;
}

static RunResult runProgram(const std::string& source, LineReader reader,
                            const Mapping& m,
                            const std::vector<std::string>& cliArgs) {
    RunResult res; res.exit_code = 0;
    std::map<int,Line> program;
    std::istringstream src(source);
    std::string raw; int rawNum = 0;
    while (std::getline(src, raw)) {
        rawNum++;
        if (!raw.empty() && raw.back() == '\r') raw.pop_back();
        if (raw.empty()) continue;
        if (raw[0] == '#') continue;
        if (raw.find(' ') != std::string::npos) {
            res.err = "[MADLANG] err raw " + std::to_string(rawNum) + ": space forbidden\n";
            res.exit_code = 1; return res;
        }
        std::vector<std::string> parts = splitBy(raw, '|');
        if (parts.size() < 2) {
            res.err = "[MADLANG] err raw " + std::to_string(rawNum) + ": malformed\n";
            res.exit_code = 1; return res;
        }
        int lineNo = std::atoi(parts[0].c_str());
        int cmd = lookupCmd(m, parts[1]);
        if (cmd < 0) {
            res.err = "[MADLANG] err raw " + std::to_string(rawNum) + ": unknown token\n";
            res.exit_code = 1; return res;
        }
        Line ln; ln.lineNo = lineNo; ln.cmd = cmd;
        for (size_t i = 2; i < parts.size(); i++) ln.args.push_back(parts[i]);
        program[lineNo] = ln;
    }

    std::vector<int> order;
    for (std::map<int,Line>::iterator it = program.begin(); it != program.end(); ++it)
        order.push_back(it->first);
    std::map<int, size_t> lineIdx;
    for (size_t i = 0; i < order.size(); i++) lineIdx[order[i]] = i;

    std::map<char, std::string> vars;
    std::vector<std::string> stack;
    std::vector<size_t> callstack;
    std::ostringstream out, err;
    std::mt19937 rng((unsigned)std::time(nullptr));
    size_t pc = 0; int guard = 0;
    while (pc < order.size()) {
        if (++guard > 1000000) { err << "[MADLANG] step limit\n"; res.exit_code = 1; break; }
        Line& ln = program[order[pc]];
        bool jumped = false; bool ok = true;
        switch (ln.cmd) {
        case CMD_PRINT: case CMD_PRINTLN: {
            std::string v = resolveValue(ln.args[0], vars, ok);
            if (!ok) { err << "[MADLANG] line " << ln.lineNo << ": undef var\n"; res.exit_code = 1; goto done; }
            out.write(v.data(), (std::streamsize)v.size());
            if (ln.cmd == CMD_PRINTLN) out << "\n";
            break; }
        case CMD_SET: {
            if (ln.args.size() < 2 || ln.args[0].size() != 1 ||
                ln.args[0][0] < 'A' || ln.args[0][0] > 'Z') {
                err << "[MADLANG] line " << ln.lineNo << ": bad var\n"; res.exit_code = 1; goto done;
            }
            std::string v = resolveValue(ln.args[1], vars, ok);
            if (!ok) { err << "[MADLANG] line " << ln.lineNo << ": undef var\n"; res.exit_code = 1; goto done; }
            vars[ln.args[0][0]] = v; break; }
        case CMD_INPUT: {
            if (ln.args.size() < 1 || ln.args[0].size() != 1 ||
                ln.args[0][0] < 'A' || ln.args[0][0] > 'Z') {
                err << "[MADLANG] line " << ln.lineNo << ": bad var\n"; res.exit_code = 1; goto done;
            }
            std::string s;
            if (!reader(s)) s = "";
            vars[ln.args[0][0]] = s; break; }
        case CMD_JUMP: {
            int t = std::atoi(ln.args[0].c_str());
            if (!lineIdx.count(t)) { err << "[MADLANG] line " << ln.lineNo << ": bad jump\n"; res.exit_code = 1; goto done; }
            pc = lineIdx[t]; jumped = true; break; }
        case CMD_IFEQ: {
            if (ln.args.size() < 3) { err << "[MADLANG] line " << ln.lineNo << ": ifeq\n"; res.exit_code = 1; goto done; }
            char v = ln.args[0][0];
            std::string val = resolveValue(ln.args[1], vars, ok);
            if (!ok) { err << "[MADLANG] line " << ln.lineNo << ": undef var\n"; res.exit_code = 1; goto done; }
            if (vars.count(v) && vars[v] == val) {
                int t = std::atoi(ln.args[2].c_str());
                if (!lineIdx.count(t)) { err << "[MADLANG] line " << ln.lineNo << ": bad jump\n"; res.exit_code = 1; goto done; }
                pc = lineIdx[t]; jumped = true;
            }
            break; }
        case CMD_NEQ: {
            if (ln.args.size() < 3) { err << "[MADLANG] line " << ln.lineNo << ": neq\n"; res.exit_code = 1; goto done; }
            char v = ln.args[0][0];
            std::string val = resolveValue(ln.args[1], vars, ok);
            if (!ok) { err << "[MADLANG] line " << ln.lineNo << ": undef var\n"; res.exit_code = 1; goto done; }
            if (vars.count(v) && vars[v] != val) {
                int t = std::atoi(ln.args[2].c_str());
                if (!lineIdx.count(t)) { err << "[MADLANG] line " << ln.lineNo << ": bad jump\n"; res.exit_code = 1; goto done; }
                pc = lineIdx[t]; jumped = true;
            }
            break; }
        case CMD_CLEAR: out << "\x1b[2J\x1b[H"; break;
        case CMD_SLEEP: {
            int ms = std::atoi(ln.args[0].c_str());
            std::this_thread::sleep_for(std::chrono::milliseconds(ms));
            break; }
        case CMD_HALT: goto done;
        case CMD_RAND: {
            int max = std::atoi(ln.args[1].c_str());
            if (max <= 0) max = 1;
            std::uniform_int_distribution<int> d(0, max-1);
            vars[ln.args[0][0]] = std::to_string(d(rng));
            break; }
        case CMD_ADD: case CMD_SUB: case CMD_MUL: case CMD_DIV: case CMD_MOD: {
            char v = ln.args[0][0];
            int n = std::atoi(ln.args[1].c_str());
            int cur = (vars.count(v) && !vars[v].empty()) ? std::atoi(vars[v].c_str()) : 0;
            switch (ln.cmd) {
            case CMD_ADD: cur += n; break;
            case CMD_SUB: cur -= n; break;
            case CMD_MUL: cur *= n; break;
            case CMD_DIV:
                if (n == 0) { err << "[MADLANG] line " << ln.lineNo << ": div by 0\n"; res.exit_code = 1; goto done; }
                cur /= n; break;
            case CMD_MOD:
                if (n == 0) { err << "[MADLANG] line " << ln.lineNo << ": mod by 0\n"; res.exit_code = 1; goto done; }
                cur %= n; break;
            }
            vars[v] = std::to_string(cur);
            break; }
        case CMD_READFILE: {
            std::string path = resolveValue(ln.args[1], vars, ok);
            if (!ok) { err << "[MADLANG] line " << ln.lineNo << ": undef var\n"; res.exit_code = 1; goto done; }
            std::ifstream f(path.c_str(), std::ios::binary);
            if (!f.good()) { err << "[MADLANG] line " << ln.lineNo << ": open fail " << path << "\n"; res.exit_code = 1; goto done; }
            std::stringstream ss; ss << f.rdbuf();
            vars[ln.args[0][0]] = ss.str(); break; }
        case CMD_WRITEFILE: {
            if (ln.args.size() < 2) { err << "[MADLANG] line " << ln.lineNo << ": wf args\n"; res.exit_code = 1; goto done; }
            char pv = ln.args[0][0], cv = ln.args[1][0];
            if (!vars.count(pv) || !vars.count(cv)) { err << "[MADLANG] line " << ln.lineNo << ": undef var\n"; res.exit_code = 1; goto done; }
            std::ofstream of(vars[pv].c_str(), std::ios::binary);
            if (!of.good()) { err << "[MADLANG] line " << ln.lineNo << ": open fail\n"; res.exit_code = 1; goto done; }
            of.write(vars[cv].data(), (std::streamsize)vars[cv].size());
            break; }
        case CMD_TIME: vars[ln.args[0][0]] = std::to_string((long long)std::time(nullptr)); break;
        case CMD_MOV: {
            char d = ln.args[0][0], s = ln.args[1][0];
            if (!vars.count(s)) { err << "[MADLANG] line " << ln.lineNo << ": undef " << s << "\n"; res.exit_code = 1; goto done; }
            vars[d] = vars[s]; break; }
        case CMD_CONCAT: {
            char d = ln.args[0][0], s = ln.args[1][0];
            if (!vars.count(s)) { err << "[MADLANG] line " << ln.lineNo << ": undef " << s << "\n"; res.exit_code = 1; goto done; }
            vars[d] = vars[d] + vars[s]; break; }
        case CMD_LEN: {
            char d = ln.args[0][0], s = ln.args[1][0];
            if (!vars.count(s)) { err << "[MADLANG] line " << ln.lineNo << ": undef " << s << "\n"; res.exit_code = 1; goto done; }
            vars[d] = std::to_string(vars[s].size()); break; }
        case CMD_GT: case CMD_LT: {
            char a = ln.args[0][0], b = ln.args[1][0];
            int t = std::atoi(ln.args[2].c_str());
            long la = vars.count(a) ? std::atol(vars[a].c_str()) : 0;
            long lb = vars.count(b) ? std::atol(vars[b].c_str()) : 0;
            bool hit = (ln.cmd == CMD_GT) ? (la > lb) : (la < lb);
            if (hit) {
                if (!lineIdx.count(t)) { err << "[MADLANG] line " << ln.lineNo << ": bad jump\n"; res.exit_code = 1; goto done; }
                pc = lineIdx[t]; jumped = true;
            }
            break; }
        case CMD_PUSH: {
            char v = ln.args[0][0];
            if (!vars.count(v)) { err << "[MADLANG] line " << ln.lineNo << ": undef " << v << "\n"; res.exit_code = 1; goto done; }
            stack.push_back(vars[v]); break; }
        case CMD_POP: {
            char v = ln.args[0][0];
            if (stack.empty()) { err << "[MADLANG] line " << ln.lineNo << ": stack empty\n"; res.exit_code = 1; goto done; }
            vars[v] = stack.back(); stack.pop_back(); break; }
        case CMD_CALL: {
            int t = std::atoi(ln.args[0].c_str());
            if (!lineIdx.count(t)) { err << "[MADLANG] line " << ln.lineNo << ": bad call\n"; res.exit_code = 1; goto done; }
            callstack.push_back(pc + 1);
            pc = lineIdx[t]; jumped = true; break; }
        case CMD_RET: {
            if (callstack.empty()) { err << "[MADLANG] line " << ln.lineNo << ": ret w/o call\n"; res.exit_code = 1; goto done; }
            pc = callstack.back(); callstack.pop_back(); jumped = true; break; }
        case CMD_ENV: {
            char v = ln.args[0][0];
            std::string name = resolveValue(ln.args[1], vars, ok);
            if (!ok) { err << "[MADLANG] line " << ln.lineNo << ": undef var\n"; res.exit_code = 1; goto done; }
            const char* val = std::getenv(name.c_str());
            vars[v] = val ? val : ""; break; }
        case CMD_ARGV: {
            char v = ln.args[0][0];
            int idx = std::atoi(ln.args[1].c_str());
            vars[v] = (idx >= 0 && idx < (int)cliArgs.size()) ? cliArgs[idx] : ""; break; }
        }
        if (!jumped) pc++;
    }
done:
    res.out = out.str();
    if (err.str().size() > 0 && res.err.empty()) res.err = err.str();
    return res;
}

static std::string jsonEscape(const std::string& s) {
    std::string o;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\u%04x", c); o += b; }
                else o += (char)c;
        }
    }
    return o;
}

static const char* HTML_PAGE = R"HTML(<!DOCTYPE html>
<html lang="zh"><head><meta charset="utf-8"><title>MADLANG v4 Web IDE</title>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
*{box-sizing:border-box}
body{font-family:ui-monospace,Menlo,monospace;background:#121212;color:#d0d0d0;margin:0;padding:16px}
h1{color:#ff7043;margin:0 0 4px;font-size:20px}
h3{color:#ff7043;margin:16px 0 6px;font-size:14px}
.tip{color:#888;font-size:11px;margin-bottom:12px}
.wrap{display:grid;grid-template-columns:2fr 1fr;gap:16px}
@media(max-width:900px){.wrap{grid-template-columns:1fr}}
textarea{width:100%;height:360px;background:#0a0a0a;color:#b0ffb0;border:1px solid #333;padding:10px;font-family:inherit;font-size:13px;line-height:1.5;resize:vertical}
button{background:#ff7043;color:#fff;border:0;padding:8px 16px;cursor:pointer;font-size:13px;margin:8px 8px 0 0;border-radius:3px}
button:hover{background:#ff5722}
button.g{background:#444}button.g:hover{background:#555}
pre{background:#0a0a0a;padding:10px;border:1px solid #333;min-height:180px;white-space:pre-wrap;word-break:break-all;font-size:13px;margin:8px 0 0}
pre.err{color:#ff5555}
table{border-collapse:collapse;width:100%;font-size:12px}
td,th{border:1px solid #333;padding:3px 6px;text-align:left}
th{background:#1e1e1e;color:#ff7043}
code{color:#ffcc80}
</style></head><body>
<h1>MADLANG v4 Web IDE</h1>
<div class="tip">&lt;行号&gt;|&lt;token&gt;|&lt;b64x2参数&gt; — 禁空格 | 按行号升序执行 | 行首 # 注释</div>
<div class="wrap">
<div>
<textarea id="src" spellcheck="false"></textarea>
<div>
<button onclick="run()">▶ 运行</button>
<button class="g" onclick="clr()">✕ 清空</button>
<button class="g" onclick="load()">↻ 刷新清单</button>
</div>
<pre id="out">[输出]</pre>
</div>
<div>
<h3>混淆清单 (30条)</h3>
<table id="map"></table>
</div>
</div>
<script>
async function load(){
  const r=await fetch('/mapping');const d=await r.json();
  const t=document.getElementById('map');
  t.innerHTML='<tr><th>命令</th><th>token</th></tr>';
  d.cmds.forEach(c=>{const tr=document.createElement('tr');
    tr.innerHTML='<td>'+c.name+'</td><td><code>'+c.token+'</code></td>';
    t.appendChild(tr);});
}
async function run(){
  const src=document.getElementById('src').value;
  const r=await fetch('/run',{method:'POST',body:src});
  const d=await r.json();
  const o=document.getElementById('out');
  let txt='';
  if(d.stdout)txt+=d.stdout;
  if(d.stderr){txt+=(txt?'\n':'')+'[stderr]\n'+d.stderr;o.classList.add('err');}
  else o.classList.remove('err');
  txt+='\n\n[exit '+d.exit_code+']';
  o.textContent=txt;
}
function clr(){document.getElementById('out').textContent='[输出]';document.getElementById('out').classList.remove('err');}
load();
</script></body></html>
)HTML";

static void runServer(int port, const Mapping& m) {
#ifdef _WIN32
    WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa);
#endif
    sock_t srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv == INVALID_SOCKET) { std::cerr << "socket failed\n"; return; }
    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons((unsigned short)port);
    if (bind(srv, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "bind failed: port " << port << "\n"; return;
    }
    listen(srv, 8);
    std::cout << "[MADLANG] Web IDE: http://127.0.0.1:" << port << "/\n";
    std::cout << "  Ctrl+C 停止\n"; std::cout.flush();
    while (true) {
        sock_t c = accept(srv, 0, 0);
        if (c == INVALID_SOCKET) continue;
        std::string req; char buf[4096]; int n;
        size_t headerEnd = std::string::npos; int cl = 0;
        while ((n = recv(c, buf, sizeof(buf), 0)) > 0) {
            req.append(buf, n);
            headerEnd = req.find("\r\n\r\n");
            if (headerEnd != std::string::npos) {
                std::string hl = req.substr(0, headerEnd);
                std::string lower;
                for (size_t i = 0; i < hl.size(); i++) lower += (char)tolower((unsigned char)hl[i]);
                size_t p = lower.find("\r\ncontent-length:");
                if (p == std::string::npos) break;
                p += 17;
                size_t e = lower.find("\r\n", p);
                cl = std::atoi(lower.substr(p, e-p).c_str());
                if (req.size() - (headerEnd+4) >= (size_t)cl) break;
            }
        }
        std::string body;
        if (headerEnd != std::string::npos) body = req.substr(headerEnd+4);
        if ((int)body.size() > cl && cl > 0) body.resize(cl);

        size_t le = req.find("\r\n");
        std::istringstream fl(le == std::string::npos ? req : req.substr(0, le));
        std::string method, path, ver; fl >> method >> path >> ver;
        size_t q = path.find('?'); if (q != std::string::npos) path.resize(q);

        std::string resp;
        if (method == "GET" && (path == "/" || path == "/index.html")) {
            resp = "HTTP/1.1 200 OK\r\nContent-Type: text/html; charset=utf-8\r\nContent-Length: "
                 + std::to_string(strlen(HTML_PAGE)) + "\r\nConnection: close\r\n\r\n" + HTML_PAGE;
        } else if (method == "GET" && path == "/mapping") {
            std::string j = "{\"cmds\":[";
            for (int i = 0; i < CMD_COUNT; i++) {
                if (i) j += ",";
                j += "{\"name\":\"" + std::string(CMD_NAMES[i]) + "\",\"token\":\"" + m.cmds[i].primary + "\"}";
            }
            j += "]}";
            resp = "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: "
                 + std::to_string(j.size()) + "\r\nConnection: close\r\n\r\n" + j;
        } else if (method == "POST" && path == "/run") {
            LineReader nr = [](std::string&) -> bool { return false; };
            RunResult r = runProgram(body, nr, m, std::vector<std::string>());
            std::string j = "{\"exit_code\":" + std::to_string(r.exit_code)
                          + ",\"stdout\":\"" + jsonEscape(r.out) + "\""
                          + ",\"stderr\":\"" + jsonEscape(r.err) + "\"}";
            resp = "HTTP/1.1 200 OK\r\nContent-Type: application/json; charset=utf-8\r\nContent-Length: "
                 + std::to_string(j.size()) + "\r\nConnection: close\r\n\r\n" + j;
        } else {
            resp = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
        }
        send(c, resp.c_str(), (int)resp.size(), 0);
        MADCLOSE(c);
    }
}

int main(int argc, char** argv) {
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_BINARY);
#endif
    Mapping m = generateMapping();

    if (argc >= 2) {
        std::string a1 = argv[1];
        if (a1 == "--help" || a1 == "-h") {
            std::cout << "MADLANG v4.0 (" << CMD_COUNT << " commands)\n\n";
            std::cout << "madlang <program.mgl> [args...]   运行\n";
            std::cout << "madlang --serve [port]            Web IDE (默认 8765)\n";
            std::cout << "madlang --mapping                 打印清单\n";
            std::cout << "madlang --regen                   重建文档+示例\n";
            return 0;
        }
        if (a1 == "--mapping") {
            for (int i = 0; i < CMD_COUNT; i++)
                std::cout << CMD_NAMES[i] << "\t" << m.cmds[i].primary << "\n";
            return 0;
        }
        if (a1 == "--regen") { ensureInitialized(m, true); return 0; }
        if (a1 == "--serve") {
            ensureInitialized(m, false);
            int port = (argc >= 3) ? std::atoi(argv[2]) : 8765;
            runServer(port, m);
            return 0;
        }
    }

    ensureInitialized(m, false);

    if (argc < 2) {
        std::cout << "usage: madlang <program.mgl> [args...]\n";
        std::cout << "       madlang --serve [port]\n";
        std::cout << "       madlang --help\n";
        return 0;
    }

    std::ifstream f(argv[1]);
    if (!f.good()) {
        std::cerr << "[MADLANG] cannot open: " << argv[1] << "\n";
        return 1;
    }
    std::stringstream src; src << f.rdbuf();

    std::vector<std::string> cliArgs;
    for (int i = 2; i < argc; i++) cliArgs.push_back(argv[i]);

    LineReader reader = [](std::string& line) -> bool {
        return (bool)std::getline(std::cin, line);
    };

    RunResult r = runProgram(src.str(), reader, m, cliArgs);
    std::cout << r.out;
    std::cout.flush();
    std::cerr << r.err;
    return r.exit_code;
}
