// madgen.cpp - MADLANG v5/v6 生成器（C++ 版）
// 编译: g++ -std=c++11 -O2 -o madgen madgen.cpp
// 用法:
//   madgen v5 <spec> <out.m5>
//   madgen v6 <spec> <out.m6>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <random>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>

static const char B64C[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
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

static uint32_t fnv1a(const std::string& s) {
    uint32_t h = 2166136261u;
    for (size_t i = 0; i < s.size(); i++) { h ^= (unsigned char)s[i]; h *= 16777619u; }
    return h;
}

static const char OBF[] = "Il1O0S5Z2B8Qq";
static const int OBFN = 13;

static std::string genTok(std::mt19937& rng, int len) {
    std::uniform_int_distribution<int> d(0, OBFN-1);
    std::string s;
    for (int i = 0; i < len; i++) s += OBF[d(rng)];
    return s;
}

// ========== 命令表 ==========
static std::vector<std::string> CMD_LIST_V5;
static std::vector<std::string> CMD_LIST_V6;
static void initCmdLists() {
    CMD_LIST_V5.clear();
    const char* v5[] = {
        "PRINT","PRINTLN","SET","INPUT","JUMP","IFEQ",
        "CLEAR","SLEEP","HALT","RAND","ADD","SUB","READFILE","TIME",
        "MOV","CONCAT","LEN","MUL","DIV","MOD",
        "GT","LT","NEQ","PUSH","POP","CALL","RET","WRITEFILE","ENV","ARGV"
    };
    for (int i = 0; i < 30; i++) CMD_LIST_V5.push_back(v5[i]);

    CMD_LIST_V6.clear();
    const char* v6[] = {
        "PRINT","PRINTLN","SET","INPUT","JUMP","IFEQ",
        "CLEAR","SLEEP","HALT","RAND","ADD","SUB","READFILE","TIME",
        "MOV","CONCAT","LEN","MUL","DIV","MOD",
        "GT","LT","NEQ","PUSH","POP","CALL","RET","WRITEFILE","ENV","ARGV",
        "TREAD","TWRITE","COMEFROM","PLEASE",
        "DIM","DECL","BIND","SALT","CHK","COMMIT","REFRESH","UNSET",
        "STR_LEN","STR_AT","STR_SUB","STR_FIND","STR_SPLIT","STR_UPPER",
        "STR_LOWER","STR_TRIM","STR_REPL","STR_STARTS","STR_ENDS","CHR","ORD",
        "LST_NEW","LST_PUSH","LST_POP","LST_GET","LST_SET","LST_LEN","LST_DEL","LST_INS",
        "SIN","COS","TAN","SQRT","POW","LOG","EXP","ABS","FLOOR","CEIL","ROUND","MIN","MAX",
        "GOTO","COLOR","BGCOLOR","CLR_LINE","CLR_SCREEN",
        "DRAW_CH","DRAW_STR","DRAW_HLINE","DRAW_VLINE","DRAW_BOX",
        "F_OPEN","F_CLOSE","F_READ","F_READLN","F_WRITE","F_SEEK","F_TELL",
        "DIR_LIST","FILE_DEL","FILE_REN",
        "CMP","CMP_IMM","JE","JNE","JL","JG","JZ","JNZ","NOP",
        "LOAD","STORE","OUT","OUT_LN",
        "TIME_MS","TIME_NS","TIME_FMT","SLEEP_MS"
    };
    for (size_t i = 0; i < sizeof(v6)/sizeof(v6[0]); i++) CMD_LIST_V6.push_back(v6[i]);
}

// ========== op 表 ==========
static std::vector<std::vector<std::string> > OP_TABLE;
static void genOpTable(const std::vector<std::string>& cmds) {
    std::mt19937 rng(0x4D35334Du);
    std::set<std::string> used;
    OP_TABLE.clear();
    OP_TABLE.resize(10);
    for (int md = 0; md < 10; md++) {
        OP_TABLE[md].resize(cmds.size());
        for (size_t c = 0; c < cmds.size(); c++) {
            std::string s;
            do {
                std::uniform_int_distribution<int> dl(2, 3);
                int len = dl(rng);
                s = "";
                for (int i = 0; i < len; i++) {
                    std::uniform_int_distribution<int> dp(0, OBFN-1);
                    s += OBF[dp(rng)];
                }
            } while (used.count(s));
            used.insert(s);
            OP_TABLE[md][c] = s;
        }
    }
}

// ========== 关键字 ==========
static std::vector<std::string> KW_V5; // [SCHEMA,MEM,STACK,TMP,REG,BODY,END,SEP]
static std::vector<std::string> KW_V6; // [SCHEMA,MEM,STACK,TMP,REG,H,BODY,END,P,SEP,MUT,SCENE,WHO,WHERE]
static void genKeywords() {
    KW_V5.clear();
    {
        std::mt19937 rng(0x4B57534Du);
        std::set<std::string> used;
        for (int k = 0; k < 8; k++) {
            int L = (k == 7) ? 7 : 5;
            std::string s;
            do {
                s = "";
                for (int i = 0; i < L; i++) {
                    std::uniform_int_distribution<int> dp(0, OBFN-1);
                    s += OBF[dp(rng)];
                }
            } while (used.count(s));
            used.insert(s);
            KW_V5.push_back(s);
        }
    }
    KW_V6.clear();
    {
        std::mt19937 rng(0x4B57534Du);
        std::set<std::string> used;
        for (int k = 0; k < 14; k++) {
            int L = (k == 9) ? 7 : 5;
            std::string s;
            do {
                s = "";
                for (int i = 0; i < L; i++) {
                    std::uniform_int_distribution<int> dp(0, OBFN-1);
                    s += OBF[dp(rng)];
                }
            } while (used.count(s));
            used.insert(s);
            KW_V6.push_back(s);
        }
    }
}

// ========== schema / sigil ==========
struct SchemaDef { char oo, oc, ao, ac, sep; };
static SchemaDef SCHEMAS[16] = {
    {0,0,0,0,'|'},{0,0,'[',']',0},{0,0,'{','}',0},{'(',')',0,0,' '},
    {0,0,'(',')',','},{'{','}',0,0,':'},{0,0,0,0,'~'},{0,0,0,0,'^'},
    {'[',']','<','>',0},{0,0,0,0,'%'},{0,0,0,0,'#'},{0,0,0,0,'@'},
    {0,0,0,0,'$'},{0,0,0,0,'&'},{0,0,0,0,'*'},{0,0,0,0,'?'}
};
static const char SIGS[4] = {'@','%','&','*'};

// ========== 转义 ==========
static std::string decodeEscape(const std::string& s) {
    std::string out;
    for (size_t i = 0; i < s.size(); i++) {
        if (s[i] == '~' && i + 1 < s.size()) {
            char n = s[i+1];
            switch (n) {
                case 's': out += ' '; break;
                case 'c': out += ';'; break;
                case 't': out += ':'; break;
                case 'd': out += '-'; break;
                case 'u': out += '_'; break;
                case '~': out += '~'; break;
                case 'n': out += '\n'; break;
                default: out += '~'; out += n; break;
            }
            i++;
        } else out += s[i];
    }
    return out;
}

static bool isAllDigits(const std::string& s) {
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); i++) if (s[i] < '0' || s[i] > '9') return false;
    return true;
}
static bool isBFPath(const std::string& s) {
    if (s.empty()) return false;
    for (size_t i = 0; i < s.size(); i++) {
        char c = s[i];
        if (c!='<' && c!='>' && c!='^' && c!='v' && c!='+' && c!='-' && c!='@') return false;
    }
    return true;
}

