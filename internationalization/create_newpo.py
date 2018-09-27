import sys

f = open(sys.argv[1])
lines = f.readlines()
f.close()

msgids = {}
for line in lines:
    if line[:4] == "msgid":
        msgids[line[7:-1]] = True

o = open(sys.argv[2], 'w')        
for id in msgids.keys():
    line = 'msgid "%s"\n' % id
    o.write(line)
    o.write('msgstr ""\n')
    o.write("\n")
    
o.close()
    