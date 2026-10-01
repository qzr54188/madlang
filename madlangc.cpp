// madlangc.cpp - MADLANG v4 编译器
// compile: g++ -std=c++11 -O2 -o madlangc madlangc.cpp
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

struct Line { int lineNo; int cmd; std::vector<std::string> args; };

static std::vector<std::string> splitBy(const std::string& s, char sep) {
    std::vector<std::string> out; std::string cur;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == sep) { out.push_back(cur); cur.clear(); }
        else cur += s[i];
    }
    out.push_back(cur);
    return out;
}

static std::map<int, Line> parse(const std::string& source, const Mapping& m, std::string& err) {
    std::map<int, Line> program;
    std::istringstream src(source);
    std::string raw; int rawNum = 0;
    while (std::getline(src, raw)) {
        rawNum++;
        if (!raw.empty() && raw.back() == '\r') raw.pop_back();
        if (raw.empty()) continue;
        if (raw[0] == '#') continue;
        if (raw.find(' ') != std::string::npos) {
            err = "raw " + std::to_string(rawNum) + ": space forbidden";
            return program;
        }
        std::vector<std::string> parts = splitBy(raw, '|');
        if (parts.size() < 2) {
            err = "raw " + std::to_string(rawNum) + ": malformed";
            return program;
        }
        int lineNo = std::atoi(parts[0].c_str());
        int cmd = lookupCmd(m, parts[1]);
        if (cmd < 0) {
            err = "raw " + std::to_string(rawNum) + ": unknown token";
            return program;
        }
        Line ln; ln.lineNo = lineNo; ln.cmd = cmd;
        for (size_t i = 2; i < parts.size(); i++) ln.args.push_back(parts[i]);
        program[lineNo] = ln;
    }
    return program;
}

static std::string escapeCpp(const std::string& s) {
    std::string o = "\"";
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = (unsigned char)s[i];
        switch (c) {
            case '"': o += "\\\""; break;
            case '\\': o += "\\\\"; break;
            case '\n': o += "\\n"; break;
            case '\r': o += "\\r"; break;
            case '\t': o += "\\t"; break;
            default:
                if (c < 0x20) { char b[8]; snprintf(b, sizeof(b), "\\x%02x", c); o += b; }
                else o += (char)c;
        }
    }
    o += "\"";
    return o;
}

static std::string resolveCpp(const std::string& arg) {
    std::string twice = b64decode(b64decode(arg));
    if (twice.size() == 2 && twice[0] == '$' && twice[1] >= 'A' && twice[1] <= 'Z')
        return std::string("V['") + twice[1] + "']";
    return escapeCpp(twice);
}

