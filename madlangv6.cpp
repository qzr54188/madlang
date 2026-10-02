// madlangv6.cpp - MADLANG v6 集大成者
// compile: g++ -std=c++11 -O2 -pthread -o madlangv6 madlangv6.cpp
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
#include <cmath>
#include <thread>
#include <chrono>
#include <ctime>

static const char B64C[]="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int b64rev[256]; static bool b64init=false;
static void initB64(){if(b64init)return;for(int i=0;i<256;i++)b64rev[i]=-1;
  for(int i=0;i<64;i++)b64rev[(unsigned char)B64C[i]]=i;b64init=true;}
static std::string b64decode(const std::string&in){initB64();std::string out;
  uint32_t buf=0;int bits=0;for(size_t i=0;i<in.size();i++){char c=in[i];if(c=='=')break;
  int d=b64rev[(unsigned char)c];if(d<0)continue;buf=(buf<<6)|(uint32_t)d;bits+=6;
  if(bits>=8){bits-=8;out+=(char)((buf>>bits)&0xFF);buf&=(1u<<bits)-1u;}}return out;}

enum {CMD_PRINT=0,CMD_PRINTLN,CMD_SET,CMD_INPUT,CMD_JUMP,CMD_IFEQ,
  CMD_CLEAR,CMD_SLEEP,CMD_HALT,CMD_RAND,CMD_ADD,CMD_SUB,
  CMD_READFILE,CMD_TIME,CMD_MOV,CMD_CONCAT,CMD_LEN,CMD_MUL,CMD_DIV,CMD_MOD,
  CMD_GT,CMD_LT,CMD_NEQ,CMD_PUSH,CMD_POP,CMD_CALL,CMD_RET,
  CMD_WRITEFILE,CMD_ENV,CMD_ARGV,CMD_TREAD,CMD_TWRITE,
  CMD_COMEFROM,CMD_PLEASE,
  CMD_COUNT};
static const char* CMD_NAMES[CMD_COUNT]={
  "PRINT","PRINTLN","SET","INPUT","JUMP","IFEQ","CLEAR","SLEEP","HALT","RAND",
  "ADD","SUB","READFILE","TIME","MOV","CONCAT","LEN","MUL","DIV","MOD",
  "GT","LT","NEQ","PUSH","POP","CALL","RET","WRITEFILE","ENV","ARGV",
  "TREAD","TWRITE","COMEFROM","PLEASE"};

static std::string OP_TABLE[10][CMD_COUNT];
static const char OPCHARS[]="Il1O0S5Z2B8Qq";
static const int OPCHARS_N=13;
static void genOpTable(){
  std::mt19937 rng(0x4D35334Du);
  std::set<std::string> used;
  for(int md=0;md<10;md++)for(int c=0;c<CMD_COUNT;c++){
    std::string s;
    do{std::uniform_int_distribution<int> dl(2,3);int len=dl(rng);s="";
      for(int i=0;i<len;i++){std::uniform_int_distribution<int> dp(0,OPCHARS_N-1);s+=OPCHARS[dp(rng)];}
    }while(used.count(s));
    used.insert(s);OP_TABLE[md][c]=s;
  }
}
static int lookupOp(int mode,const std::string&sym){
  std::string s=sym;
  if(mode>=5){std::string r(s.rbegin(),s.rend());s=r;}
  for(int c=0;c<CMD_COUNT;c++)if(OP_TABLE[mode][c]==s)return c;
  return -1;
}

static std::string KW_SCHEMA,KW_MEM,KW_STACK,KW_TMP,KW_REG,KW_H,KW_BODY,KW_END,KW_P,KW_SEP;
static std::string KW_MUT,KW_SCENE,KW_WHO,KW_WHERE;
static void genKeywords(){
  std::mt19937 rng(0x4B57534Du);
  std::set<std::string> used;
  std::string* kws[]={&KW_SCHEMA,&KW_MEM,&KW_STACK,&KW_TMP,&KW_REG,&KW_H,&KW_BODY,&KW_END,&KW_P,&KW_SEP,
    &KW_MUT,&KW_SCENE,&KW_WHO,&KW_WHERE};
  for(int k=0;k<14;k++){
    int L=(k==9)?7:5;
    std::string s;
    do{
      s="";
      for(int i=0;i<L;i++){
        std::uniform_int_distribution<int> dp(0,OPCHARS_N-1);
        s+=OPCHARS[dp(rng)];
      }
    }while(used.count(s));
    used.insert(s);
    *kws[k]=s;
  }
}