// v5 参数编码
static std::string encodeArgV5(const std::string& arg) {
    if (arg.size() == 1 && arg[0] >= 'A' && arg[0] <= 'Z') return arg;
    if (isAllDigits(arg)) return arg;
    if (arg.size() == 3 && arg[0] == '#' && arg[1] == '$' && arg[2] >= '0' && arg[2] <= '9') {
        return b64x2(arg);
    }
    return b64x2(decodeEscape(arg));
}

// v6 参数编码（哈希前缀 + Unlambda 前缀 + BF 路径）
static std::string encodeArgV6(const std::string& arg) {
    if (isBFPath(arg) && !arg.empty()) return "00" + arg;
    if (arg.size() == 1 && arg[0] >= 'A' && arg[0] <= 'Z') return "00" + arg;
    if (isAllDigits(arg)) return "00" + arg;
    std::string raw;
    if (arg.size() == 2 && arg[0] == '$' && arg[1] >= 'A' && arg[1] <= 'Z') raw = "." + arg;
    else raw = decodeEscape(arg);
    std::string b1 = b64encode(raw);
    std::string b2 = b64encode(b1);
    uint32_t h = fnv1a(b2) & 0xFF;
    char pfx[4]; snprintf(pfx, sizeof(pfx), "%02X", h);
    return std::string(pfx) + b2;
}