static std::string genCpp(const std::map<int,Line>& program, const std::string& srcName) {
    std::vector<int> order;
    for (std::map<int,Line>::const_iterator it = program.begin(); it != program.end(); ++it)
        order.push_back(it->first);
    if (order.empty()) return "int main(){return 0;}\n";

    std::ostringstream c;
    c << "// auto-generated by madlangc from " << srcName << "\n";
    c << "// compile: g++ -std=c++11 -O2 -o prog prog.cpp\n\n";
    c << "#include <iostream>\n#include <fstream>\n#include <sstream>\n";
    c << "#include <string>\n#include <vector>\n#include <map>\n";
    c << "#include <random>\n#include <chrono>\n#include <thread>\n";
    c << "#include <cstdlib>\n#include <ctime>\n\n";
    c << "int main(int argc, char** argv) {\n";
    c << "    std::map<char,std::string> V;\n";
    c << "    std::vector<std::string> STK, CALLSTK, ARGV;\n";
    c << "    for (int i = 1; i < argc; i++) ARGV.push_back(argv[i]);\n";
    c << "    std::mt19937 RNG((unsigned)std::time(nullptr));\n";
    c << "    int PC = " << order[0] << ";\n";
    c << "    while (PC != 0) {\n";
    c << "        int NX = 0;\n";
    c << "        switch (PC) {\n";

    for (size_t i = 0; i < order.size(); i++) {
        int ln = order[i];
        int nx = (i + 1 < order.size()) ? order[i+1] : 0;
        const Line& L = program.find(ln)->second;
        c << "        case " << ln << ": {\n";
        switch (L.cmd) {
            case CMD_PRINT:
                c << "            std::cout << " << resolveCpp(L.args[0]) << ";\n";
                c << "            std::cout.flush();\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_PRINTLN:
                c << "            std::cout << " << resolveCpp(L.args[0]) << " << \"\\n\";\n";
                c << "            std::cout.flush();\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_SET:
                c << "            V['" << L.args[0][0] << "'] = " << resolveCpp(L.args[1]) << ";\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_INPUT:
                c << "            { std::string _s; if (!std::getline(std::cin, _s)) _s = \"\";\n";
                c << "              V['" << L.args[0][0] << "'] = _s; }\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_JUMP:
                c << "            NX = " << L.args[0] << "; break;\n"; break;
            case CMD_IFEQ:
                c << "            if (V['" << L.args[0][0] << "'] == " << resolveCpp(L.args[1]) << ") NX = " << L.args[2] << ";\n";
                c << "            else NX = " << nx << "; break;\n"; break;
            case CMD_NEQ:
                c << "            if (V['" << L.args[0][0] << "'] != " << resolveCpp(L.args[1]) << ") NX = " << L.args[2] << ";\n";
                c << "            else NX = " << nx << "; break;\n"; break;
            case CMD_CLEAR:
                c << "            std::cout << \"\\x1b[2J\\x1b[H\"; std::cout.flush();\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_SLEEP:
                c << "            std::this_thread::sleep_for(std::chrono::milliseconds(" << L.args[0] << "));\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_HALT:
                c << "            return 0;\n"; break;
            case CMD_RAND:
                c << "            { std::uniform_int_distribution<int> _d(0, " << L.args[1] << "-1);\n";
                c << "              V['" << L.args[0][0] << "'] = std::to_string(_d(RNG)); }\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_ADD: case CMD_SUB: case CMD_MUL: case CMD_DIV: case CMD_MOD: {
                char v = L.args[0][0]; int n = atoi(L.args[1].c_str());
                c << "            { int _c = V['" << v << "'].empty() ? 0 : std::atoi(V['" << v << "'].c_str());\n";
                if (L.cmd == CMD_MOD)
                    c << "              if (" << n << " != 0) _c %= " << n << ";\n";
                else {
                    const char* op = (L.cmd == CMD_ADD) ? "+=" : (L.cmd == CMD_SUB) ? "-=" : (L.cmd == CMD_MUL) ? "*=" : "/=";
                    c << "              _c " << op << " " << n << ";\n";
                }
                c << "              V['" << v << "'] = std::to_string(_c); }\n";
                c << "            NX = " << nx << "; break;\n"; break; }
            case CMD_READFILE:
                c << "            { std::ifstream _f(" << resolveCpp(L.args[1]) << ", std::ios::binary);\n";
                c << "              if (!_f.good()) { std::cerr << \"read fail\\n\"; return 1; }\n";
                c << "              std::stringstream _ss; _ss << _f.rdbuf();\n";
                c << "              V['" << L.args[0][0] << "'] = _ss.str(); }\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_WRITEFILE:
                c << "            { std::ofstream _f(V['" << L.args[0][0] << "'].c_str(), std::ios::binary);\n";
                c << "              _f.write(V['" << L.args[1][0] << "'].data(), (std::streamsize)V['" << L.args[1][0] << "'].size()); }\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_TIME:
                c << "            V['" << L.args[0][0] << "'] = std::to_string((long long)std::time(nullptr));\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_MOV:
                c << "            V['" << L.args[0][0] << "'] = V['" << L.args[1][0] << "'];\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_CONCAT:
                c << "            V['" << L.args[0][0] << "'] += V['" << L.args[1][0] << "'];\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_LEN:
                c << "            V['" << L.args[0][0] << "'] = std::to_string(V['" << L.args[1][0] << "'].size());\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_GT: case CMD_LT: {
                char a = L.args[0][0], b = L.args[1][0];
                const char* op = (L.cmd == CMD_GT) ? ">" : "<";
                c << "            if (std::atol(V['" << a << "'].c_str()) " << op << " std::atol(V['" << b << "'].c_str())) NX = " << L.args[2] << ";\n";
                c << "            else NX = " << nx << "; break;\n"; break; }
            case CMD_PUSH:
                c << "            STK.push_back(V['" << L.args[0][0] << "']);\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_POP:
                c << "            if (STK.empty()) { std::cerr << \"stack empty\\n\"; return 1; }\n";
                c << "            V['" << L.args[0][0] << "'] = STK.back(); STK.pop_back();\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_CALL:
                c << "            CALLSTK.push_back(\"" << nx << "\");\n";
                c << "            NX = " << L.args[0] << "; break;\n"; break;
            case CMD_RET:
                c << "            if (CALLSTK.empty()) return 0;\n";
                c << "            NX = std::atoi(CALLSTK.back().c_str()); CALLSTK.pop_back();\n";
                c << "            break;\n"; break;
            case CMD_ENV:
                c << "            { const char* _e = std::getenv(" << resolveCpp(L.args[1]) << ");\n";
                c << "              V['" << L.args[0][0] << "'] = _e ? _e : \"\"; }\n";
                c << "            NX = " << nx << "; break;\n"; break;
            case CMD_ARGV: {
                char v = L.args[0][0]; int idx = atoi(L.args[1].c_str());
                c << "            V['" << v << "'] = (" << idx << " < (int)ARGV.size()) ? ARGV[" << idx << "] : \"\";\n";
                c << "            NX = " << nx << "; break;\n"; break; }
        }
        c << "        }\n";
    }
    c << "        default: return 0;\n";
    c << "        }\n";
    c << "        PC = NX;\n";
    c << "    }\n";
    c << "    return 0;\n";
    c << "}\n";
    return c.str();
}

