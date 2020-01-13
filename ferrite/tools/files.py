import re

def substitute(rep, src, dst=None):
    if dst is None:
        dst = src
    
    with open(src, 'r') as file:
        data = file.read()

    print("replace '%s' -> '%s':" % (src, dst))
    for s, d in rep:
        data = re.sub(s, d, data, flags=re.M)
        print("  %s -> %s" % (s, d))

    with open(dst, 'w') as file:
        file.write(data)
