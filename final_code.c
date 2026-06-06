from flask import Flask, request, redirect, send_from_directory, render_template_string
import os, json

app = Flask(__name__)
os.makedirs("data/ds", exist_ok=True)
TEAM_FILE = "data/teams.json"
MATCH_FILE = "data/matches.json"
KNOCKOUT_FILE = "data/knockout.json"

def load(f):
    if not os.path.exists(f):
        return []
    with open(f, "r") as fh:
        return json.load(fh)

def save(f, v):
    os.makedirs(os.path.dirname(f), exist_ok=True)
    with open(f, "w") as fh:
        json.dump(v, fh, indent=2)

c_samples = {
"linked_list.c": r'''
#include <stdio.h>
#include <stdlib.h>
typedef struct N{int v; struct N*next;} N;
N*mk(int x){N*p=malloc(sizeof(N)); p->v=x; p->next=0; return p;}
void push(N**h,int x){N*p=mk(x); p->next=*h; *h=p;}
void printl(N*h){for(;h;h=h->next) printf("%d ",h->v); printf("\n");}
int main(){N*head=0; push(&head,3); push(&head,5); push(&head,7); printl(head); return 0;}
''',
"stack.c": r'''
#include <stdio.h>
#define M 50
typedef struct{int a[M]; int t;} S;
void init(S*s){s->t=-1;}
void push(S*s,int x){ if(s->t<M-1) s->a[++s->t]=x; }
int pop(S*s){ if(s->t>=0) return s->a[s->t--]; return -1; }
int main(){ S s; init(&s); push(&s,1); push(&s,2); printf("%d\n",pop(&s)); return 0;}
''',
"queue.c": r'''
#include <stdio.h>
#define M 50
typedef struct{int a[M]; int h,t,c;} Q;
void init(Q*q){q->h=0; q->t=0; q->c=0;}
void en(Q*q,int x){ if(q->c<M){ q->a[q->t]=x; q->t=(q->t+1)%M; q->c++; } }
int de(Q*q){ if(q->c==0) return -1; int x=q->a[q->h]; q->h=(q->h+1)%M; q->c--; return x;}
int main(){ Q q; init(&q); en(&q,4); en(&q,9); printf("%d\n",de(&q)); return 0;}
''',
"tree.c": r'''
#include <stdio.h>
#include <stdlib.h>
typedef struct T{int v; struct T*l,*r;} T;
T*mk(int x){T*p=malloc(sizeof(T)); p->v=x; p->l=p->r=0; return p;}
void inorder(T*n){ if(!n) return; inorder(n->l); printf("%d ",n->v); inorder(n->r); }
int main(){ T*root=mk(8); root->l=mk(3); root->r=mk(10); inorder(root); printf("\n"); return 0; }
''',
"graph.c": r'''
#include <stdio.h>
#include <stdlib.h>
#define N 10
typedef struct E{int to; struct E*next;} E;
E*adj[N];
void ae(int u,int v){ E*p=malloc(sizeof(E)); p->to=v; p->next=adj[u]; adj[u]=p; }
void bfs(int s){
  int q[N],h=0,t=0,vis[N]={0};
  q[t++]=s; vis[s]=1;
  while(h<t){ int u=q[h++]; printf("%d ",u);
    for(E*p=adj[u];p;p=p->next) if(!vis[p->to]){ vis[p->to]=1; q[t++]=p->to; }
  }
}
int main(){ ae(0,1); ae(0,2); ae(1,3); printf("BFS: "); bfs(0); printf("\n"); return 0;}
'''
}

for name, code in c_samples.items():
    path = os.path.join("data/ds", name)
    with open(path, "w") as f:
        f.write(code.strip() + "\n")

for f in (TEAM_FILE, MATCH_FILE, KNOCKOUT_FILE):
    if not os.path.exists(f):
        save(f, [])

style = """
<style>
body{font-family:Arial;background:#0f1724;color:#f2aa4c;margin:0;padding:0}
.wrap{max-width:900px;margin:30px auto;padding:20px;background:#111;border-radius:10px}
h1,h2{color:#f2aa4c}
input,button,select{padding:8px;margin:6px;border-radius:6px;border:0}
table{width:100%;border-collapse:collapse;margin-top:10px}
th,td{border:1px solid #2b2b2b;padding:8px;text-align:left}
a{color:#f2aa4c;text-decoration:none}
.small{font-size:0.9rem;color:#ffdca3}
</style>
"""

home_html = """
{{style}}
<div class="wrap">
<h1>Arena Master</h1>
<p class="small">Register teams, run round-robin, then knockout.</p>
<form action="/register" method="post">
  <input name="team" placeholder="Team name" required>
  <input name="captain" placeholder="Captain" required>
  <button>Register</button>
</form>
<p>
<a href="/teams">Teams</a> |
<a href="/generate">Generate Fixtures</a> |
<a href="/fixtures">Fixtures</a> |
<a href="/leaderboard">Leaderboard</a> |
<a href="/knockout">Knockout</a> |
<a href="/ds">DS (C examples)</a>
</p>
</div>
"""

