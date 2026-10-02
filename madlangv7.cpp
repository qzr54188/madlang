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
  CMD_DIM,CMD_DECL,CMD_BIND,CMD_SALT,CMD_CHK,CMD_COMMIT,CMD_REFRESH,CMD_UNSET,
  CMD_STR_LEN,CMD_STR_AT,CMD_STR_SUB,CMD_STR_FIND,CMD_STR_SPLIT,
  CMD_STR_UPPER,CMD_STR_LOWER,CMD_STR_TRIM,CMD_STR_REPL,CMD_STR_STARTS,CMD_STR_ENDS,
  CMD_CHR,CMD_ORD,
  CMD_LST_NEW,CMD_LST_PUSH,CMD_LST_POP,CMD_LST_GET,CMD_LST_SET,CMD_LST_LEN,CMD_LST_DEL,CMD_LST_INS,
  CMD_SIN,CMD_COS,CMD_TAN,CMD_SQRT,CMD_POW,CMD_LOG,CMD_EXP,CMD_ABS,
  CMD_FLOOR,CMD_CEIL,CMD_ROUND,CMD_MIN,CMD_MAX,
  CMD_GOTO,CMD_COLOR,CMD_BGCOLOR,CMD_CLR_LINE,CMD_CLR_SCREEN,
  CMD_DRAW_CH,CMD_DRAW_STR,CMD_DRAW_HLINE,CMD_DRAW_VLINE,CMD_DRAW_BOX,
  CMD_F_OPEN,CMD_F_CLOSE,CMD_F_READ,CMD_F_READLN,CMD_F_WRITE,
  CMD_F_SEEK,CMD_F_TELL,CMD_DIR_LIST,CMD_FILE_DEL,CMD_FILE_REN,
  CMD_CMP,CMD_CMP_IMM,CMD_JE,CMD_JNE,CMD_JL,CMD_JG,CMD_JZ,CMD_JNZ,
  CMD_NOP,CMD_LOAD,CMD_STORE,CMD_OUT,CMD_OUT_LN,
  CMD_TIME_MS,CMD_TIME_NS,CMD_TIME_FMT,CMD_SLEEP_MS,
  CMD_COUNT};
static const char* CMD_NAMES[CMD_COUNT]={
  "PRINT","PRINTLN","SET","INPUT","JUMP","IFEQ","CLEAR","SLEEP","HALT","RAND",
  "ADD","SUB","READFILE","TIME","MOV","CONCAT","LEN","MUL","DIV","MOD",
  "GT","LT","NEQ","PUSH","POP","CALL","RET","WRITEFILE","ENV","ARGV",
  "TREAD","TWRITE","COMEFROM","PLEASE",
  "DIM","DECL","BIND","SALT","CHK","COMMIT","REFRESH","UNSET",
  "STR_LEN","STR_AT","STR_SUB","STR_FIND","STR_SPLIT",
  "STR_UPPER","STR_LOWER","STR_TRIM","STR_REPL","STR_STARTS","STR_ENDS",
  "CHR","ORD",
  "LST_NEW","LST_PUSH","LST_POP","LST_GET","LST_SET","LST_LEN","LST_DEL","LST_INS",
  "SIN","COS","TAN","SQRT","POW","LOG","EXP","ABS","FLOOR","CEIL","ROUND","MIN","MAX",
  "GOTO","COLOR","BGCOLOR","CLR_LINE","CLR_SCREEN",
  "DRAW_CH","DRAW_STR","DRAW_HLINE","DRAW_VLINE","DRAW_BOX",
  "F_OPEN","F_CLOSE","F_READ","F_READLN","F_WRITE","F_SEEK","F_TELL",
  "DIR_LIST","FILE_DEL","FILE_REN",
  "CMP","CMP_IMM","JE","JNE","JL","JG","JZ","JNZ","NOP","LOAD","STORE","OUT","OUT_LN",
  "TIME_MS","TIME_NS","TIME_FMT","SLEEP_MS"};

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
    lastMode=mode;prevSig=sigilVal;prevArgc=(int)args.size();
    blockCount++;
    while(off<(int)lines.size()&&lines[off].empty())off++;
    lastStack=rb.stack;lastTmp=rb.tmp;
  }
  return true;
}

static std::map<char,std::string> VARS;

