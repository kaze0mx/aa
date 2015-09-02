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
import cStringIO as StringIO
from PIL import ImageFont, ImageDraw, ImageEnhance, ImageFilter, Image, ImageOps
 
PASX=8
PASY=16
PEN_VIDE=0.8
PEN_PLEIN=2
SUBSET=" .o~/\\-_,;\"'<>^vrpdbqQ#|+`()yi=7"
ADAPT=(0,1,-1,-2,2)
NOADAPT=(0,)


class AA:
    def __init__(self,car_subset=SUBSET,font="",penalty_empty=PEN_VIDE,penalty_full=PEN_PLEIN,adjust_translate=ADAPT,preprocess=True):
        self.fontmap=u" ²$^¨°+*#&é\"'`|[]{}~(-_à)=123456789azertyuiopqsdfghjklm<wxcvbn,;:!AZERTYUIOPQSDFGHJKLM%µ>WXC@VBN?.\\/§%"
        if car_subset is None:
            car_subset=self.fontmap
        self.indmap=[]
        for s in car_subset:
            self.indmap.append(self.fontmap.find(s))
        if font:
            # Create Font tab
            fnt=Image.new("L",(PASX*len(self.fontmap),PASY))
            draw = ImageDraw.Draw(fnt)
            font = ImageFont.truetype(font, 14)
            draw.text((0, 0), self.fontmap, font=font,fill="white")
            fnt.save(os.path.join(os.path.dirname(__file__),"fnt.png"),"PNG")
        # Create bloc tab
        try:
            fnt=Image.open(os.path.join(os.path.dirname(__file__),"fnt.png"))
            self.blocmap=[]
            for car in range(len(self.fontmap)):
                bloc=self.get_bloc(fnt,car*PASX,0,perpixel=lambda k: int(k!=0))
                self.blocmap.append(bloc)
        except:
            pass
        self.last_img=None
        self.penalty_empty=penalty_empty
        self.penalty_full=penalty_full
        self.adjust_translate=adjust_translate
        self.preprocess=preprocess



    def dessine(self,fn,ratio=32):
        """
        (str,int)
        Telecharge l'image depuis le fichier <fn>, la convertit en ascii art et l'affiche
        """
        res=""
        img=Image.open(fn)
        height=ratio*PASY
        percent = (height/float(img.size[1]))
        hsize = int((float(img.size[0])*float(percent)))
        img = img.resize((hsize,height), Image.ANTIALIAS)
        width=img.size[0]+PASX-img.size[0]%PASX
        w=img.size[0]+0
        wl=int((width-w)/2)
        wr=int((width-w)/2) + (width-w)%2
        img=img.crop((0-wl,0,w+wr,height))
        if self.preprocess:
            try:
                img=img.filter(ImageFilter.CONTOUR)
                img=img.filter(ImageFilter.SMOOTH)
                myfilter=ImageFilter.Kernel((3,3),(0,0,0,0,1,0,0,0,0))
                img=img.filter(myfilter)
                pass
            except: pass
        img=img.convert("1")
        draw=ImageDraw.Draw(img)
        draw.rectangle((0,0,wl,height),fill=1)
        draw.rectangle((width-wr,0,width,height),fill=1)
        #img.save("/volume1/web/img.png","PNG")
        #res+=u"Traitee: http://rhino9.fr/img.png\n"
        diffs=[]
        for y in range(0,height,PASY):
            for x in range(0,width,PASX):
                minoferrors=None
                bestofbests=None
                for dx in self.adjust_translate:
                    for dy in self.adjust_translate:
                        if x+dx+PASX > img.size[0] or x+dx < 0 or y+dy+PASY > img.size[1] or y+dy < 0:
                            continue
                        best,error=self.get_best_car(img,x+dx,y+dy)
                        if minoferrors is None or error < minoferrors:
                            minoferrors = error
                            bestofbests = best
                res=res+bestofbests
            res+="\n"
        self.last_img=res.encode("ascii",errors="replace")
        return self.last_img


    def redessine(self):
        """
        Redessine la derniere image affichee
        """
        return self.last_img

    def fyglet(self,text,font=None):
        """
        (str,str)
        Dessine <text> via fyglet. Si <font> est nul, choisis une font au hasard
        """
        import pyfiglet  #lame package
        res=""
        first = True
        while not res.replace("\n","").replace("\r","").strip():
            if font is None or not first:
                font=random.choice(pyfiglet.FigletFont.getFonts())
            f=pyfiglet.Figlet(font=font,width=60)
            res=f.renderText(text)
            first = False
        return res

    def get_best_car(self,img,x,y):
        minerr=None
        best=None
        ibloc=self.get_bloc(img,x,y,perpixel=lambda k: int(k==0))
        for i in self.indmap:
            fbloc=self.blocmap[i]
            car=self.fontmap[i]
            err=self.diff_bloc(ibloc,fbloc)
            if minerr is None or err<minerr:
                best=car
                minerr=err
            if minerr<2:
                break
        return best,minerr

    def get_bloc(self,img,dx,dy,w=PASX,h=PASY,perpixel=lambda k : k):
        bloc=[]
        for y in range(dy,dy+h):
            ligne=[]
            for x in range(dx,dx+w):
                pix=perpixel(img.getpixel((x,y)))
                ligne.append(pix)
            bloc.append(ligne)
        return bloc

    def diff_bloc(self,a,b,w=PASX,h=PASY):
        err=0
        for y in range(h):
            for x in range(w):
                if a[y][x]!=b[y][x]:
                    if a[y][x]!=0:
                        err+=self.penalty_full
                    else:
                        err+=self.penalty_empty
        return err