msg_html = """
{{style}}
<div class="wrap">
<h2>{{text}}</h2>
<p><a href="/">Back</a></p>
</div>
"""

teams_html = """
{{style}}
<div class="wrap">
<h2>Teams ({{teams|length}})</h2>
<table><tr><th>ID</th><th>Name</th><th>Captain</th></tr>
{% for t in teams %}
<tr><td>{{t.id}}</td><td>{{t.name}}</td><td>{{t.captain}}</td></tr>
{% endfor %}
</table>
<p><a href="/">Back</a></p>
</div>
"""

fixtures_html = """
{{style}}
<div class="wrap">
<h2>Fixtures</h2>
<form action="/generate" method="get">
  <button>Regenerate Fixtures (round-robin)</button>
</form>
<table><tr><th>ID</th><th>Team 1</th><th>Team 2</th><th>Result</th></tr>
{% for m in matches %}
<tr>
<td>{{m.id}}</td><td>{{m.team1}}</td><td>{{m.team2}}</td>
<td>
{% if m.played %}
  {{m.score1}} - {{m.score2}}
{% else %}
  <form action="/record" method="post" style="display:inline">
    <input type="hidden" name="mid" value="{{m.id}}">
    <input name="s1" type="number" min=0 required style="width:60px">
    <input name="s2" type="number" min=0 required style="width:60px">
    <button>Save</button>
  </form>
{% endif %}
</td>
</tr>
{% endfor %}
</table>
<p><a href="/">Back</a></p>
</div>
"""

leader_html = """
{{style}}
<div class="wrap">
<h2>Leaderboard</h2>
<table><tr><th>Pos</th><th>Team</th><th>Pl</th><th>W</th><th>D</th><th>L</th><th>Pts</th></tr>
{% for i,t in enumerate(teams) %}
<tr><td>{{i+1}}</td><td>{{t.name}}</td><td>{{t.played}}</td><td>{{t.wins}}</td><td>{{t.draws}}</td><td>{{t.losses}}</td><td>{{t.points}}</td></tr>
{% endfor %}
</table>
<p><a href="/">Back</a></p>
</div>
"""

knock_html = """
{{style}}
<div class="wrap">
<h2>Knockout (top 4)</h2>
{% if top4 %}
  <form action="/record_knock" method="post">
    <input type="hidden" name="r" value="semi1">
    <input type="hidden" name="a" value="{{top4[0].name}}">
    <input type="hidden" name="b" value="{{top4[3].name}}">
    <label>{{top4[0].name}}</label>
    <input name="s1" type="number" min=0 required style="width:60px">
    vs
    <input name="s2" type="number" min=0 required style="width:60px">
    <label>{{top4[3].name}}</label>
    <button>Submit Semi 1</button>
  </form>
  <form action="/record_knock" method="post">
    <input type="hidden" name="r" value="semi2">
    <input type="hidden" name="a" value="{{top4[1].name}}">
    <input type="hidden" name="b" value="{{top4[2].name}}">
    <label>{{top4[1].name}}</label>
    <input name="s1" type="number" min=0 required style="width:60px">
    vs
    <input name="s2" type="number" min=0 required style="width:60px">
    <label>{{top4[2].name}}</label>
    <button>Submit Semi 2</button>
  </form>
  <p><a href="/show_knock">Show bracket</a></p>
{% else %}
  <p>Need 4 teams.</p>
{% endif %}
<p><a href="/">Back</a></p>
</div>
"""

show_knock_html = """
{{style}}
<div class="wrap">
<h2>Bracket</h2>
<table><tr><th>Round</th><th>Team A</th><th>Score</th><th>Team B</th><th>Winner</th></tr>
{% for m in matches %}
<tr><td>{{m.round}}</td><td>{{m.team1}}</td><td>{% if m.score1 is not none %}{{m.score1}} - {{m.score2}}{% else %} - {% endif %}</td><td>{{m.team2}}</td><td>{{m.winner or "-"}}</td></tr>
{% endfor %}
</table>
{% if champion %}<h3 style="color:#0f0">Champion: {{champion}}</h3>{% endif %}
<p><a href="/">Back</a></p>
</div>
"""

ds_index_html = """
{{style}}
<div class="wrap">
<h2>DS C examples</h2>
<ul>
{% for f in files %}
  <li><a href="/ds/{{f}}">{{f}}</a></li>
{% endfor %}
</ul>
<p><a href="/">Back</a></p>
</div>
"""

ds_view_html = """
{{style}}
<div class="wrap">
<h2>{{name}}</h2>
<pre>{{code}}</pre>
<p><a href="/ds">Back</a> | <a href="/download/{{name}}">Download</a></p>
</div>
"""