static unsigned char TAPE[256]={0};
static int TPX=0,TPY=0;
static char TPDIR='R';  // Befunge: 方向 L/R/U/D

static bool execBFPath(const std::string&p){
  TPX=0;TPY=0;TPDIR='R';
  for(size_t i=0;i<p.size();i++){
    char c=p[i];
    switch(c){
      case '<': TPDIR='L'; if(TPX<=0)return false; TPX--; break;
      case '>': TPDIR='R'; if(TPX>=15)return false; TPX++; break;
      case '^': TPDIR='U'; if(TPY<=0)return false; TPY--; break;
      case 'v': TPDIR='D'; if(TPY>=15)return false; TPY++; break;
      case '+': TAPE[TPY*16+TPX]++; break;
      case '-': TAPE[TPY*16+TPX]--; break;
      case '@': break;
      default: return false;
    }
  }
  return true;
}
static std::string readTapeStr(){
  std::string out;
  int x=TPX,y=TPY;
  while(y>=0&&y<16&&x>=0&&x<16){
    unsigned char v=TAPE[y*16+x];
    if(v==0)break;
    out+=(char)v;
    if(TPDIR=='R'){x++;if(x>=16){x=0;y++;}}
    else if(TPDIR=='L'){x--;if(x<0){x=15;y--;}}
    else if(TPDIR=='D'){y++;if(y>=16){y=0;x++;}}
    else{y--;if(y<0){y=15;x--;}}
  }
  return out;
}
static void writeTapeStr(const std::string&sv){
  int x=TPX,y=TPY;
  for(size_t i=0;i<=sv.size();i++){
    if(y<0||y>=16||x<0||x>=16)return;
    TAPE[y*16+x]=(i<sv.size())?(unsigned char)sv[i]:0;
    if(TPDIR=='R'){x++;if(x>=16){x=0;y++;}}
    else if(TPDIR=='L'){x--;if(x<0){x=15;y--;}}
    else if(TPDIR=='D'){y++;if(y>=16){y=0;x++;}}
    else{y--;if(y<0){y=15;x--;}}
  }
}

static uint32_t fnv1a(const std::string&s){
  uint32_t h=2166136261u;
  for(size_t i=0;i<s.size();i++){h^=(unsigned char)s[i];h*=16777619u;}
  return h;
}
static int popcountStr(const std::string&s){
  int n=0;
  for(size_t i=0;i<s.size();i++){
    unsigned char c=(unsigned char)s[i];
    while(c){n+=(c&1);c>>=1;}
  }
  return n;
}
static bool evenPopcount(const std::string&s){
  return (popcountStr(s)%2)==0;
}

struct SchemaDef{char outer_open,outer_close,arg_open,arg_close,sep;};
static SchemaDef SCHEMAS[16]={
  {0,0,0,0,'|'},{0,0,'[',']',0},{0,0,'{','}',0},{'(',')',0,0,' '},
  {0,0,'(',')',','},{'{','}',0,0,':'},{0,0,0,0,'~'},{0,0,0,0,'^'},
  {'[',']','<','>',0},{0,0,0,0,'%'},{0,0,0,0,'#'},{0,0,0,0,'@'},
  {0,0,0,0,'$'},{0,0,0,0,'&'},{0,0,0,0,'*'},{0,0,0,0,'?'}
};

static std::vector<std::string> splitStr(const std::string&s,char sep){
  std::vector<std::string> out;std::string cur;
  for(size_t i=0;i<s.size();i++){if(s[i]==sep){out.push_back(cur);cur.clear();}else cur+=s[i];}
  out.push_back(cur);return out;
}

struct ParsedBody{std::string op;std::vector<std::string>args;bool ok;};

static ParsedBody parseBodyBySchema(int schema,const std::string&rest){
  ParsedBody p;p.ok=false;
  std::string s=rest;
  const SchemaDef&sd=SCHEMAS[schema];
  if(sd.outer_open&&!s.empty()&&s[0]==sd.outer_open){
    if(s.back()!=sd.outer_close)return p;
    s=s.substr(1,s.size()-2);
  }
  if(sd.arg_open&&sd.arg_close){
    size_t pos=s.find(sd.arg_open);
    if(pos==std::string::npos)return p;
    p.op=s.substr(0,pos);
    s=s.substr(pos);
    while(!s.empty()&&s[0]==sd.arg_open){
      size_t end=s.find(sd.arg_close);
      if(end==std::string::npos)return p;
      std::string av=s.substr(1,end-1);
      if(!av.empty())p.args.push_back(av);
      s=s.substr(end+1);
    }
  }else{
    std::vector<std::string> parts=splitStr(s,sd.sep);
    if(parts.empty())return p;
    p.op=parts[0];
    for(size_t i=1;i<parts.size();i++)p.args.push_back(parts[i]);
  }
  p.ok=true;return p;
}