def get_img_openclipart(what):
    query = urllib.urlencode({"query" : what, "page":"1"})
    url = "http://openclipart.org/search/json/?%s" % (query)
    search_results = urllib.urlopen(url)
    json = simplejson.loads(search_results.read())
    r=json["payload"][0]
    #r=random.choice(json["responseData"]["results"][0])
    u=r["svg"]["png_thumb"]
    return u

def get_img_google(what):
    query = urllib.urlencode({"q" : what, "googlehost":"google.com","imgtype":"clipart","hl":"fr","isc":"white","isz":"s","safe":"off"})
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
    parser.add_option('-c', '--openclipart', dest='isopenclipart', action="store_true", default=False, help='The command line argument is a openclipart image search')
    parser.add_option('-l', '--fyglet', dest='isfyglet', action="store_true", default=False, help='The command line argument is a figlet text')
    parser.add_option('-s', '--size', dest='size', action="store", type=int, default=32, help='Height of the ascii art in characters (default is 32)')
    parser.add_option('--subset', dest='subset', action="store", default=SUBSET, help='Subset of the ascii table to use (defaults to %s)' % (repr(SUBSET),))
    parser.add_option('--empty', dest='empty_penalty', action="store", type=float, default=PEN_VIDE, help='Penality for a black pixel miss (defaults to %.02f)' % (PEN_VIDE,))
    parser.add_option('--full', dest='full_penalty', action="store", type=float, default=PEN_VIDE, help='Penality for a white pixel miss (defaults to %.02f)' % (PEN_PLEIN,))
    parser.add_option('--translate', dest='translate_level', action="store", type=int, default=0, help='To improve the quality of the aa, we can try for eac bloc to translate it from a few pixel. Default is to not translate it (0)')
    parser.add_option('--nopreproc', dest='nopreproc', action="store_true", default=False, help='Do not preprocess the image (edge detection+smoothing)')
    parser.add_option('--font', dest='font', default="", help='Do not preprocess the image (edge detection+smoothing)')
    (options, args) = parser.parse_args()
    if len(args) != 1:
        parser.error('Incorrect number of arguments')
    what=args[0]
    translate=[0,]
    for i in range(1,options.translate_level):
        translate.extend([i,-i])
    aa=AA(car_subset=options.subset,penalty_empty=options.empty_penalty,penalty_full=options.full_penalty,adjust_translate=translate,preprocess=not options.nopreproc,font=options.font)
    imageaa=None
    filepointer=None
    fyglettext=None
    fygletfont=None
    if options.isgoogle:
        url=get_img_google(what)
        print "Image originale: %s" % url
        what = url
        options.isurl=True
    elif options.isopenclipart:
        url=get_img_openclipart(what)
        print "Image originale: %s" % url
        what = url
        options.isurl=True        
    if options.isurl:
        filepointer=StringIO.StringIO(urllib.urlopen(url).read())
    if options.isfile:
        filepointer=open(what,"rb")
    if options.isfyglet:
        fyglettext=what
    
    if filepointer:
        print aa.dessine(filepointer,options.size)
        filepointer.close()

    if fyglettext:
        print aa.fyglet(fyglettext,fygletfont)









