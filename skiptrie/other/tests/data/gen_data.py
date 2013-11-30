import random
import math

MAXN = 8 * 1000 * 1000 #8M items = 2^23
MAXQUERY  = 16*1000*1000 #16M = 2^24
MAXLVL = 20

def getRandLevel():
    r = random.randint(1, 1 << MAXLVL)
    lvl = int(math.log(r, 2))
    return MAXLVL - lvl


insertData = []
queryData = []

print "init"

for x in range(1, MAXQUERY + 1):
    queryData.append(x)
random.shuffle(queryData)

print "part1 done"

#unique MAXN elements choosen at random from (1, MAXQUERY)
for x in range(0, MAXN):
    insertData.append(queryData[x])

print "part 2 done"

#re-shuffle queryData [effectively re-shuffles insertData]
random.shuffle(queryData)

print "part 3 done"

f = open("insert.txt", "w")
for x in range(0, MAXN):
    f.write(str(insertData[x]) + " " + str(getRandLevel()) + "\n")
f.close()

print "part 4 done"

f = open("query.txt", "w")
for x in range(0, MAXQUERY):
    f.write(str(queryData[x])  + "\n")
f.close()

print "part 5 done"
    
    