static std::string decodeEscape(const std::string&s){
  std::string out;
  for(size_t i=0;i<s.size();i++){
    if(s[i]=='~'&&i+1<s.size()){
      char n=s[++i];
      switch(n){
        case 's':out+=' ';break;
        case 'c':out+=';';break;
        case 't':out+=':';break;
        case 'd':out+='-';break;
        case 'u':out+='_';break;
        case '~':out+='~';break;
        case 'n':out+='\n';break;
        default:out+='~';out+=n;break;
      }
    }else out+=s[i];
  }
  return out;
}

struct V6Cmd{
  int logical;uint32_t addr;int schema;int mode;int sigilVal;
  int cmd;std::vector<std::string>args;
};

static bool isPrime(int n){if(n<2)return false;for(int i=2;i*i<=n;i++)if(n%i==0)return false;return true;}
static bool isPow2(int n){return n>0&&(n&(n-1))==0;}
static bool isHex(char c){return (c>='0'&&c<='9')||(c>='A'&&c<='F');}
static bool allHex(const std::string&s){if(s.empty())return false;for(size_t i=0;i<s.size();i++)if(!isHex(s[i]))return false;return true;}

struct RawBlock{
  int schema;uint32_t addrStart,addrEnd;
  int stack;int tmp;std::string regRead,regWrite;
  uint32_t hash;uint32_t prevHash;
  bool mutOK;
  std::string scene,who,where;
  std::string body;
  int depth;
};

