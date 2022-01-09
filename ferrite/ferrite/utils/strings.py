
def quote(text: str, char: str = '"'):
    return char + text.replace("\\", "\\\\").replace(char, "\\" + char) + char

def try_format(src, **kwargs):
    rep = {}
    for k, v in kwargs.items():
        if ("{" + k + "}") in src:
            rep[k] = v
    return src.format(**rep)
