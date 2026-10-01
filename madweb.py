#!/usr/bin/env python3
import http.server, socketserver, subprocess, os, tempfile, json, sys, shlex
from urllib.parse import urlparse, parse_qs, unquote

DIR = os.path.dirname(os.path.abspath(__file__))
MADLANG = os.path.join(DIR, "madlang")
MADLANGC = os.path.join(DIR, "madlangc")
MADCTL = os.path.join(DIR, "madctl")

HTML = r'''<!DOCTYPE html>
<html lang="zh"><head><meta charset="utf-8"><title>MADLANG Studio</title>
<meta name="viewport" content="width=device-width,initial-scale=1">
<style>
*{box-sizing:border-box}
body{font-family:ui-monospace,Menlo,monospace;background:#121212;color:#d0d0d0;margin:0;padding:14px}
h1{color:#ff7043;margin:0 0 6px;font-size:20px}
h3{color:#ff7043;margin:12px 0 6px;font-size:13px;text-transform:uppercase;letter-spacing:1px}
.tip{color:#777;font-size:11px;margin-bottom:10px}
button{background:#ff7043;color:#fff;border:0;padding:7px 14px;cursor:pointer;font-size:13px;margin:0 6px 6px 0;border-radius:3px;font-family:inherit}
button:hover{background:#ff5722}
button.g{background:#333}button.g:hover{background:#444}
button.sm{padding:4px 10px;font-size:12px}
select,input[type=text]{background:#1e1e1e;color:#d0d0d0;border:1px solid #333;padding:6px 10px;font-family:inherit;font-size:13px;margin:0 6px 6px 0;border-radius:3px}
input[type=text]{width:calc(100% - 100px)}
.tabs{display:flex;gap:4px;margin:14px 0 0;border-bottom:1px solid #333}
.tab{padding:8px 16px;background:#1a1a1a;color:#888;cursor:pointer;border:1px solid #333;border-bottom:none;border-radius:3px 3px 0 0;font-size:13px}
.tab.on{background:#0a0a0a;color:#ff7043}
.pane{display:none;padding-top:10px}
.pane.on{display:block}
textarea{width:100%;height:340px;background:#0a0a0a;color:#b0ffb0;border:1px solid #333;padding:10px;font-family:inherit;font-size:13px;line-height:1.5;resize:vertical}
pre{background:#0a0a0a;padding:12px;border:1px solid #333;min-height:160px;white-space:pre-wrap;word-break:break-all;font-size:12px;margin:0;overflow:auto;max-height:400px}
pre.err{border-color:#b03030}
pre.ok{border-color:#30a030}
table{border-collapse:collapse;width:100%;font-size:11px}
td,th{border:1px solid #333;padding:3px 6px;text-align:left}
th{background:#1e1e1e;color:#ff7043}
code{color:#ffcc80}
.badge{display:inline-block;background:#222;color:#aaa;padding:2px 6px;border-radius:2px;font-size:10px;margin-left:6px}
.wrap{display:grid;grid-template-columns:2fr 1fr;gap:14px;margin-bottom:14px}
@media(max-width:900px){.wrap{grid-template-columns:1fr}}
.qbar{margin:8px 0}
.qbar button{background:#2a2a2a;color:#ccc;font-size:11px;padding:4px 10px}
.qbar button:hover{background:#3a3a3a}
</style></head><body>
<h1>MADLANG Studio <span class="badge">v4</span></h1>
<div class="tip">&lt;行号&gt;|&lt;token&gt;|&lt;b64x2参数&gt; — 禁空格 | 按行号升序 | # 注释</div>
<div class="tabs">
<div class="tab on" onclick="tab('editor')">📝 编辑器</div>
<div class="tab" onclick="tab('term')">⌨ 终端</div>
</div>

<div id="pane-editor" class="pane on">
  <div>
    <button onclick="doRun()">▶ 运行</button>
    <button onclick="doCompile()">⚙ 编译成原生</button>
    <button onclick="doBundle()">📦 打包</button>
    <button class="g" onclick="doSave()">💾 保存</button>
    <select id="exs" onchange="loadEx()"><option value="">-- 载入示例 --</option></select>
    <button class="g" onclick="loadMap()">↻ 刷新清单</button>
  </div>
  <div class="wrap">
    <div><textarea id="src" spellcheck="false" placeholder="在这里写 .mgl 源码..."></textarea></div>
    <div><h3>混淆清单</h3><table id="map"></table></div>
  </div>
  <h3>输出 / 日志</h3>
  <pre id="out">[就绪]</pre>
</div>

<div id="pane-term" class="pane">
  <h3>执行命令（白名单）</h3>
  <div class="qbar">
    <button onclick="qcmd('madctl info')">madctl info</button>
    <button onclick="qcmd('madctl regen')">madctl regen</button>
    <button onclick="qcmd('madlang --mapping')">--mapping</button>
    <button onclick="qcmd('madlang --help')">--help</button>
    <button onclick="qcmd('ls -la')">ls -la</button>
    <button onclick="qcmd('ls examples/')">ls examples</button>
    <button onclick="qcmd('pwd')">pwd</button>
    <button onclick="qcmd('cc examples/hello.mgl')">cc hello</button>
  </div>
  <div>
    <input type="text" id="cmd" placeholder="输入命令，例如: madctl info" onkeydown="if(event.key==='Enter')termRun()">
    <button onclick="termRun()">▷ 执行</button>
  </div>
  <pre id="term-out">[终端就绪]</pre>
</div>

<script>
function tab(name){
  document.querySelectorAll('.tab').forEach(t=>t.classList.remove('on'));
  document.querySelectorAll('.pane').forEach(p=>p.classList.remove('on'));
  document.querySelectorAll('.tab').forEach(t=>{
    if(t.textContent.includes(name==='editor'?'编辑器':'终端')) t.classList.add('on');
  });
  document.getElementById('pane-'+name).classList.add('on');
}
async function loadMap(){
  const r=await fetch('/mapping');const d=await r.json();
  const t=document.getElementById('map');
  t.innerHTML='<tr><th>命令</th><th>token</th></tr>';
  d.cmds.forEach(c=>{const tr=document.createElement('tr');
    tr.innerHTML='<td>'+c.name+'</td><td><code>'+c.token+'</code></td>';t.appendChild(tr);});
}
async function loadExList(){
  const r=await fetch('/examples');const d=await r.json();
  const s=document.getElementById('exs');
  d.examples.forEach(n=>{const o=document.createElement('option');o.value=n;o.textContent=n;s.appendChild(o);});
}
async function loadEx(){
  const n=document.getElementById('exs').value; if(!n)return;
  const r=await fetch('/example/'+n);const d=await r.json();
  if(d.ok){document.getElementById('src').value=d.src;setOut('[已载入 '+n+']','ok');}
  else setOut('载入失败: '+d.error,'err');
}
function setOut(t,c){const o=document.getElementById('out');o.textContent=t;
  o.classList.remove('err','ok');if(c)o.classList.add(c);}
function show(d){let t='';if(d.log)t+=d.log;if(d.stdout)t+=d.stdout;
  if(d.stderr)t+=(t?'\n':'')+'[stderr]\n'+d.stderr;
  if(d.exit_code!==undefined)t+='\n[exit '+d.exit_code+']';
  setOut(t||'[无输出]',d.ok===false?'err':'ok');}
async function doRun(){const s=document.getElementById('src').value;
  setOut('[运行中...]');
  const r=await fetch('/run',{method:'POST',body:s});show(await r.json());}
async function doCompile(){const s=document.getElementById('src').value;
  setOut('[编译中...]');
  const r=await fetch('/compile',{method:'POST',body:s});show(await r.json());}
async function doBundle(){const s=document.getElementById('src').value;
  setOut('[打包中...]');
  const r=await fetch('/bundle',{method:'POST',body:s});show(await r.json());}
async function doSave(){
  const name=prompt('保存为（示例: myprog.mgl）:','myprog.mgl');
  if(!name)return;
  const src=document.getElementById('src').value;
  const r=await fetch('/save',{method:'POST',headers:{'X-Filename':name},body:src});
  const d=await r.json();
  setOut(d.ok?('✓ 已保存到 examples/'+name):('保存失败: '+d.error),d.ok?'ok':'err');
}
async function qcmd(c){document.getElementById('cmd').value=c;termRun();}
async function termRun(){
  const c=document.getElementById('cmd').value.trim();
  if(!c)return;
  const o=document.getElementById('term-out');
  o.textContent='$ '+c+'\n[执行中...]';
  o.classList.remove('err','ok');
  const r=await fetch('/term',{method:'POST',body:c});
  const d=await r.json();
  let t='$ '+c+'\n';
  if(d.stdout)t+=d.stdout;
  if(d.stderr)t+='[stderr]\n'+d.stderr;
  t+='[exit '+d.exit_code+']';
  o.textContent=t;
  o.classList.add(d.exit_code===0?'ok':'err');
}
loadMap();loadExList();
</script></body></html>
'''