static bool parseBlock(const std::vector<std::string>&lines,int off,
                       RawBlock&out,int&consumed,std::string&err,int lastStack,int lastTmp){
  if(off+13>=(int)lines.size()){err="块不完整";return false;}
  int i=off;
  out.depth=0;
  while(i<(int)lines.size()&&lines[i].size()>0&&lines[i][0]=='\t'){out.depth++;i++;}
  std::string first=lines[i];
  // SCENE:XXX
  if(first.size()>KW_SCENE.size()+1&&first.substr(0,KW_SCENE.size())==KW_SCENE&&first[KW_SCENE.size()]==':'){
    out.scene=first.substr(KW_SCENE.size()+1);
  } else { err="缺 SCENE"; return false; }
  i++;
  if(i>=(int)lines.size()||lines[i].size()<=KW_WHO.size()+1||lines[i].substr(0,KW_WHO.size())!=KW_WHO||lines[i][KW_WHO.size()]!=':'){
    err="缺 WHO"; return false; }
  out.who=lines[i].substr(KW_WHO.size()+1); i++;
  if(i>=(int)lines.size()||lines[i].size()<=KW_WHERE.size()+1||lines[i].substr(0,KW_WHERE.size())!=KW_WHERE||lines[i][KW_WHERE.size()]!=':'){
    err="缺 WHERE"; return false; }
  out.where=lines[i].substr(KW_WHERE.size()+1); i++;
  // MUT=NO/OK
  if(i>=(int)lines.size()||lines[i].size()<=KW_MUT.size()+1||lines[i].substr(0,KW_MUT.size())!=KW_MUT||lines[i][KW_MUT.size()]!='='){
    err="缺 MUT"; return false; }
  std::string mv=lines[i].substr(KW_MUT.size()+1);
  out.mutOK=(mv=="OK"||mv=="ok"); i++;
  // SCHEMA=?
  if(i>=(int)lines.size()||lines[i].size()<=KW_SCHEMA.size()+1||lines[i].substr(0,KW_SCHEMA.size())!=KW_SCHEMA||lines[i][KW_SCHEMA.size()]!='='){
    err="缺 SCHEMA"; return false; }
  std::string sc=lines[i].substr(KW_SCHEMA.size()+1); i++;
  // MEM
  if(i>=(int)lines.size()||lines[i].size()<=KW_MEM.size()+1||lines[i].substr(0,KW_MEM.size())!=KW_MEM||lines[i][KW_MEM.size()]!='='){
    err="缺 MEM"; return false; }
  std::string mem=lines[i].substr(KW_MEM.size()+1); i++;
  size_t dash=mem.find('-');
  if(dash==std::string::npos){err="MEM 格式";return false;}
  out.addrStart=(uint32_t)strtoul(mem.substr(0,dash).c_str(),0,16);
  out.addrEnd=(uint32_t)strtoul(mem.substr(dash+1).c_str(),0,16);
  // STACK（支持 ^ / ^+N / ^-N）
  if(i>=(int)lines.size()||lines[i].size()<=KW_STACK.size()+1||lines[i].substr(0,KW_STACK.size())!=KW_STACK||lines[i][KW_STACK.size()]!='='){
    err="缺 STACK"; return false; }
  std::string stk=lines[i].substr(KW_STACK.size()+1); i++;
  if(stk=="^")out.stack=lastStack;
  else if(stk.size()>2&&stk[0]=='^'&&(stk[1]=='+'||stk[1]=='-'))out.stack=lastStack+atoi(stk.substr(1).c_str());
  else out.stack=atoi(stk.c_str());
  if(!isPrime(out.stack)){err="STACK 必须质数";return false;}
  // TMP（支持 ^）
  if(i>=(int)lines.size()||lines[i].size()<=KW_TMP.size()+1||lines[i].substr(0,KW_TMP.size())!=KW_TMP||lines[i][KW_TMP.size()]!='='){
    err="缺 TMP"; return false; }
  std::string tmpv=lines[i].substr(KW_TMP.size()+1); i++;
  if(tmpv=="^")out.tmp=lastTmp;
  else out.tmp=atoi(tmpv.c_str());
  if(!isPow2(out.tmp)&&out.tmp!=0){err="TMP 必须 2 的幂";return false;}
  // REG
  if(i>=(int)lines.size()||lines[i].size()<=KW_REG.size()+1||lines[i].substr(0,KW_REG.size())!=KW_REG||lines[i][KW_REG.size()]!='='){
    err="缺 REG"; return false; }
  std::string reg=lines[i].substr(KW_REG.size()+1); i++;
  size_t sl=reg.find('/');
  if(sl==std::string::npos){err="REG 格式";return false;}
  out.regRead=reg.substr(0,sl);out.regWrite=reg.substr(sl+1);
  // H
  if(i>=(int)lines.size()||lines[i].size()<=KW_H.size()+1||lines[i].substr(0,KW_H.size())!=KW_H||lines[i][KW_H.size()]!='='){
    err="缺 H"; return false; }
  out.hash=(uint32_t)strtoul(lines[i].substr(KW_H.size()+1).c_str(),0,16); i++;
  // BODY
  if(i>=(int)lines.size()||lines[i]!=KW_BODY){err="缺 BODY";return false;}
  i++;
  out.body=lines[i]; i++;
  // END
  if(i>=(int)lines.size()||lines[i]!=KW_END){err="缺 END";return false;}
  i++;
  // P
  if(i>=(int)lines.size()||lines[i].size()<=KW_P.size()+1||lines[i].substr(0,KW_P.size())!=KW_P||lines[i][KW_P.size()]!='='){
    err="缺 P"; return false; }
  out.prevHash=(uint32_t)strtoul(lines[i].substr(KW_P.size()+1).c_str(),0,16); i++;
  // SEP
  if(i>=(int)lines.size()||lines[i]!=KW_SEP){err="缺 SEP";return false;}
  i++;
  // 把 schema 从 ? 解析出来（条件 schema，由哈希参与）
  if(sc=="?"){
    uint32_t h=fnv1a(out.body);
    out.schema = out.addrStart & 0xF;
    // 也要重算 H 是否匹配
    (void)h;
  } else {
    if(sc.size()!=1||!isHex(sc[0])){err="SCHEMA 非法";return false;}
    out.schema=(sc[0]<='9')?(sc[0]-'0'):(sc[0]-'A'+10);
  }
  consumed=i-off;
  return true;
}