@app.route("/")
def home():
    return render_template_string(home_html, style=style)

@app.route("/register", methods=["POST"])
def register():
    teams = load(TEAM_FILE)
    name = request.form.get("team","").strip()
    cap = request.form.get("captain","").strip()
    if not name:
        return render_template_string(msg_html, style=style, text="Name required")
    for t in teams:
        if t["name"].lower() == name.lower():
            return render_template_string(msg_html, style=style, text="Team exists")
    teams.append({"id": len(teams)+1, "name": name, "captain": cap, "played":0, "wins":0, "draws":0, "losses":0, "points":0})
    save(TEAM_FILE, teams)
    return redirect("/teams")

@app.route("/teams")
def show_teams():
    return render_template_string(teams_html, style=style, teams=load(TEAM_FILE))

@app.route("/generate")
def gen():
    teams = load(TEAM_FILE)
    matches = []
    for i in range(len(teams)):
        for j in range(i+1, len(teams)):
            matches.append({"id": len(matches)+1, "team1": teams[i]["name"], "team2": teams[j]["name"], "score1": None, "score2": None, "played": False})
    save(MATCH_FILE, matches)
    return redirect("/fixtures")

@app.route("/fixtures")
def fixtures():
    return render_template_string(fixtures_html, style=style, matches=load(MATCH_FILE))

@app.route("/record", methods=["POST"])
def record_result():
    mid = int(request.form.get("mid"))
    s1 = int(request.form.get("s1", 0))
    s2 = int(request.form.get("s2", 0))
    matches = load(MATCH_FILE)
    teams = load(TEAM_FILE)
    for m in matches:
        if m["id"] == mid and not m["played"]:
            m["score1"] = s1; m["score2"] = s2; m["played"] = True
            for t in teams:
                if t["name"] in (m["team1"], m["team2"]):
                    t["played"] += 1
            if s1 > s2:
                for t in teams:
                    if t["name"] == m["team1"]: t["wins"] +=1; t["points"] += 3
                    if t["name"] == m["team2"]: t["losses"] +=1
            elif s2 > s1:
                for t in teams:
                    if t["name"] == m["team2"]: t["wins"] +=1; t["points"] += 3
                    if t["name"] == m["team1"]: t["losses"] +=1
            else:
                for t in teams:
                    if t["name"] in (m["team1"], m["team2"]): t["draws"] +=1; t["points"] +=1
    save(MATCH_FILE, matches); save(TEAM_FILE, teams)
    return redirect("/fixtures")

@app.route("/leaderboard")
def leaderboard():
    teams = load(TEAM_FILE)
    teams = sorted(teams, key=lambda x:(-x["points"], -x["wins"], x["name"]))
    return render_template_string(leader_html, style=style, teams=teams)

@app.route("/knockout")
def knockout():
    teams = load(TEAM_FILE)
    if len(teams) < 4:
        return render_template_string(msg_html, style=style, text="Need 4 teams")
    teams = sorted(teams, key=lambda x:(-x["points"], -x["wins"], x["name"]))
    top4 = teams[:4]
    save(KNOCKOUT_FILE, [])
    return render_template_string(knock_html, style=style, top4=top4)

@app.route("/record_knock", methods=["POST"])
def record_knock():
    r = request.form.get("r")
    a = request.form.get("a")
    b = request.form.get("b")
    s1 = int(request.form.get("s1", 0))
    s2 = int(request.form.get("s2", 0))
    matches = load(KNOCKOUT_FILE)
    winner = a if s1 >= s2 else b
    matches.append({"round": r, "team1": a, "team2": b, "score1": s1, "score2": s2, "winner": winner})
    if len(matches) == 2:
        w0 = matches[0]["winner"]; w1 = matches[1]["winner"]
        matches.append({"round":"Final", "team1":w0, "team2":w1, "score1": None, "score2": None, "winner": None})
    save(KNOCKOUT_FILE, matches)
    return redirect("/show_knock")

@app.route("/show_knock")
def show_knock():
    matches = load(KNOCKOUT_FILE)
    champ = None
    if matches and matches[-1].get("winner"):
        champ = matches[-1]["winner"]
    return render_template_string(show_knock_html, style=style, matches=matches, champion=champ)

@app.route("/ds")
def ds_index():
    files = sorted(os.listdir("data/ds"))
    return render_template_string(ds_index_html, style=style, files=files)

@app.route("/ds/<name>")
def ds_view(name):
    path = os.path.join("data/ds", name)
    if not os.path.exists(path):
        return render_template_string(msg_html, style=style, text="Not found")
    with open(path, "r") as f:
        code = f.read()
    return render_template_string(ds_view_html, style=style, name=name, code=code)

@app.route("/download/<name>")
def dl(name):
    return send_from_directory("data/ds", name, as_attachment=True)

if __name__ == "__main__":
    app.run(debug=True)
