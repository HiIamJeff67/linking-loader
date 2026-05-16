import sys

import pass1
import pass2
import common

ESTAB = {}

memoryspace = pass1.execute(sys.argv, ESTAB)

for key in ESTAB:
    print("%s:%s" % (key, hex(ESTAB[key])))

memorycontent = []
for i in range(0, (2*memoryspace)):
    memorycontent.append(".")
    
pass2.execute(sys.argv, ESTAB, memorycontent)

memorycontentstr = "".join(memorycontent)

PROGADDR = int ("%s" % sys.argv[1], 16)

for i in range(0, (2*memoryspace)):
    if (i%32 == 0):
        print("\n%s " % common.hexToString(hex(PROGADDR + int((i/2))), 4), end ="")
    print("%s" % memorycontent[i], end =" ")
print()
    
        

