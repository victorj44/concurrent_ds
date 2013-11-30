
f = open("data.txt", "r")

for line in f:
    line = line.strip()
    foo = line.split(" ")
    if len(foo) > 1 and not ("#" in foo[0]):
        print str(foo[0])

f.close()
