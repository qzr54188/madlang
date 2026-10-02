// mad6.cpp - MADLANG v6 Launcher
// compile: g++ -std=c++11 -O2 -o mad6 mad6.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>

static std::string SELF_DIR = ".";
static void detectSelfDir() {
    char p[4096];
    ssize_t n = readlink("/proc/self/exe", p, sizeof(p)-1);
    if (n > 0) { p[n] = 0; std::string s = p; size_t k = s.rfind('/'); if (k != std::string::npos) SELF_DIR = s.substr(0, k); }
}

#define RST "\x1b[0m"
#define BOLD "\x1b[1m"
#define DIM  "\x1b[90m"
#define YEL  "\x1b[33m"
#define GRN  "\x1b[32m"
#define CYA  "\x1b[36m"
#define MAG  "\x1b[35m"
#define RED  "\x1b[31m"
#define ORG  "\x1b[38;5;208m"

static void clr() { std::cout << "\x1b[2J\x1b[H"; std::cout.flush(); }

struct RawMode {
    termios old; bool ok;
    RawMode() : ok(false) {
        if (tcgetattr(STDIN_FILENO, &old) == 0) {
            termios raw = old;
            raw.c_lflag &= ~(ICANON | ECHO);
            raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0;
            if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) ok = true;
        }
    }
    ~RawMode() { if (ok) tcsetattr(STDIN_FILENO, TCSANOW, &old); }
};

static char getch() {
    RawMode raw; char c = 0;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    if (n <= 0) return 0;
    if (c == 17) return 'Q';
    if (c == 27) return 27;
    if (c >= 'a' && c <= 'z') c = c - 'a' + 'A';
    return c;
}

static std::string prompt(const std::string& msg, const std::string& def="") {
    std::cout << "\n  " << msg;
    if (!def.empty()) std::cout << " [" << def << "]";
    std::cout << ": "; std::cout.flush();
    std::string s;
    std::getline(std::cin, s);
    if (s.empty()) s = def;
    return s;
}

static int runShell(const std::string& cmd) {
    std::cout.flush();
    int r = system(cmd.c_str());
    if (r == -1) return -1;
    if (WIFEXITED(r)) return WEXITSTATUS(r);
    return -1;
}

static void pauseAny() {
    std::cout << "\n" DIM "-- 任意键 --" RST; std::cout.flush();
    getch();
}

static std::vector<std::string> listFiles(const std::string& ext) {
    std::vector<std::string> out;
    DIR* d = opendir(SELF_DIR.c_str());
    if (!d) return out;
    struct dirent* e;
    while ((e = readdir(d)) != NULL) {
        std::string n = e->d_name;
        if (n.size() > ext.size() && n.substr(n.size()-ext.size()) == ext) out.push_back(n);
    }
    closedir(d);
    std::sort(out.begin(), out.end());
    return out;
}

static bool fileExists(const std::string& path) {
    struct stat st;
    return stat(path.c_str(), &st) == 0;
}

static void header(const std::string& title) {
    clr();
    std::cout << "\n  " ORG "MADLANG" RST "  " DIM << title << RST "\n";
    std::cout << DIM "  ────────────────────────────────" RST "\n\n";
}

static void ensureFile(const std::string& path, const std::string& content) {
    if (!fileExists(path)) { std::ofstream o(path.c_str()); o << content; }
}

static const char* editorCmd() {
    const char* e = getenv("EDITOR");
    if (e && *e) return e;
    return "vi";
}

static int genM5(const std::string& spec, const std::string& m5) {
    return runShell("cd \"" + SELF_DIR + "\" && python3 mad5gen.py \"" + spec + "\" \"" + m5 + "\"");
}
static int runM5(const std::string& m5) {
    return runShell("cd \"" + SELF_DIR + "\" && ./madlangv5 \"" + m5 + "\" < /dev/null");
}

