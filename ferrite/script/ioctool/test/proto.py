import re

redef = re.compile("^#define\s+(\S+)\s+(.*)$")
recom = [re.compile("/\*.*\*/"), re.compile("//.*$")]

def remove_comments(line):
    for rec in recom:
        line = re.sub(rec, "", line)
    return line

def read_defines(path):
    pairs = {}
    for l in open(path, "r"):
        l = l.strip()
        m = re.search(redef, l)

        if m is not None:
            v = remove_comments(m.group(2)).strip()
            try:
                v = int(v, 0)
            except ValueError:
                try:
                    v = float(v)
                except ValueError:
                    pass

            pairs[m.group(1)] = v

    return pairs
