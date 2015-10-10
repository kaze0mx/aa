import logging, os, sys
sys.path.append(os.path.join(os.path.dirname(__file__), ".."))
from aa import *

aa = AaConverter()

def convert(args):
    url, lines, quality = args
    params = ConvertParameters.optimize(quality)
    try:
        inputimage = InputImage.from_url(url)
        inputimage.autocrop()
        t = str(aa.convert(inputimage, lines=lines, params=params))
        if not t.strip():
            raise ValueError("empty")
        nl = t.splitlines()
        if nl < lines:
            t += "\n"*(SEARCH_LINES-nl)
        return (True, url, t)
    except BaseException as e:
        return (False, url, str(e))