static std::string pickFile(const std::string& ext) {
    while (true) {
        header("select " + ext);
        std::vector<std::string> files = listFiles(ext);
        if (files.empty()) std::cout << "  " DIM "(空)" RST "\n";
        else for (size_t i = 0; i < files.size(); i++)
            std::cout << "  " YEL << (char)('A'+i) << RST "  " << files[i] << "\n";
        std::cout << "\n  " GRN "N" RST " 新建\n";
        std::cout << "  " RED "Q" RST " 取消\n\n  > "; std::cout.flush();
        char c = getch();
        if (c == 'Q' || c == 27) return "";
        if (c == 'N' && files.size() < 26) {
            std::string name = prompt("文件名", "new" + ext);
            if (name.empty()) continue;
            if (name.size() < ext.size() || name.substr(name.size()-ext.size()) != ext) name += ext;
            std::ofstream o((SELF_DIR + "/" + name).c_str());
            return name;
        }
        if (c >= 'A' && c < 'A' + (int)files.size()) return files[c - 'A'];
    }
}

// ===== 简单模式 =====
static void simpleMode() {
    std::string spec = "work.spec";
    std::string m5 = "work.m5";
    ensureFile(SELF_DIR + "/" + spec, "PRINTLN Hello~sWorld\nHALT\n");
    while (true) {
        header("简单模式");
        std::cout << "  " CYA << spec << RST " -> " CYA << m5 << RST "\n\n";
        std::cout << "  " GRN "E" RST " 编辑 spec\n";
        std::cout << "  " GRN "G" RST " 生成\n";
        std::cout << "  " GRN "V" RST " 查看 m5\n";
        std::cout << "  " GRN "R" RST " 运行\n";
        std::cout << "  " GRN "D" RST " 一键\n";
        std::cout << "  " CYA "O" RST " 打开\n";
        std::cout << "  " CYA "S" RST " 另存\n";
        std::cout << "  " CYA "N" RST " 重置示例\n";
        std::cout << "\n  " RED "Q" RST " 返回\n\n  > "; std::cout.flush();
        char c = getch();
        if (c == 'Q' || c == 27) return;
        if (c == 'E') runShell(std::string(editorCmd()) + " \"" + SELF_DIR + "/" + spec + "\"");
        else if (c == 'O') { std::string f = pickFile(".spec"); if (!f.empty()) { spec = f; m5 = f.substr(0, f.size()-5) + ".m5"; } }
        else if (c == 'S') {
            std::string n = prompt("另存为", spec);
            if (!n.empty()) {
                if (n.size() < 5 || n.substr(n.size()-5) != ".spec") n += ".spec";
                runShell("cp \"" + SELF_DIR + "/" + spec + "\" \"" + SELF_DIR + "/" + n + "\"");
                spec = n; m5 = n.substr(0, n.size()-5) + ".m5";
            }
        }
        else if (c == 'G') { clr(); int rc = genM5(spec, m5); std::cout << "\n" << (rc==0?GRN"  ok":RED"  fail") << RST "\n"; pauseAny(); }
        else if (c == 'V') { clr(); runShell("cat \"" + SELF_DIR + "/" + m5 + "\""); pauseAny(); }
        else if (c == 'R') { clr(); int rc = runM5(m5); std::cout << "\n" DIM "[exit " << rc << "]" RST "\n"; pauseAny(); }
        else if (c == 'D') {
            clr();
            if (genM5(spec, m5) != 0) { std::cout << RED "  gen fail" RST "\n"; pauseAny(); continue; }
            std::cout << "\n"; runShell("cat \"" + SELF_DIR + "/" + m5 + "\"");
            std::cout << "\n" DIM "-- run --" RST "\n\n";
            int rc = runM5(m5);
            std::cout << "\n" DIM "[exit " << rc << "]" RST "\n"; pauseAny();
        }
        else if (c == 'N') {
            std::ofstream o((SELF_DIR + "/" + spec).c_str());
            o << "PRINTLN Hello~sWorld\nHALT\n";
        }
    }
}