// ============ v7: 命名变量系统 ============
struct VarSlot {
  std::string name;    // 原样名字
  int type;            // 0=TEXT 1=NUM 2=LIST
  int checksum;        // 提交的校验和
  int salt;            // 槽位²
  int useCount;        // 使用次数
  bool dimmed, declared, bound, salted, chked, committed;
  VarSlot() : type(0), checksum(0), salt(0), useCount(0),
              dimmed(false), declared(false), bound(false),
              salted(false), chked(false), committed(false) {}
};
static std::map<int, VarSlot> SLOTS;
static std::map<std::string, int> NAME2SLOT;
static std::map<int, std::string> SLOT_VAL;
static int NEXT_SLOT = 1;
static const int USE_LIMIT = 100;
static std::string strToLower(const std::string& in) {
  std::string o = in;
  for (size_t i = 0; i < o.size(); i++)
    if (o[i] >= 'A' && o[i] <= 'Z') o[i] += 32;
  return o;
}
static int b64len(const std::string& s) {
  // base64 解码后的字节数（估算，忽略填充）
  int n = (int)s.size();
  while (n > 0 && s[n-1] == '=') n--;
  return n * 3 / 4;
}
static bool isPrimeN(int n) {
  if (n < 2) return false;
  for (int i = 2; i*i <= n; i++) if (n % i == 0) return false;
  return true;
}
static std::string slotErr(int slot) {
  char b[64]; snprintf(b, sizeof(b), "槽位 %d", slot); return b;
}
static bool requireSlot(int slot, bool committed, std::string& err) {
  if (!SLOTS.count(slot)) { err = slotErr(slot) + " 未 DIM"; return false; }
  if (committed && !SLOTS[slot].committed) { err = slotErr(slot) + " 未 COMMIT"; return false; }
  return true;
}
static std::map<char, std::vector<std::string> > LISTS;
static std::map<char, std::ifstream*> FIN;
static std::map<char, std::ofstream*> FOUT;
static int g_cmpFlag = 0;
static std::string g_acc;

static std::string argGet(const std::string& a, bool& ok) {
  ok = true;
  if (a.size() == 1 && a[0] >= 'A' && a[0] <= 'Z') {
    if (!VARS.count(a[0])) { ok = false; return ""; }
    return VARS[a[0]];
  }
  bool _allD = !a.empty();
  for (size_t i = 0; i < a.size(); i++) if (a[i] < '0' || a[i] > '9') { _allD = false; break; }
  if (_allD) return a;
  std::string dec = b64decode(b64decode(a));
  if (dec.size() >= 2 && dec[0] == ':') {
    std::string n = strToLower(dec.substr(1));
    if (!NAME2SLOT.count(n)) { ok = false; return ""; }
    int sl = NAME2SLOT[n];
    if (!SLOTS.count(sl) || !SLOTS[sl].committed) { ok = false; return ""; }
    if (SLOTS[sl].useCount >= USE_LIMIT) { ok = false; return ""; }
    SLOTS[sl].useCount++;
    return SLOT_VAL.count(sl) ? SLOT_VAL[sl] : "";
  }
  return dec;
}

static bool argSet(const std::string& a, const std::string& val) {
  if (a.size() == 1 && a[0] >= 'A' && a[0] <= 'Z') {
    VARS[a[0]] = val;
    return true;
  }
  std::string dec = b64decode(b64decode(a));
  if (dec.size() >= 2 && dec[0] == ':') {
    std::string n = strToLower(dec.substr(1));
    if (!NAME2SLOT.count(n)) return false;
    int sl = NAME2SLOT[n];
    if (!SLOTS.count(sl) || !SLOTS[sl].committed) return false;
    SLOT_VAL[sl] = val;
    return true;
  }
  return false;
}

static std::string simpleArg(const std::string& a) {
  if (a.size() == 1 && a[0] >= 'A' && a[0] <= 'Z') {
    if (VARS.count(a[0])) return VARS[a[0]];
    return a;
  }
  bool _allD = !a.empty();
  for (size_t i = 0; i < a.size(); i++) if (a[i] < '0' || a[i] > '9') { _allD = false; break; }
  if (_allD) return a;
  std::string dec = b64decode(b64decode(a));
  if (dec.size() >= 2 && dec[0] == ':') {
    std::string n = strToLower(dec.substr(1));
    if (NAME2SLOT.count(n)) {
      int sl = NAME2SLOT[n];
      if (SLOTS.count(sl) && SLOTS[sl].committed && SLOT_VAL.count(sl)) return SLOT_VAL[sl];
    }
  }
  return dec;
}

static int hex2int(const std::string& h) {
  int v = 0;
  for (size_t i = 0; i < h.size(); i++) {
    v <<= 4;
    char c = h[i];
    if (c >= '0' && c <= '9') v += c - '0';
    else if (c >= 'A' && c <= 'F') v += c - 'A' + 10;
    else if (c >= 'a' && c <= 'f') v += c - 'a' + 10;
  }
  return v & 0xFF;
}
// Unlambda: 变量访问必须有 . 前缀
static bool g_unlambdaStrict=true;

