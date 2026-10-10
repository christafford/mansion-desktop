#!/usr/bin/env python3
"""Local test page; real Chrome is the client under test, not this fixture."""
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path
import json
import sys

state = Path(sys.argv[1])
state.mkdir(parents=True, exist_ok=True)
page = b'''<!doctype html><meta charset="utf-8"><title>Elsewhere browser acceptance</title>
<style>body{margin:36px;font:20px sans-serif;background:#f4efe5;color:#253338}
h1{font-size:36px}input{display:block;font:24px sans-serif;padding:14px;width:470px}
button{margin-top:24px;border:0;background:rgb(20,120,200);color:white;padding:20px 36px;font:22px sans-serif}
footer{margin-top:1100px;padding:30px;background:#253338;color:white}</style>
<h1>A browser in the elsewhere</h1><p>This page is running in real Chrome through Elsewhere's Wayland compositor.</p>
<label>Workspace note<input id="note" autofocus autocomplete="off"></label>
<button id="save">Save note</button><p id="result">Ready for keyboard and pointer input.</p>
<footer>End of the local scrolling test page.</footer>
<script>
let clicks=0, trusted=false;
function report(){fetch('/report',{method:'POST',body:JSON.stringify({value:note.value,ready:document.readyState==='complete'&&document.activeElement===note,clicks,trusted,width:innerWidth,height:innerHeight,scroll:scrollY})});}
note.oninput=e=>{trusted=e.isTrusted;report()};
save.onclick=e=>{clicks++;trusted=e.isTrusted;result.textContent='Saved: '+note.value;report()};
onresize=report;onscroll=report;onload=()=>{note.focus();report()};
</script>'''


class Handler(BaseHTTPRequestHandler):
    def do_GET(self):
        self.send_response(200)
        self.send_header('Content-Type', 'text/html; charset=utf-8')
        self.end_headers()
        self.wfile.write(page)

    def do_POST(self):
        data = json.loads(self.rfile.read(int(self.headers['Content-Length'])))
        tmp = state / 'page.json.tmp'
        tmp.write_text(json.dumps(data))
        tmp.replace(state / 'page.json')
        self.send_response(204)
        self.end_headers()

    def log_message(self, *_args):
        pass


with HTTPServer(('127.0.0.1', 0), Handler) as server:
    (state / 'port.txt').write_text(str(server.server_port))
    server.serve_forever()