// ========== body 拼接 ==========
static std::string buildBodyTail(int schema, const std::string& op,
                                  const std::vector<std::string>& args) {
    char oo = SCHEMAS[schema].oo, oc = SCHEMAS[schema].oc;
    char ao = SCHEMAS[schema].ao, ac = SCHEMAS[schema].ac;
    char sep = SCHEMAS[schema].sep;
    std::string r;
    if (ao && ac) {
        r = op;
        if (!args.empty()) {
            for (size_t i = 0; i < args.size(); i++) { r += ao; r += args[i]; r += ac; }
        } else { r += ao; r += ac; }
    } else {
        r = op;
        for (size_t i = 0; i < args.size(); i++) { r += sep; r += args[i]; }
    }
    if (oo && oc) r = std::string(1,oo) + r + oc;
    return r;
}

// ========== spec 读取 ==========
static bool readSpec(const std::string& path, std::vector<std::string>& out) {
    std::ifstream f(path.c_str());
    if (!f.good()) return false;
    std::string ln;
    while (std::getline(f, ln)) {
        if (!ln.empty() && ln.back() == '\r') ln.pop_back();
        if (ln.empty() || ln[0] == '#') continue;
        out.push_back(ln);
    }
    return true;
}

static void splitLine(const std::string& line, std::string& cmd, std::vector<std::string>& args) {
    std::istringstream iss(line);
    iss >> cmd;
    std::string a;
    while (iss >> a) args.push_back(a);
}

static int findCmd(const std::vector<std::string>& list, const std::string& name) {
    for (size_t i = 0; i < list.size(); i++) if (list[i] == name) return (int)i;
    return -1;
}

// ========== v5 生成 ==========
static int buildV5(const std::string& specFile, const std::string& outFile) {
    initCmdLists();
    genOpTable(CMD_LIST_V5);
    genKeywords();
    std::vector<std::string> spec;
    if (!readSpec(specFile, spec)) { std::cerr << "打不开 " << specFile << "\n"; return 1; }

    std::ostringstream out;
    uint32_t addr = 0x10;
    int lastMode = -1, lastSigVal = 0, lastArgc = 0;
    for (size_t i = 0; i < spec.size(); i++) {
        std::string cmd; std::vector<std::string> args;
        splitLine(spec[i], cmd, args);
        int mode = (lastMode < 0) ? 0 : (lastMode * 7 + lastSigVal + lastArgc) % 10;
        int cmdIdx = findCmd(CMD_LIST_V5, cmd);
        if (cmdIdx < 0) { std::cerr << "未知命令: " << cmd << "\n"; return 1; }
        std::string op = OP_TABLE[mode][cmdIdx];
        if (mode >= 5) std::reverse(op.begin(), op.end());
        int sq = (int)((i+1) * (i+1));
        int sigIdx = (sq + mode) % 4;
        char sigChar = SIGS[sigIdx];
        int sigVal = sigIdx + 1;
        std::vector<std::string> enc;
        for (size_t k = 0; k < args.size(); k++) enc.push_back(encodeArgV5(args[k]));
        int schema = addr & 0xF;
        std::string bodyTail = buildBodyTail(schema, op, enc);
        char pre[32]; snprintf(pre, sizeof(pre), "%c%d{%d}", sigChar, sq, mode);
        std::string body = std::string(pre) + bodyTail;
        uint32_t bodyLen = body.size();
        uint32_t addrEnd = addr + bodyLen;

        char scBuf[8]; snprintf(scBuf, sizeof(scBuf), "%X", schema);
        char memBuf[32]; snprintf(memBuf, sizeof(memBuf), "%04X-%04X", addr, addrEnd);

        out << KW_V5[0] << "=" << scBuf << "\n";
        out << KW_V5[1] << "=" << memBuf << "\n";
        out << KW_V5[2] << "=2\n";
        out << KW_V5[3] << "=1\n";
        out << KW_V5[4] << "=N/N\n";
        out << KW_V5[5] << "\n";
        out << body << "\n";
        out << KW_V5[6] << "\n";
        out << KW_V5[7] << "\n\n";

        lastMode = mode; lastSigVal = sigVal; lastArgc = (int)args.size();
        addr = addrEnd + 1;
    }
    std::ofstream of(outFile.c_str());
    if (!of.good()) { std::cerr << "写不了 " << outFile << "\n"; return 1; }
    of << out.str();
    std::cout << "[madgen] 生成 " << outFile << " (" << spec.size() << " 条命令)\n";
    return 0;
}