// ===== 困难模式 =====
static void hardMode() {
    std::string m5 = "work.m5";
    ensureFile(SELF_DIR + "/" + m5, "");
    while (true) {
        header("困难模式");
        std::cout << "  " CYA << m5 << RST "\n\n";
        std::cout << "  " YEL "E" RST " 编辑\n";
        std::cout << "  " YEL "V" RST " 查看\n";
        std::cout << "  " YEL "R" RST " 运行\n";
        std::cout << "  " MAG "T" RST " 模板\n";
        std::cout << "  " MAG "K" RST " 关键字\n";
        std::cout << "  " MAG "M" RST " op 表\n";
        std::cout << "  " CYA "O" RST " 打开\n";
        std::cout << "  " CYA "S" RST " 另存\n";
        std::cout << "\n  " RED "Q" RST " 返回\n\n  > "; std::cout.flush();
        char c = getch();
        if (c == 'Q' || c == 27) return;
        if (c == 'E') runShell(std::string(editorCmd()) + " \"" + SELF_DIR + "/" + m5 + "\"");
        else if (c == 'O') { std::string f = pickFile(".m5"); if (!f.empty()) m5 = f; }
        else if (c == 'S') {
            std::string n = prompt("另存为", m5);
            if (!n.empty()) {
                if (n.size() < 3 || n.substr(n.size()-3) != ".m5") n += ".m5";
                runShell("cp \"" + SELF_DIR + "/" + m5 + "\" \"" + SELF_DIR + "/" + n + "\"");
                m5 = n;
            }
        }
        else if (c == 'V') { clr(); runShell("cat \"" + SELF_DIR + "/" + m5 + "\""); pauseAny(); }
        else if (c == 'R') {
            clr();
            int rc = runM5(m5);
            std::cout << "\n" << (rc==0?GRN"  ok":RED"  fail") << RST "  " DIM "[exit " << rc << "]" RST "\n";
            pauseAny();
        }
        else if (c == 'T') {
            clr();
            std::cout << "  <SCHEMA>=?   <MEM>=????-????\n";
            std::cout << "  <STACK>=2    <TMP>=1    <REG>=N/N\n";
            std::cout << "  <BODY>\n  ?{?}???\n  <END>\n  <SEP>\n\n";
            std::cout << DIM "  关键字对照:" RST "\n\n";
            runShell("cd \"" + SELF_DIR + "\" && ./madlangv5 --keywords");
            pauseAny();
        }
        else if (c == 'K') { clr(); runShell("cd \"" + SELF_DIR + "\" && ./madlangv5 --keywords"); pauseAny(); }
        else if (c == 'M') { clr(); runShell("cd \"" + SELF_DIR + "\" && ./madlangv5 --map"); pauseAny(); }
    }
}

// ===== 终端 =====
static void termMode() {
    std::string hist[32]; int histN = 0;
    while (true) {
        header("终端");
        std::cout << "  1  madctl info\n";
        std::cout << "  2  madctl compile\n";
        std::cout << "  3  ls -la\n";
        std::cout << "  4  git status\n";
        std::cout << "  5  madlangv5 --map\n";
        std::cout << "  6  madlangv5 --keywords\n";
        std::cout << "  " CYA "H" RST "  历史\n";
        std::cout << "  " CYA "C" RST "  输入命令\n";
        std::cout << "\n  " RED "Q" RST " 返回\n\n  > "; std::cout.flush();
        char c = getch();
        if (c == 'Q' || c == 27) return;
        std::string cmd;
        if (c == '1') cmd = "./madctl info";
        else if (c == '2') cmd = "./madctl compile";
        else if (c == '3') cmd = "ls -la";
        else if (c == '4') cmd = "git status --short 2>&1";
        else if (c == '5') cmd = "./madlangv5 --map";
        else if (c == '6') cmd = "./madlangv5 --keywords";
        else if (c == 'H') {
            clr();
            for (int i = 0; i < histN; i++) std::cout << "  " DIM << (i+1) << RST "  " << hist[i] << "\n";
            pauseAny(); continue;
        }
        else if (c == 'C') {
            std::string s = prompt("cmd", "");
            if (s.empty()) continue;
            if (histN < 32) hist[histN++] = s;
            const char* ok[] = {"madctl ","madlangv5 ","./madlangv5 ","python3 ","g++ ","ls","cat ","stat ","wc ","file ","pwd","git ","cd ","echo ","which ",NULL};
            bool allowed = false;
            for (int i = 0; ok[i]; i++) if (s.compare(0, strlen(ok[i]), ok[i]) == 0) { allowed = true; break; }
            if (!allowed) { clr(); std::cout << RED "  not allowed\n" RST; pauseAny(); continue; }
            cmd = s;
        }
        else continue;
        clr();
        std::cout << DIM "  $ " << cmd << RST "\n\n";
        int rc = runShell("cd \"" + SELF_DIR + "\" && " + cmd);
        std::cout << "\n" DIM "[exit " << rc << "]" RST "\n";
        pauseAny();
    }
}

