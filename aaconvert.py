#!/usr/bin/python
# -*- coding: utf8 -*-
import sys,random,re,cgi
import urllib
import simplejson
import base64
import random
import sys
import os
import optparse
import subprocess
import traceback
import StringIO
import tempfile



class AA:
    def __init__(self, executable_name="aaconvert"):
        if sys.platform.startswith("win32"):
            if not executable_name.endswith(".exe"):
                executable_name+=".exe"
        self.progname = os.path.join(os.path.dirname(__file__), executable_name)
        if not os.path.exists(self.progname):
            raise ValueError("Could not find program %s" % (self.progname,))



    def dessine(self, fn, num_lignes=25, quality=5):
        """
        Lis l'image depuis le fichier <fn>, la convertit en ascii art et retourne le texte ascii
        """
        if not os.path.exists(fn):
            raise IOError("Cannot open file %s" % (fn,))
        cline = [self.progname, fn, str(num_lignes), str(quality)]
        try:
            self.last_img = subprocess.check_output(cline)
        except Exception,e:
            return str(e)
        return self.last_img


    def redessine(self):
        """
        Redessine la derniere image affichee
        """
        return self.last_img

    def fyglet(self, text, font=None):
        """
        (str,str)
        Dessine <text> via fyglet. Si <font> est nul, choisis une font au hasard
        """
        import pyfiglet, pyfiglet.fonts  #lame package
        res=""
        first = True
        while not res.replace("\n","").replace("\r","").strip():
            if font is None or not first:
                font=random.choice(pyfiglet.FigletFont.getFonts())
            f=pyfiglet.Figlet(font=font,width=60)
            res=f.renderText(text)
            first = False
        return res



def get_img_openclipart(what):
    query = urllib.urlencode({"query" : what, "page":"1"})
    url = "http://openclipart.org/search/json/?%s" % (query)
    search_results = urllib.urlopen(url)
    json = simplejson.loads(search_results.read())
    r=json["payload"][0]
    #r=random.choice(json["responseData"]["results"][0])
    u=r["svg"]["png_thumb"]
    return u

def get_img_google(what, clipart=True):
    params = {
            "q" : what, 
            "googlehost":"google.com",
            "hl":"fr",
            "isc":"white",
            "isz":"s",
            "safe":"off"
    }
    if clipart:
        params["imgtype"] ="clipart"
    query = urllib.urlencode(params)
    url = "http://ajax.googleapis.com/ajax/services/search/images?v=1.0&%s" % (query)
    search_results = urllib.urlopen(url)
    json = simplejson.loads(search_results.read())
    r=json["responseData"]["results"][0]
    #r=random.choice(json["responseData"]["results"][0])
    u=r["url"]
    return u




if __name__=="__main__":
    usage = 'usage: %prog [options] <what>'
    parser = optparse.OptionParser(usage=usage, description="""Draws an image in compact ascii art""")
    parser.add_option('-f', '--file', dest='isfile', action="store_true", default=False, help='The command line argument is a filepath')
    parser.add_option('-u', '--url', dest='isurl', action="store_true", default=False, help='The command line argument is an url')
    parser.add_option('-g', '--google', dest='isgoogle', action="store_true", default=False, help='The command line argument is a google image search')
    parser.add_option('-i', '--googleclipart', dest='isgoogleclipart', action="store_true", default=False, help='The command line argument is a google clipart image search')
    parser.add_option('-c', '--openclipart', dest='isopenclipart', action="store_true", default=False, help='The command line argument is a openclipart image search')
    parser.add_option('-y', '--fyglet', dest='isfyglet', action="store_true", default=False, help='The command line argument is a figlet text')
    parser.add_option('-s', '--size', dest='size', action="store", type=int, default=25, help='Height of the ascii art in characters (default is 25)')
    parser.add_option('-q', '--quality', dest='quality', action="store", type=int, default=5, help='Quality of the ascii translation (0-10). Default to 5.')
    parser.add_option('-t', '--font', dest='font', default="", help='font name to use for pyfiglet')
    (options, args) = parser.parse_args()
    if len(args) != 1:
        parser.error('Incorrect number of arguments')
    what = args[0]
    aa=AA()
    imageaa = None
    filepointer = None
    fyglettext = None
    fygletfont = None
    if options.isgoogle or options.isgoogleclipart:
        url = get_img_google(what, clipart=options.isgoogleclipart)
        print "Image originale: %s" % url
        what = url
        options.isurl=True
    elif options.isopenclipart:
        url = get_img_openclipart(what)
        print "Image originale: %s" % url
        what = url
        options.isurl = True        

    if options.isfyglet:
        print aa.fyglet(fyglettext,fygletfont)
    elif options.isurl:
        fname = tempfile.mktemp("aaconvert")
        with open(fname, "wb") as f:
            f.write(urllib.urlopen(url).read())
        try:
            print aa.dessine(fname, options.size, options.quality)
        finally:
            os.remove(fname)
    elif options.isfile:
        print aa.dessine(what, options.size, options.quality)