static std::string resolveArg(const std::string&a,bool&ok){
  ok=true;
  if (a.size() == 1 && a[0] >= 'A' && a[0] <= 'Z') {
    if (!VARS.count(a[0])) { ok = false; return ""; }
    return VARS[a[0]];
  }
  bool _allD = !a.empty();
  for (size_t i = 0; i < a.size(); i++) if (a[i] < '0' || a[i] > '9') { _allD = false; break; }
  if (_allD) return a;
  std::string dec=b64decode(b64decode(a));
  // 命名变量 :name
  if (dec.size() >= 2 && dec[0] == ':') {
    std::string name = strToLower(dec.substr(1));
    if (!NAME2SLOT.count(name)) { ok=false; return ""; }
    int slot = NAME2SLOT[name];
    if (!SLOTS.count(slot) || !SLOTS[slot].committed) { ok=false; return ""; }
    if (SLOTS[slot].useCount >= USE_LIMIT) { ok=false; return ""; }
    SLOTS[slot].useCount++;
    if (!SLOT_VAL.count(slot)) { ok=false; return ""; }
    return SLOT_VAL[slot];
  }
  // Unlambda 前缀
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
        if(c.args.size()<2){std::cerr<<"[v6] SET\n";return 1;}
        std::string v=resolveArg(c.args[1],ok);
        if(!ok){std::cerr<<"[v6] 值未定义\n";return 1;}
        std::string dst;
        if(c.args[0].size()==1&&c.args[0][0]>='A'&&c.args[0][0]<='Z'){dst=c.args[0];}
        else dst=b64decode(b64decode(c.args[0]));
        if(dst.size()>=2&&dst[0]==':'){
          std::string name=strToLower(dst.substr(1));
          if(!NAME2SLOT.count(name)){std::cerr<<"[v7] 变量未注册: "<<name<<"\n";return 1;}
          int slot=NAME2SLOT[name];
          if(!SLOTS[slot].committed){std::cerr<<"[v7] 未 COMMIT\n";return 1;}
          SLOT_VAL[slot]=v;
        } else if(dst.size()==1){
          VARS[dst[0]]=v;
        } else { std::cerr<<"[v7] 变量名非法\n"; return 1; }
        break;}
      case CMD_INPUT:{
        if(c.args.empty()){std::cerr<<"[v6] INPUT\n";return 1;}
        std::string s;std::getline(std::cin,s);
        if(!argSet(c.args[0],s)){std::cerr<<"[v7] INPUT 变量非法\n";return 1;}
        break;}
      case CMD_JUMP:{
        int t=atoi(c.args[0].c_str());
        if(!vm.lineMap.count(t)){std::cerr<<"[v6] 跳转目标不存在\n";return 1;}
        pc=vm.lineMap[t];jumped=true;break;}
      case CMD_IFEQ:{
        if(c.args.size()<3){std::cerr<<"[v6] IFEQ\n";return 1;}
        bool o2; std::string cur=argGet(c.args[0],o2);
        std::string val=resolveArg(c.args[1],ok);
        if(!ok){std::cerr<<"[v6] 变量未定义\n";return 1;}
        if(o2&&cur==val){
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
        argSet(c.args[0],std::to_string(d(vm.rng)));break;}
      case CMD_ADD:case CMD_SUB:case CMD_MUL:case CMD_DIV:case CMD_MOD:{
        bool o2; std::string curS=argGet(c.args[0],o2);
        int n=atoi(c.args[1].c_str());
        int cur=curS.empty()?0:atoi(curS.c_str());
        if(c.cmd==CMD_ADD)cur+=n;
        else if(c.cmd==CMD_SUB)cur-=n;
        else if(c.cmd==CMD_MUL)cur*=n;
        else if(c.cmd==CMD_DIV){if(n==0){std::cerr<<"[v6] 除零\n";return 1;}cur/=n;}
        else{if(n==0){std::cerr<<"[v6] 模零\n";return 1;}cur%=n;}
        argSet(c.args[0],std::to_string(cur));break;}
      case CMD_TIME:argSet(c.args[0],std::to_string((long long)std::time(nullptr)));break;
      case CMD_MOV:{
        bool o2; std::string sv=argGet(c.args[1],o2);
        if(!o2){std::cerr<<"[v6] 变量未定义\n";return 1;}
        argSet(c.args[0],sv);break;}
      case CMD_CONCAT:{
        bool o2; std::string sv=argGet(c.args[1],o2);
        if(!o2){std::cerr<<"[v6] 变量未定义\n";return 1;}
        bool o3; std::string dv=argGet(c.args[0],o3);
        argSet(c.args[0],dv+sv);break;}
      case CMD_LEN:{
        bool o2; std::string sv=argGet(c.args[1],o2);
        if(!o2){std::cerr<<"[v6] 变量未定义\n";return 1;}
        argSet(c.args[0],std::to_string(sv.size()));break;}
      case CMD_GT:case CMD_LT:{
        bool o2,o3; std::string av=argGet(c.args[0],o2); std::string bv=argGet(c.args[1],o3);
        int t=atoi(c.args[2].c_str());
        long la=atol(av.c_str()); long lb=atol(bv.c_str());
        bool hit=(c.cmd==CMD_GT)?(la>lb):(la<lb);
        if(hit){
          if(!vm.lineMap.count(t)){std::cerr<<"[v6] 跳转目标不存在\n";return 1;}
          pc=vm.lineMap[t];jumped=true;
        }
        break;}
      case CMD_NEQ:{
        bool o2; std::string cur=argGet(c.args[0],o2);
        std::string val=resolveArg(c.args[1],ok);
        if(!ok){std::cerr<<"[v6] 变量未定义\n";return 1;}
        if(o2&&cur!=val){
          int t=atoi(c.args[2].c_str());
          if(!vm.lineMap.count(t)){std::cerr<<"[v6] 跳转目标不存在\n";return 1;}
          pc=vm.lineMap[t];jumped=true;
        }
        break;}
      case CMD_PUSH:{
        bool o2; std::string cur=argGet(c.args[0],o2);
        if(!o2){std::cerr<<"[v6] 变量未定义\n";return 1;}
        vm.stack.push_back(cur);break;}
      case CMD_POP:{
        if(vm.stack.empty()){std::cerr<<"[v6] 栈空\n";return 1;}
        argSet(c.args[0],vm.stack.back());vm.stack.pop_back();break;}
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
        argSet(c.args[0],ss.str());break;}
      case CMD_WRITEFILE:{
        bool o2,o3; std::string pv=argGet(c.args[0],o2); std::string cv=argGet(c.args[1],o3);
        if(!o2||!o3){std::cerr<<"[v6] 变量未定义\n";return 1;}
        std::ofstream of(pv.c_str(),std::ios::binary);
        of.write(cv.data(),(std::streamsize)cv.size());break;}
      case CMD_ENV:{
        std::string name=resolveArg(c.args[1],ok);
        if(!ok){std::cerr<<"[v6] 变量未定义\n";return 1;}
        const char*val=std::getenv(name.c_str());
        argSet(c.args[0],val?val:"");break;}
      case CMD_ARGV:argSet(c.args[0],"");break;
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
      case CMD_PLEASE:break;
      case CMD_STR_LEN:{bool o;std::string v=argGet(c.args[1],o);if(!o){std::cerr<<"[v7] undef\n";return 1;}argSet(c.args[0],std::to_string(v.size()));break;}
      case CMD_STR_AT:{bool o;std::string v=argGet(c.args[1],o);int i=atoi(simpleArg(c.args[2]).c_str());if(!o){std::cerr<<"[v7] undef\n";return 1;}if(i<0||i>=(int)v.size()){std::cerr<<"[v7] oob\n";return 1;}argSet(c.args[0],std::string(1,v[i]));break;}
      case CMD_STR_SUB:{bool o;std::string v=argGet(c.args[1],o);int st=atoi(simpleArg(c.args[2]).c_str());int ln=atoi(simpleArg(c.args[3]).c_str());if(!o){std::cerr<<"[v7] undef\n";return 1;}if(st<0)st=0;if(st>(int)v.size())st=(int)v.size();if(st+ln>(int)v.size())ln=(int)v.size()-st;argSet(c.args[0],v.substr(st,ln));break;}
      case CMD_STR_FIND:{bool o;std::string v=argGet(c.args[1],o);bool o2;std::string nd=argGet(c.args[2],o2);if(!o){std::cerr<<"[v7] undef\n";return 1;}size_t k=v.find(nd);argSet(c.args[0],k==std::string::npos?"-1":std::to_string((int)k));break;}
      case CMD_STR_SPLIT:{bool o;std::string v=argGet(c.args[1],o);bool o2;std::string sp=argGet(c.args[2],o2);if(!o){std::cerr<<"[v7] undef\n";return 1;}size_t k=v.find(sp);argSet(c.args[0],k==std::string::npos?v:v.substr(0,k));break;}
      case CMD_STR_UPPER:{bool o;std::string v=argGet(c.args[1],o);if(!o){std::cerr<<"[v7] undef\n";return 1;}for(size_t i=0;i<v.size();i++)if(v[i]>='a'&&v[i]<='z')v[i]-=32;argSet(c.args[0],v);break;}
      case CMD_STR_LOWER:{bool o;std::string v=argGet(c.args[1],o);if(!o){std::cerr<<"[v7] undef\n";return 1;}for(size_t i=0;i<v.size();i++)if(v[i]>='A'&&v[i]<='Z')v[i]+=32;argSet(c.args[0],v);break;}
      case CMD_STR_TRIM:{bool o;std::string v=argGet(c.args[1],o);if(!o){std::cerr<<"[v7] undef\n";return 1;}size_t a=0,b=v.size();while(a<b&&(v[a]==' '||v[a]=='\t'||v[a]=='\n'))a++;while(b>a&&(v[b-1]==' '||v[b-1]=='\t'||v[b-1]=='\n'))b--;argSet(c.args[0],v.substr(a,b-a));break;}
      case CMD_STR_REPL:{bool o;std::string v=argGet(c.args[1],o);bool o2;std::string fr=argGet(c.args[2],o2);bool o3;std::string to=argGet(c.args[3],o3);if(!o){std::cerr<<"[v7] undef\n";return 1;}size_t k=0;while((k=v.find(fr,k))!=std::string::npos){v.replace(k,fr.size(),to);k+=to.size();}argSet(c.args[0],v);break;}
      case CMD_STR_STARTS:{bool o;std::string v=argGet(c.args[1],o);bool o2;std::string pfx=argGet(c.args[2],o2);if(!o){std::cerr<<"[v7] undef\n";return 1;}argSet(c.args[0],(v.compare(0,pfx.size(),pfx)==0)?"1":"0");break;}
      case CMD_STR_ENDS:{bool o;std::string v=argGet(c.args[1],o);bool o2;std::string sfx=argGet(c.args[2],o2);if(!o){std::cerr<<"[v7] undef\n";return 1;}argSet(c.args[0],(v.size()>=sfx.size()&&v.compare(v.size()-sfx.size(),sfx.size(),sfx)==0)?"1":"0");break;}
      case CMD_CHR:{int n=atoi(simpleArg(c.args[1]).c_str());argSet(c.args[0],std::string(1,(char)(n&0xFF)));break;}
      case CMD_ORD:{bool o;std::string v=argGet(c.args[1],o);if(!o){std::cerr<<"[v7] undef\n";return 1;}argSet(c.args[0],v.empty()?"0":std::to_string((int)(unsigned char)v[0]));break;}
      case CMD_LST_NEW:{std::string n=simpleArg(c.args[0]);if(n.size()!=1){std::cerr<<"[v7] lst name\n";return 1;}LISTS[n[0]].clear();break;}
      case CMD_LST_PUSH:{std::string n=simpleArg(c.args[0]);bool o;std::string v=argGet(c.args[1],o);if(n.size()!=1||!o){std::cerr<<"[v7] lst\n";return 1;}LISTS[n[0]].push_back(v);break;}
      case CMD_LST_POP:{std::string n=simpleArg(c.args[1]);if(n.size()!=1||LISTS[n[0]].empty()){std::cerr<<"[v7] lst\n";return 1;}std::string v=LISTS[n[0]].back();LISTS[n[0]].pop_back();argSet(c.args[0],v);break;}
      case CMD_LST_GET:{std::string n=simpleArg(c.args[1]);int i=atoi(simpleArg(c.args[2]).c_str());if(n.size()!=1||i<0||i>=(int)LISTS[n[0]].size()){std::cerr<<"[v7] lst oob\n";return 1;}argSet(c.args[0],LISTS[n[0]][i]);break;}
      case CMD_LST_SET:{std::string n=simpleArg(c.args[0]);int i=atoi(simpleArg(c.args[1]).c_str());bool o;std::string v=argGet(c.args[2],o);if(n.size()!=1){std::cerr<<"[v7] lst name\n";return 1;}if(i<0)LISTS[n[0]].insert(LISTS[n[0]].begin(),v);else{if(i>=(int)LISTS[n[0]].size())LISTS[n[0]].resize(i+1);LISTS[n[0]][i]=v;}break;}
      case CMD_LST_LEN:{std::string n=simpleArg(c.args[1]);if(n.size()!=1){std::cerr<<"[v7] lst name\n";return 1;}argSet(c.args[0],std::to_string(LISTS[n[0]].size()));break;}
      case CMD_LST_DEL:{std::string n=simpleArg(c.args[0]);int i=atoi(simpleArg(c.args[1]).c_str());if(n.size()==1&&i>=0&&i<(int)LISTS[n[0]].size())LISTS[n[0]].erase(LISTS[n[0]].begin()+i);break;}
      case CMD_LST_INS:{std::string n=simpleArg(c.args[0]);int i=atoi(simpleArg(c.args[1]).c_str());bool o;std::string v=argGet(c.args[2],o);if(n.size()!=1){std::cerr<<"[v7] lst name\n";return 1;}if(i<0)i=0;if(i>(int)LISTS[n[0]].size())i=(int)LISTS[n[0]].size();LISTS[n[0]].insert(LISTS[n[0]].begin()+i,v);break;}
      case CMD_SIN:case CMD_COS:case CMD_TAN:case CMD_SQRT:case CMD_LOG:case CMD_EXP:case CMD_ABS:case CMD_FLOOR:case CMD_CEIL:case CMD_ROUND:{double x=atof(simpleArg(c.args[1]).c_str());double r=0;if(c.cmd==CMD_SIN)r=std::sin(x);else if(c.cmd==CMD_COS)r=std::cos(x);else if(c.cmd==CMD_TAN)r=std::tan(x);else if(c.cmd==CMD_SQRT)r=std::sqrt(x);else if(c.cmd==CMD_LOG)r=std::log(x);else if(c.cmd==CMD_EXP)r=std::exp(x);else if(c.cmd==CMD_ABS)r=std::fabs(x);else if(c.cmd==CMD_FLOOR)r=std::floor(x);else if(c.cmd==CMD_CEIL)r=std::ceil(x);else r=std::floor(x+0.5);char b[64];snprintf(b,sizeof(b),"%g",r);argSet(c.args[0],b);break;}
      case CMD_POW:{double a=atof(simpleArg(c.args[1]).c_str()),b=atof(simpleArg(c.args[2]).c_str());char buf[64];snprintf(buf,sizeof(buf),"%g",std::pow(a,b));argSet(c.args[0],buf);break;}
      case CMD_MIN:{double a=atof(simpleArg(c.args[1]).c_str()),b=atof(simpleArg(c.args[2]).c_str());char buf[64];snprintf(buf,sizeof(buf),"%g",a<b?a:b);argSet(c.args[0],buf);break;}
      case CMD_MAX:{double a=atof(simpleArg(c.args[1]).c_str()),b=atof(simpleArg(c.args[2]).c_str());char buf[64];snprintf(buf,sizeof(buf),"%g",a>b?a:b);argSet(c.args[0],buf);break;}
      case CMD_GOTO:{int x=atoi(simpleArg(c.args[0]).c_str()),y=atoi(simpleArg(c.args[1]).c_str());std::cout<<"\x1b["<<y<<";"<<x<<"H";std::cout.flush();break;}
      case CMD_COLOR:{int n=atoi(simpleArg(c.args[0]).c_str());std::cout<<"\x1b[38;5;"<<n<<"m";std::cout.flush();break;}
      case CMD_BGCOLOR:{int n=atoi(simpleArg(c.args[0]).c_str());std::cout<<"\x1b[48;5;"<<n<<"m";std::cout.flush();break;}
      case CMD_CLR_LINE:{std::cout<<"\x1b[2K";std::cout.flush();break;}
      case CMD_CLR_SCREEN:{std::cout<<"\x1b[2J\x1b[H";std::cout.flush();break;}
      case CMD_DRAW_CH:{bool o;std::string v=argGet(c.args[0],o);if(o)std::cout<<v;std::cout.flush();break;}
      case CMD_DRAW_STR:{bool o;std::string v=argGet(c.args[0],o);std::cout<<v;std::cout.flush();break;}
      case CMD_DRAW_HLINE:{int n=atoi(simpleArg(c.args[0]).c_str());for(int i=0;i<n;i++)std::cout<<"-";std::cout.flush();break;}
      case CMD_DRAW_VLINE:{int n=atoi(simpleArg(c.args[0]).c_str());for(int i=0;i<n;i++)std::cout<<"|\n";std::cout.flush();break;}
      case CMD_DRAW_BOX:{int w=atoi(simpleArg(c.args[0]).c_str()),h=atoi(simpleArg(c.args[1]).c_str());for(int x=0;x<w;x++)std::cout<<"-";std::cout<<"\n";for(int y=0;y<h;y++){std::cout<<"|";for(int x=0;x<w-2;x++)std::cout<<" ";std::cout<<"|\n";}for(int x=0;x<w;x++)std::cout<<"-";std::cout<<"\n";std::cout.flush();break;}
      case CMD_F_OPEN:{std::string fv=simpleArg(c.args[0]);bool o;std::string path=argGet(c.args[1],o);std::string mode=c.args.size()>2?simpleArg(c.args[2]):"r";if(fv.size()!=1){std::cerr<<"[v7] fh\n";return 1;}char fc=fv[0];if(FIN.count(fc)){FIN[fc]->close();delete FIN[fc];FIN.erase(fc);}if(FOUT.count(fc)){FOUT[fc]->close();delete FOUT[fc];FOUT.erase(fc);}if(mode=="r"){FIN[fc]=new std::ifstream(path.c_str(),std::ios::binary);}else{FOUT[fc]=new std::ofstream(path.c_str(),std::ios::binary|(mode=="a"?std::ios::app:std::ios::trunc));}break;}
      case CMD_F_CLOSE:{std::string fv=simpleArg(c.args[0]);if(fv.size()!=1)break;char fc=fv[0];if(FIN.count(fc)){FIN[fc]->close();delete FIN[fc];FIN.erase(fc);}if(FOUT.count(fc)){FOUT[fc]->close();delete FOUT[fc];FOUT.erase(fc);}break;}
      case CMD_F_READ:{std::string fv=simpleArg(c.args[1]);if(fv.size()!=1||!FIN.count(fv[0])){std::cerr<<"[v7] fin\n";return 1;}std::stringstream ss;ss<<FIN[fv[0]]->rdbuf();argSet(c.args[0],ss.str());break;}
      case CMD_F_READLN:{std::string fv=simpleArg(c.args[1]);if(fv.size()!=1||!FIN.count(fv[0])){std::cerr<<"[v7] fin\n";return 1;}std::string ln;std::getline(*FIN[fv[0]],ln);argSet(c.args[0],ln);break;}
      case CMD_F_WRITE:{std::string fv=simpleArg(c.args[0]);bool o;std::string t=argGet(c.args[1],o);if(fv.size()!=1||!FOUT.count(fv[0])){std::cerr<<"[v7] fout\n";return 1;}*FOUT[fv[0]]<<t;break;}
      case CMD_F_SEEK:{std::string fv=simpleArg(c.args[0]);int pos=atoi(simpleArg(c.args[1]).c_str());if(fv.size()!=1||!FIN.count(fv[0]))break;FIN[fv[0]]->seekg(pos);break;}
      case CMD_F_TELL:{std::string fv=simpleArg(c.args[1]);if(fv.size()!=1||!FIN.count(fv[0]))break;argSet(c.args[0],std::to_string((long long)FIN[fv[0]]->tellg()));break;}
      case CMD_DIR_LIST:{break;}
      case CMD_FILE_DEL:{bool o;std::string p=argGet(c.args[0],o);std::remove(p.c_str());break;}
      case CMD_FILE_REN:{bool o;std::string a=argGet(c.args[0],o);bool o2;std::string b=argGet(c.args[1],o2);std::rename(a.c_str(),b.c_str());break;}
      case CMD_CMP:{bool o;std::string a=argGet(c.args[0],o);bool o2;std::string b=argGet(c.args[1],o2);if(a<b)g_cmpFlag=-1;else if(a>b)g_cmpFlag=1;else g_cmpFlag=0;break;}
      case CMD_CMP_IMM:{bool o;std::string a=argGet(c.args[0],o);long x=atol(a.c_str());long y=atol(simpleArg(c.args[1]).c_str());g_cmpFlag=(x<y)?-1:(x>y)?1:0;break;}
      case CMD_JE:{if(g_cmpFlag==0){int t=atoi(c.args[0].c_str());if(!vm.lineMap.count(t)){std::cerr<<"[v7] je\n";return 1;}pc=vm.lineMap[t];jumped=true;}break;}
      case CMD_JNE:{if(g_cmpFlag!=0){int t=atoi(c.args[0].c_str());if(!vm.lineMap.count(t)){std::cerr<<"[v7] jne\n";return 1;}pc=vm.lineMap[t];jumped=true;}break;}
      case CMD_JL:{if(g_cmpFlag<0){int t=atoi(c.args[0].c_str());if(!vm.lineMap.count(t)){std::cerr<<"[v7] jl\n";return 1;}pc=vm.lineMap[t];jumped=true;}break;}
      case CMD_JG:{if(g_cmpFlag>0){int t=atoi(c.args[0].c_str());if(!vm.lineMap.count(t)){std::cerr<<"[v7] jg\n";return 1;}pc=vm.lineMap[t];jumped=true;}break;}
      case CMD_JZ:{bool o;std::string v=argGet(c.args[0],o);if(atol(v.c_str())==0){int t=atoi(c.args[1].c_str());if(!vm.lineMap.count(t)){std::cerr<<"[v7] jz\n";return 1;}pc=vm.lineMap[t];jumped=true;}break;}
      case CMD_JNZ:{bool o;std::string v=argGet(c.args[0],o);if(atol(v.c_str())!=0){int t=atoi(c.args[1].c_str());if(!vm.lineMap.count(t)){std::cerr<<"[v7] jnz\n";return 1;}pc=vm.lineMap[t];jumped=true;}break;}
      case CMD_NOP:break;
      case CMD_LOAD:{bool o;g_acc=argGet(c.args[0],o);break;}
      case CMD_STORE:{argSet(c.args[0],g_acc);break;}
      case CMD_OUT:{bool o;std::string v=argGet(c.args[0],o);std::cout<<v;std::cout.flush();break;}
      case CMD_OUT_LN:{bool o;std::string v=argGet(c.args[0],o);std::cout<<v<<"\n";std::cout.flush();break;}
      case CMD_TIME_MS:{long long ms=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();argSet(c.args[0],std::to_string(ms));break;}
      case CMD_TIME_NS:{long long ns=std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::system_clock::now().time_since_epoch()).count();argSet(c.args[0],std::to_string(ns));break;}
      case CMD_TIME_FMT:{time_t t=std::time(NULL);char buf[64];strftime(buf,sizeof(buf),"%Y-%m-%d %H:%M:%S",localtime(&t));argSet(c.args[0],buf);break;}
      case CMD_SLEEP_MS:{int n=atoi(simpleArg(c.args[0]).c_str());std::this_thread::sleep_for(std::chrono::milliseconds(n));break;}
      case CMD_DIM:{
        if(c.args.size()<1){std::cerr<<"[v7] DIM\n";return 1;}
        int slot=atoi(c.args[0].c_str());
        if(slot!=NEXT_SLOT){std::cerr<<"[v7] 槽位必须连续，期望 "<<NEXT_SLOT<<" 实际 "<<slot<<"\n";return 1;}
        if(SLOTS.count(slot)){std::cerr<<"[v7] 槽位已占\n";return 1;}
        SLOTS[slot]=VarSlot();SLOTS[slot].dimmed=true;NEXT_SLOT++;
        break;}
      case CMD_DECL:{
        if(c.args.size()<2){std::cerr<<"[v7] DECL\n";return 1;}
        int slot=atoi(c.args[0].c_str());std::string t=c.args[1];
        if(!SLOTS.count(slot)||!SLOTS[slot].dimmed){std::cerr<<"[v7] 未 DIM\n";return 1;}
        if(SLOTS[slot].declared){std::cerr<<"[v7] 已 DECL\n";return 1;}
        if(t!="T"&&t!="N"&&t!="L"){std::cerr<<"[v7] 类型必须 T/N/L\n";return 1;}
        SLOTS[slot].type=(t=="T")?0:(t=="N")?1:2;SLOTS[slot].declared=true;
        break;}
      case CMD_BIND:{
        if(c.args.size()<2){std::cerr<<"[v7] BIND\n";return 1;}
        int slot=atoi(c.args[0].c_str());
        if(!SLOTS.count(slot)||!SLOTS[slot].declared){std::cerr<<"[v7] 未 DECL\n";return 1;}
        if(SLOTS[slot].bound){std::cerr<<"[v7] 已 BIND\n";return 1;}
        std::string nm=b64decode(b64decode(c.args[1]));
        if(nm.empty()){std::cerr<<"[v7] 名字空\n";return 1;}
        int bl=(int)nm.size();
        if(!isPrimeN(bl)){std::cerr<<"[v7] 名字长度 "<<bl<<" 不是质数\n";return 1;}
        SLOTS[slot].name=nm;SLOTS[slot].bound=true;
        NAME2SLOT[strToLower(nm)]=slot;
        break;}
      case CMD_SALT:{
        if(c.args.size()<1){std::cerr<<"[v7] SALT\n";return 1;}
        int slot=atoi(c.args[0].c_str());
        if(!SLOTS.count(slot)||!SLOTS[slot].bound){std::cerr<<"[v7] 未 BIND\n";return 1;}
        SLOTS[slot].salt=slot*slot;SLOTS[slot].salted=true;
        break;}
      case CMD_CHK:{
        if(c.args.size()<2){std::cerr<<"[v7] CHK\n";return 1;}
        int slot=atoi(c.args[0].c_str());
        if(!SLOTS.count(slot)||!SLOTS[slot].salted){std::cerr<<"[v7] 未 SALT\n";return 1;}
        int got=hex2int(c.args[1]);
        int sum=0;const std::string&nm=SLOTS[slot].name;
        for(size_t i=0;i<nm.size();i++)sum+=(unsigned char)nm[i];
        int want=(sum+slot)&0xFF;
        if(got!=want){char b[64];snprintf(b,sizeof(b),"[v7] CHK 错: 期望 %02X 实际 %02X",want,got);std::cerr<<b<<"\n";return 1;}
        SLOTS[slot].checksum=got;SLOTS[slot].chked=true;
        break;}
      case CMD_COMMIT:{
        if(c.args.size()<1){std::cerr<<"[v7] COMMIT\n";return 1;}
        int slot=atoi(c.args[0].c_str());
        if(!SLOTS.count(slot)){std::cerr<<"[v7] 未 DIM\n";return 1;}
        VarSlot&vs=SLOTS[slot];
        if(!vs.dimmed||!vs.declared||!vs.bound||!vs.salted||!vs.chked){
          std::cerr<<"[v7] COMMIT 前置不全\n";return 1;}
        vs.committed=true;
        break;}
      case CMD_REFRESH:{
        if(c.args.size()<2){std::cerr<<"[v7] REFRESH\n";return 1;}
        int slot=atoi(c.args[0].c_str());
        if(!SLOTS.count(slot)||!SLOTS[slot].committed){std::cerr<<"[v7] 未 COMMIT\n";return 1;}
        int got=hex2int(c.args[1]);
        int want=(SLOTS[slot].checksum*7+SLOTS[slot].useCount)&0xFF;
        if(got!=want){char b[64];snprintf(b,sizeof(b),"[v7] REFRESH 错: 期望 %02X",want);std::cerr<<b<<"\n";return 1;}
        SLOTS[slot].useCount=0;
        break;}
      case CMD_UNSET:{
        if(c.args.size()<1){std::cerr<<"[v7] UNSET\n";return 1;}
        int slot=atoi(c.args[0].c_str());
        if(SLOTS.count(slot)&&SLOTS[slot].bound)NAME2SLOT.erase(strToLower(SLOTS[slot].name));
        SLOTS.erase(slot);SLOT_VAL.erase(slot);
        break;}
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
