import re
import logging as log

def match(pat, src):
    log.debug("matching '%s': '%s':" % (src, pat))

    with open(src, 'r') as file:
        data = file.read()
    
    return re.match(pat, flags=re.M)

def substitute(rep, src, dst=None):
    if dst is None:
        dst = src
    
    log.debug("substituting '%s' -> '%s':" % (src, dst))

    with open(src, 'r') as file:
        data = file.read()

    for s, d in rep:
        data = re.sub(s, d, data, flags=re.M)

    with open(dst, 'w') as file:
        file.write(data)