static bool parseBody(const std::string&body,int schema,int&mode,int&sigilVal,
                      int&sqrtLine,std::string&op,std::vector<std::string>&args,
                      std::string&err){
  if(body.size()<5){err="正文太短";return false;}
  char sig=body[0];
  static const char SIGS[4]={'@','%','&','*'};
  int sigIdx=-1;
  for(int i=0;i<4;i++)if(SIGS[i]==sig){sigIdx=i;break;}
  if(sigIdx<0){err="sigil 非法";return false;}
  sigilVal=sigIdx+1;
  size_t i=1;
  while(i<body.size()&&body[i]>='0'&&body[i]<='9')i++;
  if(i==1){err="缺平方行号";return false;}
  sqrtLine=atoi(body.substr(1,i-1).c_str());
  if(i>=body.size()||body[i]!='{'){err="缺 {M}";return false;}
  size_t close=body.find('}',i);
  if(close==std::string::npos){err="{M} 未闭合";return false;}
  std::string ms=body.substr(i+1,close-i-1);
  if(ms.size()!=1||ms[0]<'0'||ms[0]>'9'){err="M 非法";return false;}
  mode=ms[0]-'0';
  if(SIGS[(sqrtLine+mode)%4]!=sig){err="sigil 不匹配公式";return false;}
  std::string rest=body.substr(close+1);
  ParsedBody pb=parseBodyBySchema(schema,rest);
  if(!pb.ok){err="schema 解析失败";return false;}
  op=pb.op;args=pb.args;
  // 参数哈希校验：每个参数前 2 位十六进制（L14），前缀后才是真参数
  // 但我们不校验值，只检查前缀存在并剥离
  for(size_t k=0;k<args.size();k++){
    if(args[k].size()<3){err="参数缺哈希前缀";return false;}
    if(!allHex(args[k].substr(0,2))){err="参数哈希前缀非十六进制";return false;}
    args[k]=args[k].substr(2);
  }
  return true;
}

struct Program{std::vector<V6Cmd>cmds;};

static bool parseFile(const std::string&src,Program&prog,std::string&err){
  std::vector<std::string> lines;
  std::istringstream ss(src);std::string ln;
  while(std::getline(ss,ln)){if(!ln.empty()&&ln.back()=='\r')ln.pop_back();lines.push_back(ln);}
  int off=0;int lastMode=-1;int blockCount=0;
  int prevSig=0,prevArgc=0;
  uint32_t prevHash=0;
  int lastStack=2,lastTmp=1;
  std::vector<int> pleaseRecent;  // 最近 5 条是否出现过 PLEASE
  while(off<(int)lines.size()){
    while(off<(int)lines.size()&&lines[off].empty())off++;
    if(off>=(int)lines.size())break;
    RawBlock rb;int consumed;
    if(!parseBlock(lines,off,rb,consumed,err,lastStack,lastTmp)){err="块 "+std::to_string(blockCount)+" 解析: "+err;return false;}
    off+=consumed;
    uint32_t actual=fnv1a(rb.body);
    if(actual!=rb.hash){
      char buf[128];snprintf(buf,sizeof(buf),"块 %d: H 哈希不符（期望 %08X，实际 %08X）",blockCount,rb.hash,actual);
      err=buf;return false;
    }
    if(blockCount>0&&rb.prevHash!=prevHash){
      char buf[128];snprintf(buf,sizeof(buf),"块 %d: P 不符",blockCount);
      err=buf;return false;
    }
    prevHash=rb.hash;
    // 条件 schema 已验，但还要校验 schema 与地址末位的关系？
    // v6: schema 由哈希算，不再必须等于末位
    uint32_t len=rb.addrEnd-rb.addrStart;
    if(len!=(uint32_t)rb.body.size()){
      char buf[128];snprintf(buf,sizeof(buf),"块 %d: MEM 长度 %u != 正文 %zu",blockCount,len,rb.body.size());
      err=buf;return false;
    }
    // L15 popcount
    // Whitespace 深度
    if(rb.depth>3){err="块深度不能超过 3";return false;}
    int mode,sigilVal,sqrtLine;
    std::string op;std::vector<std::string>args;
    if(!parseBody(rb.body,rb.schema,mode,sigilVal,sqrtLine,op,args,err)){
      err="块 "+std::to_string(blockCount)+" 正文: "+err;return false;
    }
    if(blockCount==0){
      if(mode!=0){err="第一块模式必须 0";return false;}
    }else{
      int expected=(lastMode*7+prevSig+prevArgc)%10;
      if(mode!=expected){
        err="模式链断裂：期望 "+std::to_string(expected)+"，实际 "+std::to_string(mode);
        return false;
      }
    }
    int cmd=lookupOp(mode,op);
    if(cmd<0){err="未知 op: "+op+" (模式"+std::to_string(mode)+")";return false;}
    V6Cmd vc;
    vc.logical=(int)sqrt((double)sqrtLine);
    vc.addr=rb.addrStart;
    vc.schema=rb.schema;
    vc.mode=mode;
    vc.sigilVal=sigilVal;
    vc.cmd=cmd;
    vc.args=args;
    prog.cmds.push_back(vc);
    // INTERCAL: PLEASE 密度
    pleaseRecent.push_back(cmd==CMD_PLEASE?1:0);
    if(pleaseRecent.size()>5)pleaseRecent.erase(pleaseRecent.begin());
    if((int)pleaseRecent.size()==5){
      int cnt=0;for(size_t z=0;z<pleaseRecent.size();z++)cnt+=pleaseRecent[z];
      if(cnt<1){err="最近 5 块必须至少有 1 个 PLEASE";return false;}
    }
    lastMode=mode;prevSig=sigilVal;prevArgc=(int)args.size();
    blockCount++;
    while(off<(int)lines.size()&&lines[off].empty())off++;
    lastStack=rb.stack;lastTmp=rb.tmp;
  }
  return true;
}

