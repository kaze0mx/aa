import logging, os, sys
import optparse
import multiprocessing
import json, urllib, requests
from flask import Flask, session, request, flash, url_for, redirect, render_template, abort, g, jsonify
sys.path.append("..")
from aa import *

DEFAULT_NUM = 30
BING_KEY = "+wGKA1eii9MBZnCIsQprsBtV8HXfjHB2huCImhM6Fko"
SEARCH_QUALITY = 5
SEARCH_LINES = 20

app = Flask(__name__)
aa = AaConverter()
last_image= None


def search_iter_bing(search, clipart=True):
    if clipart:
        # Bing
        query = urllib.urlencode({"Query" : repr(str(search)), "ImageFilters": repr("Style:Graphics+Color:Monochrome"), "Market": repr("fr-FR"), "Adult":repr("Off")})
        url = "https://api.datamarket.azure.com/Data.ashx/Bing/Search/Image?{}&$format=json".format(query)
        search_results = requests.get(url, auth=(BING_KEY, BING_KEY)).json()
        res = search_results.get("d", {}).get("results", [])
        for r in res:
            yield r.get("MediaUrl")
    return 

def search_iter_google(search, clipart=True):
    if clipart:
        query = urllib.urlencode({"q" : search, "hl":"fr", "imgc":"gray", "imgtype":"clipart", "safe":"off"})
        url = "http://ajax.googleapis.com/ajax/services/search/images?v=1.0&{}".format(query)
        search_results = urllib.urlopen(url)
        jsonobj = json.loads(search_results.read())
        results = jsonobj["responseData"]["results"]
        for r in results:
            yield r["url"]
    return 


@app.route("/")
def index():
    if request.method == "GET":
        formdata = request.args
    else:
        formdata = request.form
    r = []
    req = formdata.get("req", "")
    if req:
        urls = []
        start = formdata.get("start", 0)
        clipart = formdata.get("clipart", True)
        i = 0
        for el in search_iter_google(formdata["req"], clipart):
            if len(urls) >= DEFAULT_NUM/2:
                break        
            if i >= start:
                urls.append(el) 
            i += 1
        for el in search_iter_bing(formdata["req"], clipart):
            if len(urls) >= DEFAULT_NUM:
                break        
            if i >= start:
                urls.append(el)
            i += 1        
        # Convert
        params = ConvertParameters.optimize(SEARCH_QUALITY)
        todo = [ (u, SEARCH_LINES, params) for u in urls]
        for res in app.pool.map(convert, todo):
            ok, url, ascii = res
            if ok:
                r.append((url, ascii))
    return render_template("base.html", result=r, query=req)



def convert(args):
    url, lines, config = args
    try:
        inputimage = InputImage.from_url(url)
        inputimage.autocrop()
        t = str(aa.convert(inputimage, lines=lines, params=config))
        if not t.strip():
            raise ValueError("empty")
        nl = t.splitlines()
        if nl < SEARCH_LINES:
            t += "\n"*(SEARCH_LINES-nl)
        return (True, url, t)
    except BaseException as e:
        return (False, url, str(e))

if __name__ == '__main__':
    # Run t
    app.pool = multiprocessing.Pool(processes=DEFAULT_NUM)              # start 4 worker processes
    app.run(host='0.0.0.0', threaded=True, port=5000, debug=False)
    