# 命令白名单前缀
ALLOWED = ('madctl ', 'madlang ', 'madlangc ', 'ls', 'pwd', 'cat ', 'head ', 'tail ', 'wc ', 'file ', 'stat ', 'du ')

def run(cmd, timeout=60, shell=False):
    try:
        if shell:
            r = subprocess.run(cmd, cwd=DIR, capture_output=True, text=True, timeout=timeout, shell=True)
        else:
            r = subprocess.run(cmd, cwd=DIR, capture_output=True, text=True, timeout=timeout)
        return r.returncode, r.stdout, r.stderr
    except subprocess.TimeoutExpired:
        return 124, "", "timeout"
    except Exception as e:
        return 1, "", str(e)

class H(http.server.BaseHTTPRequestHandler):
    def log_message(self, *a): pass
    def send_t(self, s, ct="text/html; charset=utf-8", code=200):
        d = s.encode("utf-8")
        self.send_response(code); self.send_header("Content-Type", ct)
        self.send_header("Content-Length", str(len(d)))
        self.send_header("Connection", "close"); self.end_headers(); self.wfile.write(d)
    def send_j(self, o):
        self.send_t(json.dumps(o, ensure_ascii=False), "application/json; charset=utf-8")
    def body(self):
        n = int(self.headers.get("Content-Length", "0"))
        return self.rfile.read(n).decode("utf-8", errors="replace") if n > 0 else ""
    def do_GET(self):
        p = urlparse(self.path).path
        if p in ("/", "/index.html"): self.send_t(HTML)
        elif p == "/mapping":
            rc, out, err = run([MADLANG, "--mapping"])
            cmds = []
            for ln in out.strip().split("\n"):
                a = ln.split("\t")
                if len(a) == 2: cmds.append({"name": a[0], "token": a[1]})
            self.send_j({"cmds": cmds})
        elif p == "/examples":
            exdir = os.path.join(DIR, "examples")
            items = []
            if os.path.isdir(exdir):
                for f in sorted(os.listdir(exdir)):
                    if f.endswith(".mgl"): items.append(f)
            self.send_j({"examples": items})
        elif p.startswith("/example/"):
            name = p[9:]
            if "/" in name or ".." in name:
                self.send_j({"ok": False, "error": "bad"}); return
            path = os.path.join(DIR, "examples", name)
            if os.path.isfile(path):
                with open(path, "r", encoding="utf-8", errors="replace") as f:
                    self.send_j({"ok": True, "name": name, "src": f.read()})
            else:
                self.send_j({"ok": False, "error": "not found"})
        else:
            self.send_t("not found", code=404)
    def do_POST(self):
        p = urlparse(self.path).path
        b = self.body()
        if p == "/run":
            t = tempfile.NamedTemporaryFile(suffix=".mgl", delete=False, dir=DIR, mode="w", encoding="utf-8")
            t.write(b); t.close()
            try:
                rc, out, err = run([MADLANG, t.name], timeout=10)
                self.send_j({"ok": rc == 0, "exit_code": rc, "stdout": out, "stderr": err})
            finally:
                try: os.unlink(t.name)
                except: pass
        elif p == "/compile":
            src = os.path.join(DIR, "madweb_tmp.mgl")
            cpp = os.path.join(DIR, "madweb_tmp.cpp")
            exe = os.path.join(DIR, "madweb_tmp.bin")
            with open(src, "w", encoding="utf-8") as f: f.write(b)
            log = ""
            rc1, o1, e1 = run([MADLANGC, src, cpp], timeout=20)
            log += "[madlangc] rc=" + str(rc1) + "\n" + o1 + e1
            if rc1 != 0: self.send_j({"ok": False, "log": log}); return
            rc2, o2, e2 = run(["g++", "-std=c++11", "-O2", "-o", exe, cpp], timeout=60)
            log += "\n[g++] rc=" + str(rc2) + "\n" + o2 + e2
            if rc2 != 0: self.send_j({"ok": False, "log": log}); return
            log += "\n✓ 生成 madweb_tmp.bin (" + str(os.path.getsize(exe)) + " bytes)\n"
            log += "  中间源码: madweb_tmp.cpp"
            self.send_j({"ok": True, "log": log})
        elif p == "/bundle":
            t = tempfile.NamedTemporaryFile(suffix=".mgl", delete=False, dir=DIR, mode="w", encoding="utf-8")
            t.write(b); t.close()
            try:
                rc, out, err = run([MADCTL, "build", t.name, "madweb_bundle"], timeout=30)
                self.send_j({"ok": rc == 0, "log": out + err, "exit_code": rc})
            finally:
                try: os.unlink(t.name)
                except: pass
        elif p == "/save":
            name = self.headers.get("X-Filename", "")
            if not name or "/" in name or ".." in name or not name.endswith(".mgl"):
                self.send_j({"ok": False, "error": "文件名非法（必须是 .mgl 且不含 /）"}); return
            path = os.path.join(DIR, "examples", name)
            try:
                with open(path, "w", encoding="utf-8") as f: f.write(b)
                self.send_j({"ok": True, "path": path})
            except Exception as e:
                self.send_j({"ok": False, "error": str(e)})
        elif p == "/term":
            cmd = b.strip()
            if not cmd:
                self.send_j({"ok": False, "exit_code": 1, "stderr": "空命令"}); return
            if not cmd.startswith(ALLOWED):
                self.send_j({"ok": False, "exit_code": 126, "stderr":
                    "命令不在白名单。允许的前缀: " + ", ".join(ALLOWED)}); return
            rc, out, err = run(cmd, timeout=30, shell=True)
            self.send_j({"ok": rc == 0, "exit_code": rc, "stdout": out, "stderr": err})
        else:
            self.send_j({"ok": False, "error": "unknown"})

def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 8765
    try:
        srv = socketserver.TCPServer(("0.0.0.0", port), H)
    except OSError as e:
        print("[madweb] port " + str(port) + " busy: " + str(e)); sys.exit(1)
    print("")
    print("  ========================================")
    print("    MADLANG Studio 已启动")
    print("  ========================================")
    print("")
    print("  浏览器: http://127.0.0.1:" + str(port) + "/")
    print("  Ctrl+C 停止")
    print("")
    try: srv.serve_forever()
    except KeyboardInterrupt: print("\n[madweb] 停止")

if __name__ == "__main__":
    main()