static std::map<char,std::string> VARS;
// Unlambda: 变量访问必须有 . 前缀
static bool g_unlambdaStrict=true;

static std::string resolveArg(const std::string&a,bool&ok){
  ok=true;
  std::string dec=b64decode(b64decode(a));
  // Unlambda 前缀检查
  if(dec.size()>=3&&dec[0]=='.'&&dec[1]=='$'&&dec[2]>='A'&&dec[2]<='Z'){
    if(!VARS.count(dec[2])){ok=false;return "";}
    return VARS[dec[2]];
  }
  if(g_unlambdaStrict&&dec.size()==2&&dec[0]=='$'&&dec[1]>='A'&&dec[1]<='Z'){
    ok=false;return "";
  }
  return dec;
}

struct VM{std::map<int,size_t>lineMap;std::vector<std::string>stack;
  std::vector<size_t>callstack;std::mt19937 rng;};

static int runProgram(Program&prog){
  VM vm;
  vm.rng.seed((unsigned)std::time(nullptr));
  for(size_t i=0;i<prog.cmds.size();i++)vm.lineMap[prog.cmds[i].logical]=i;
  VARS.clear();
  size_t pc=0;int guard=0;
  while(pc<prog.cmds.size()){
    if(++guard>1000000){std::cerr<<"[v6] step limit\n";return 1;}
    V6Cmd&c=prog.cmds[pc];
    bool jumped=false,ok=true;
    switch(c.cmd){
      case CMD_PRINT:case CMD_PRINTLN:{
        if(c.args.empty()){std::cerr<<"[v6] PRINT\n";return 1;}
        std::string v=resolveArg(c.args[0],ok);
        if(!ok){std::cerr<<"[v6] 变量未定义\n";return 1;}
        std::cout.write(v.data(),(std::streamsize)v.size());
        if(c.cmd==CMD_PRINTLN)std::cout<<"\n";
        std::cout.flush();break;}
      case CMD_SET:{
        if(c.args.size()<2||c.args[0].size()!=1){std::cerr<<"[v6] SET\n";return 1;}
        std::string v=resolveArg(c.args[1],ok);
        if(!ok){std::cerr<<"[v6] 变量未定义\n";return 1;}
        VARS[c.args[0][0]]=v;break;}
      case CMD_INPUT:{
        if(c.args.empty()||c.args[0].size()!=1){std::cerr<<"[v6] INPUT\n";return 1;}
        std::string s;std::getline(std::cin,s);
        VARS[c.args[0][0]]=s;break;}
      case CMD_JUMP:{
        int t=atoi(c.args[0].c_str());
        if(!vm.lineMap.count(t)){std::cerr<<"[v6] 跳转目标不存在\n";return 1;}
        pc=vm.lineMap[t];jumped=true;break;}
      case CMD_IFEQ:{
        if(c.args.size()<3){std::cerr<<"[v6] IFEQ\n";return 1;}
        char v=c.args[0][0];
        std::string val=resolveArg(c.args[1],ok);
        if(!ok){std::cerr<<"[v6] 变量未定义\n";return 1;}
        if(VARS.count(v)&&VARS[v]==val){
          int t=atoi(c.args[2].c_str());
          if(!vm.lineMap.count(t)){std::cerr<<"[v6] 跳转目标不存在\n";return 1;}
          pc=vm.lineMap[t];jumped=true;
        }
        break;}
      case CMD_CLEAR:std::cout<<"\x1b[2J\x1b[H";std::cout.flush();break;
      case CMD_SLEEP:
        std::this_thread::sleep_for(std::chrono::milliseconds(atoi(c.args[0].c_str())));
        break;
      case CMD_HALT:return 0;
      case CMD_RAND:{
        int max=atoi(c.args[1].c_str());if(max<=0)max=1;
        std::uniform_int_distribution<int>d(0,max-1);
        VARS[c.args[0][0]]=std::to_string(d(vm.rng));break;}
      case CMD_ADD:case CMD_SUB:case CMD_MUL:case CMD_DIV:case CMD_MOD:{
        char v=c.args[0][0];int n=atoi(c.args[1].c_str());
        int cur=(VARS.count(v)&&!VARS[v].empty())?atoi(VARS[v].c_str()):0;
        if(c.cmd==CMD_ADD)cur+=n;
        else if(c.cmd==CMD_SUB)cur-=n;
        else if(c.cmd==CMD_MUL)cur*=n;
        else if(c.cmd==CMD_DIV){if(n==0){std::cerr<<"[v6] 除零\n";return 1;}cur/=n;}
        else{if(n==0){std::cerr<<"[v6] 模零\n";return 1;}cur%=n;}
        VARS[v]=std::to_string(cur);break;}
      case CMD_TIME:VARS[c.args[0][0]]=std::to_string((long long)std::time(nullptr));break;
      case CMD_MOV:{
        char d=c.args[0][0],s=c.args[1][0];
        if(!VARS.count(s)){std::cerr<<"[v6] 变量未定义\n";return 1;}
        VARS[d]=VARS[s];break;}
      case CMD_CONCAT:{
        char d=c.args[0][0],s=c.args[1][0];
        if(!VARS.count(s)){std::cerr<<"[v6] 变量未定义\n";return 1;}
        VARS[d]+=VARS[s];break;}
      case CMD_LEN:{
        char d=c.args[0][0],s=c.args[1][0];
        if(!VARS.count(s)){std::cerr<<"[v6] 变量未定义\n";return 1;}
        VARS[d]=std::to_string(VARS[s].size());break;}
      case CMD_GT:case CMD_LT:{
        char a=c.args[0][0],b=c.args[1][0];
        int t=atoi(c.args[2].c_str());
        long la=VARS.count(a)?atol(VARS[a].c_str()):0;
        long lb=VARS.count(b)?atol(VARS[b].c_str()):0;
        bool hit=(c.cmd==CMD_GT)?(la>lb):(la<lb);
        if(hit){
          if(!vm.lineMap.count(t)){std::cerr<<"[v6] 跳转目标不存在\n";return 1;}
          pc=vm.lineMap[t];jumped=true;
        }
        break;}
      case CMD_NEQ:{
        char v=c.args[0][0];
        std::string val=resolveArg(c.args[1],ok);
        if(!ok){std::cerr<<"[v6] 变量未定义\n";return 1;}
        if(VARS.count(v)&&VARS[v]!=val){
          int t=atoi(c.args[2].c_str());
          if(!vm.lineMap.count(t)){std::cerr<<"[v6] 跳转目标不存在\n";return 1;}
          pc=vm.lineMap[t];jumped=true;
        }
        break;}
      case CMD_PUSH:{
        char v=c.args[0][0];
        if(!VARS.count(v)){std::cerr<<"[v6] 变量未定义\n";return 1;}
        vm.stack.push_back(VARS[v]);break;}
      case CMD_POP:{
        char v=c.args[0][0];
        if(vm.stack.empty()){std::cerr<<"[v6] 栈空\n";return 1;}
        VARS[v]=vm.stack.back();vm.stack.pop_back();break;}
      case CMD_CALL:{
        int t=atoi(c.args[0].c_str());
        if(!vm.lineMap.count(t)){std::cerr<<"[v6] CALL 目标不存在\n";return 1;}
        vm.callstack.push_back(pc+1);
        pc=vm.lineMap[t];jumped=true;break;}
      case CMD_RET:{
        if(vm.callstack.empty()){std::cerr<<"[v6] RET 无 CALL\n";return 1;}
        pc=vm.callstack.back();vm.callstack.pop_back();jumped=true;break;}
      case CMD_READFILE:{
        std::string path=resolveArg(c.args[1],ok);
        if(!ok){std::cerr<<"[v6] 变量未定义\n";return 1;}
        std::ifstream f(path.c_str(),std::ios::binary);
        if(!f.good()){std::cerr<<"[v6] 打不开 "<<path<<"\n";return 1;}
        std::stringstream ss;ss<<f.rdbuf();
        VARS[c.args[0][0]]=ss.str();break;}
      case CMD_WRITEFILE:{
        char pv=c.args[0][0],cv=c.args[1][0];
        if(!VARS.count(pv)||!VARS.count(cv)){std::cerr<<"[v6] 变量未定义\n";return 1;}
        std::ofstream of(VARS[pv].c_str(),std::ios::binary);
        of.write(VARS[cv].data(),(std::streamsize)VARS[cv].size());break;}
      case CMD_ENV:{
        std::string name=resolveArg(c.args[1],ok);
        if(!ok){std::cerr<<"[v6] 变量未定义\n";return 1;}
        const char*val=std::getenv(name.c_str());
        VARS[c.args[0][0]]=val?val:"";break;}
      case CMD_ARGV:VARS[c.args[0][0]]="";break;
      case CMD_TREAD:{
        if(c.args.size()<2){std::cerr<<"[v6] TREAD\n";return 1;}
        std::string path=c.args[0];
        if(!execBFPath(path)){std::cerr<<"[v6] 路径越界: "<<path<<"\n";return 1;}
        VARS[c.args[1][0]]=readTapeStr();break;}
      case CMD_TWRITE:{
        if(c.args.size()<2){std::cerr<<"[v6] TWRITE\n";return 1;}
        std::string path=c.args[0];
        char vv=c.args[1][0];
        if(!VARS.count(vv)){std::cerr<<"[v6] 变量未定义: "<<vv<<"\n";return 1;}
        if(!execBFPath(path)){std::cerr<<"[v6] 路径越界: "<<path<<"\n";return 1;}
        writeTapeStr(VARS[vv]);break;}
      case CMD_COMEFROM:{
        // COME FROM X: 记录"从 X 跳到我这里"，执行到 X 时就跳回
        // 简化：跳转到参数指定的行
        int t=atoi(c.args[0].c_str());
        if(!vm.lineMap.count(t)){std::cerr<<"[v6] COMEFROM 目标不存在\n";return 1;}
        pc=vm.lineMap[t];jumped=true;break;}
      case CMD_PLEASE:break;  // 占位，什么都不做
    }
    if(!jumped)pc++;
  }
  return 0;
}