// ========== v6 生成 ==========
static int buildV6(const std::string& specFile, const std::string& outFile) {
    initCmdLists();
    genOpTable(CMD_LIST_V6);
    genKeywords();
    std::vector<std::string> spec;
    if (!readSpec(specFile, spec)) { std::cerr << "打不开 " << specFile << "\n"; return 1; }

    // 每 4 条插一个 PLEASE
    std::vector<std::string> lines;
    for (size_t i = 0; i < spec.size(); i++) {
        if (i % 4 == 0) lines.push_back("PLEASE");
        lines.push_back(spec[i]);
    }

    std::ostringstream out;
    uint32_t addr = 0x10;
    int lastMode = -1, lastSigVal = 0, lastArgc = 0;
    uint32_t prevHash = 0;
    for (size_t i = 0; i < lines.size(); i++) {
        std::string cmd; std::vector<std::string> args;
        splitLine(lines[i], cmd, args);
        int mode = (lastMode < 0) ? 0 : (lastMode * 7 + lastSigVal + lastArgc) % 10;
        int cmdIdx = findCmd(CMD_LIST_V6, cmd);
        if (cmdIdx < 0) { std::cerr << "未知命令: " << cmd << "\n"; return 1; }
        std::string op = OP_TABLE[mode][cmdIdx];
        if (mode >= 5) std::reverse(op.begin(), op.end());
        int sq = (int)((i+1) * (i+1));
        int sigIdx = (sq + mode) % 4;
        char sigChar = SIGS[sigIdx];
        int sigVal = sigIdx + 1;
        std::vector<std::string> enc;
        for (size_t k = 0; k < args.size(); k++) enc.push_back(encodeArgV6(args[k]));
        int schema = addr & 0xF;
        std::string bodyTail = buildBodyTail(schema, op, enc);
        char pre[32]; snprintf(pre, sizeof(pre), "%c%d{%d}", sigChar, sq, mode);
        std::string body = std::string(pre) + bodyTail;
        uint32_t bodyLen = body.size();
        uint32_t addrEnd = addr + bodyLen;
        uint32_t h = fnv1a(body);

        char memBuf[32]; snprintf(memBuf, sizeof(memBuf), "%04X-%04X", addr, addrEnd);
        char hBuf[16]; snprintf(hBuf, sizeof(hBuf), "%08X", h);
        char pBuf[16]; snprintf(pBuf, sizeof(pBuf), "%08X", prevHash);

        out << KW_V6[11] << ":I\n";                       // SCENE
        out << KW_V6[12] << ":main\n";                    // WHO
        out << KW_V6[13] << ":0,0\n";                     // WHERE
        out << KW_V6[10] << "=NO\n";                      // MUT
        out << KW_V6[0] << "=?\n";                        // SCHEMA
        out << KW_V6[1] << "=" << memBuf << "\n";         // MEM
        out << KW_V6[2] << "=2\n";                        // STACK
        out << KW_V6[3] << "=1\n";                        // TMP
        out << KW_V6[4] << "=N/N\n";                      // REG
        out << KW_V6[5] << "=" << hBuf << "\n";           // H
        out << KW_V6[6] << "\n";                          // BODY
        out << body << "\n";
        out << KW_V6[7] << "\n";                          // END
        out << KW_V6[8] << "=" << pBuf << "\n";           // P
        out << KW_V6[9] << "\n\n";                        // SEP

        lastMode = mode; lastSigVal = sigVal; lastArgc = (int)args.size();
        prevHash = h;
        addr = addrEnd + 1;
    }
    std::ofstream of(outFile.c_str());
    if (!of.good()) { std::cerr << "写不了 " << outFile << "\n"; return 1; }
    of << out.str();
    std::cout << "[madgen] 生成 " << outFile << " (" << lines.size() << " 条命令)\n";
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 4 || std::string(argv[1]) == "--help" || std::string(argv[1]) == "-h") {
        std::cout << "madgen - MADLANG v5/v6 生成器\n\n";
        std::cout << "用法:\n";
        std::cout << "  madgen v5 <spec> <out.m5>\n";
        std::cout << "  madgen v6 <spec> <out.m6>\n";
        return 0;
    }
    std::string mode = argv[1];
    if (mode == "v5") return buildV5(argv[2], argv[3]);
    if (mode == "v6") return buildV6(argv[2], argv[3]);
    std::cerr << "未知模式: " << mode << "（只支持 v5 / v6）\n";
    return 1;
}
