import os, sys
from ctypes import *

class AaColor(Structure):
    _pack_ = 1
    _fields_ = [
            ("rgbRed", c_byte),
            ("rgbGreen", c_byte),
            ("rgbBlue", c_byte),
     ]

class AaPalette(Structure):
    _pack_ = 1
    _fields_ = [
            ("raw", POINTER(AaColor)),
            ("size", c_int),
     ]

class AaImage(Structure):
    _pack_ = 1
    _fields_ = [
            ("cols", c_int),
            ("lines", c_int),
            ("characters", c_char_p),
            ("colors", POINTER(c_byte)),
            ("palette", POINTER(AaPalette)),
     ]


AA_FT_COURIER = 0
AA_FT_LUCIDA = 1
AA_FT_CONSOLA = 2

FONT_SUBSET = " $^+*#&\"'|~(-)=345atiopqdfghjkl<,:AZTYJL%>X@VN?.\\/_"
FONT_TINY_SUBSET = " .o@/\\_'yT)(,-bd"

    
class Aa:
    def __init__(self, font=AA_FT_COURIER, font_alphabet=FONT_SUBSET):
        me = os.path.abspath(os.path.dirname(__file__))
        oldpath = os.environ['PATH'] 
        os.environ['PATH'] = me + ';' + os.environ['PATH']
        if os.name == "nt":
            dll = cdll.LoadLibrary(os.path.join(me, "aa.dll"))
        else:
            dll = cdll.LoadLibrary(os.path.join(me, "aa.so"))
        os.environ['PATH'] = oldpath

        #self.ad2_get_dumps = unpack_dll.ad2_get_dumps
        #self.ad2_get_dumps.argtypes = [ctypes.c_void_p, ctypes.POINTER(ExportedDump), ctypes.c_uint]
        #self.ad2_get_dumps.restype = ctypes.c_uint     

        self.init_font_from_picture = dll.aa_init_font_from_picture
        self.init_font_from_picture.argtypes = [c_int, c_char_p, c_void_p]
        self.init_font_from_picture.restype = ctypes.c_bool     
        
        
        

        


if __name__ == "__main__":

    aa = Aa()
    