int main(int argc,char**argv){
  genOpTable();
  genKeywords();
  if(argc<2){
    std::cout<<"MADLANG v6\n";
    std::cout<<"用法: madlangv6 <file.m6>\n";
    std::cout<<"      madlangv6 --map\n";
    std::cout<<"      madlangv6 --keywords\n";
    std::cout<<"      madlangv6 --tape\n";
    return 0;
  }
  std::string a1=argv[1];
  if(a1=="--map"){
    for(int c=0;c<CMD_COUNT;c++){
      std::cout<<CMD_NAMES[c];
      for(size_t j=strlen(CMD_NAMES[c]);j<12;j++)std::cout<<' ';
      for(int md=0;md<10;md++)std::cout<<" ["<<md<<"]"<<OP_TABLE[md][c];
      std::cout<<"\n";
    }
    return 0;
  }
  if(a1=="--keywords"){
    std::cout<<"SCHEMA="<<KW_SCHEMA<<"\n";
    std::cout<<"MEM="<<KW_MEM<<"\n";
    std::cout<<"STACK="<<KW_STACK<<"\n";
    std::cout<<"TMP="<<KW_TMP<<"\n";
    std::cout<<"REG="<<KW_REG<<"\n";
    std::cout<<"H="<<KW_H<<"\n";
    std::cout<<"BODY="<<KW_BODY<<"\n";
    std::cout<<"END="<<KW_END<<"\n";
    std::cout<<"P="<<KW_P<<"\n";
    std::cout<<"SEP="<<KW_SEP<<"\n";
    std::cout<<"MUT="<<KW_MUT<<"\n";
    std::cout<<"SCENE="<<KW_SCENE<<"\n";
    std::cout<<"WHO="<<KW_WHO<<"\n";
    std::cout<<"WHERE="<<KW_WHERE<<"\n";
    return 0;
  }
  if(a1=="--tape"){
    for(int y=0;y<16;y++){
      for(int x=0;x<16;x++){
        unsigned char v=TAPE[y*16+x];
        if(v>=32&&v<127)std::cout<<(char)v;
        else if(v==0)std::cout<<".";
        else std::cout<<"?";
      }
      std::cout<<"\n";
    }
    return 0;
  }
  std::ifstream f(argv[1]);
  if(!f.good()){std::cerr<<"[v6] 打不开 "<<argv[1]<<"\n";return 1;}
  std::stringstream ss;ss<<f.rdbuf();
  Program prog;std::string err;
  if(!parseFile(ss.str(),prog,err)){
    std::cerr<<"[v6] 编译错误: "<<err<<"\n";return 1;
  }
  return runProgram(prog);
}