int main(int argc, char** argv) {
    if (argc < 2 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        std::cout << "madlangc - MADLANG 编译器\n\n";
        std::cout << "用法: madlangc <input.mgl> [output.cpp]\n\n";
        std::cout << "生成 .cpp 后编译:\n";
        std::cout << "  g++ -std=c++11 -O2 -o <exe> <output.cpp>\n";
        return 0;
    }
    std::string inPath = argv[1];
    std::string outPath;
    if (argc >= 3) outPath = argv[2];
    else {
        size_t dot = inPath.rfind('.');
        if (dot != std::string::npos && inPath.substr(dot) == ".mgl")
            outPath = inPath.substr(0, dot) + ".cpp";
        else
            outPath = inPath + ".cpp";
    }
    std::ifstream f(inPath.c_str());
    if (!f.good()) { std::cerr << "cannot open " << inPath << "\n"; return 1; }
    std::stringstream ss; ss << f.rdbuf();
    Mapping m = generateMapping();
    std::string err;
    std::map<int,Line> program = parse(ss.str(), m, err);
    if (!err.empty()) { std::cerr << "[madlangc] parse error: " << err << "\n"; return 1; }
    std::string cpp = genCpp(program, inPath);
    std::ofstream o(outPath.c_str());
    if (!o.good()) { std::cerr << "cannot write " << outPath << "\n"; return 1; }
    o.write(cpp.data(), (std::streamsize)cpp.size());
    o.close();
    std::cout << "[madlangc] generated " << outPath << " (" << cpp.size() << " bytes)\n";
    std::cout << "[madlangc] next: g++ -std=c++11 -O2 -o "
              << outPath.substr(0, outPath.rfind('.')) << " " << outPath << "\n";
    return 0;
}
