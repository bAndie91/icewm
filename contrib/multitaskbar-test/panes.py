import subprocess,re,sys
def run(*a):
    return subprocess.run(a,capture_output=True,text=True,timeout=10,env={"DISPLAY":":79"}).stdout
tree=run("xwininfo","-root","-tree").splitlines()
i=0; out=[]
while i<len(tree):
    m=re.match(r'^(\s+)(0x[0-9a-f]+) "TaskPane"',tree[i])
    if m:
        ind=len(m.group(1)); pos=re.search(r'\+(\d+)\+\d+\s*$',tree[i]); kids=[]
        j=i+1
        while j<len(tree) and (len(tree[j])-len(tree[j].lstrip()))>ind:
            k=re.match(r'^\s+(0x[0-9a-f]+) ',tree[j])
            if k and 'children:' not in tree[j] and 'child:' not in tree[j]:
                info=run("xwininfo","-id",k.group(1))
                st=re.search(r'Map State: (\w+)',info).group(1)
                kids.append(st)
            j+=1
        out.append((int(pos.group(1)) if pos else -1,kids))
    i+=1
for x,k in sorted(out): print(f"  pane abs_x={x}: {len(k)} button window(s), states={k}")