// ===== 工具 =====
static void toolsMenu() {
    while (true) {
        header("工具");
        std::cout << "  " CYA "C" RST " 编译 v5\n";
        std::cout << "  " CYA "I" RST " 状态\n";
        std::cout << "  " CYA "M" RST " op 表\n";
        std::cout << "  " CYA "K" RST " 关键字\n";
        std::cout << "  " CYA "L" RST " 文件\n";
        std::cout << "  " CYA "G" RST " git status\n";
        std::cout << "  " CYA "P" RST " commit + push\n";
        std::cout << "\n  " RED "Q" RST " 返回\n\n  > "; std::cout.flush();
        char c = getch();
        if (c == 'Q' || c == 27) return;
        if (c == 'C') { clr(); std::cout << YEL "  compiling..." RST "\n\n"; int rc = runShell("cd \"" + SELF_DIR + "\" && g++ -std=c++11 -O2 -pthread -o madlangv5 madlangv5.cpp"); std::cout << "\n" << (rc==0?GRN"  ok":RED"  fail") << RST "\n"; pauseAny(); }
        else if (c == 'I') { clr(); runShell("cd \"" + SELF_DIR + "\" && ./madctl info"); pauseAny(); }
        else if (c == 'M') { clr(); runShell("cd \"" + SELF_DIR + "\" && ./madlangv5 --map"); pauseAny(); }
        else if (c == 'K') { clr(); runShell("cd \"" + SELF_DIR + "\" && ./madlangv5 --keywords"); pauseAny(); }
        else if (c == 'L') { clr(); runShell("cd \"" + SELF_DIR + "\" && ls -la"); pauseAny(); }
        else if (c == 'G') { clr(); runShell("cd \"" + SELF_DIR + "\" && git status"); pauseAny(); }
        else if (c == 'P') {
            std::string msg = prompt("msg", "update");
            if (msg.empty()) continue;
            clr();
            int rc = runShell("cd \"" + SELF_DIR + "\" && git add -A && git commit -m \"" + msg + "\" && git push");
            std::cout << "\n" << (rc==0?GRN"  pushed":RED"  fail") << RST "\n";
            pauseAny();
        }
    }
}

// ===== 主菜单 =====
int main() {
    detectSelfDir();
    chdir(SELF_DIR.c_str());
    ensureFile(SELF_DIR + "/work.spec", "PRINTLN Hello~sWorld\nHALT\n");
    ensureFile(SELF_DIR + "/work.m5", "");
    while (true) {
        clr();
        std::cout << "\n  " ORG "MADLANG v6" RST "\n";
        std::cout << DIM "  ────────────────────────────────" RST "\n\n";
        std::cout << "  " GRN "A" RST "  简单模式\n";
        std::cout << "  " YEL "B" RST "  困难模式\n";
        std::cout << "  " CYA "C" RST "  终端\n";
        std::cout << "  " MAG "D" RST "  工具\n";
        std::cout << "\n  " RED "Q" RST "  退出\n\n  > "; std::cout.flush();
        char c = getch();
        if (c == 'Q' || c == 27) break;
        if (c == 'A') simpleMode();
        else if (c == 'B') hardMode();
        else if (c == 'C') termMode();
        else if (c == 'D') toolsMenu();
    }
    clr();
    return 0;
}
