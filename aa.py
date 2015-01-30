import os, sys, collections
from ctypes import *
import json, urllib

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

class AaInternalImage(Structure):
    _pack_ = 1
    _fields_ = [
            ("cols", c_int),
            ("lines", c_int),
            ("characters", POINTER(c_char)),
            ("colors", POINTER(c_byte)),
            ("palette", POINTER(AaPalette)),
     ]

class AaFont(Structure):
    _pack_ = 1
    _fields_ = [
            ("font_bitmap", c_void_p),
            ("font_buffer", POINTER(c_byte)),
            ("font_buffer_pitch", c_int),
            ("font_carmap", POINTER(c_char)),
            ("font_carmap_len", c_int),
            ("font_subset", POINTER(c_int)),
            ("font_subset_len", c_int),
            ("font_subset_matrix_empty", POINTER(c_int)),
            ("font_subset_matrix_full", POINTER(c_int)),
     ]


AA_FT_COURIER = 0
AA_FT_LUCIDA = 1
AA_FT_CONSOLA = 2

AA_ALG_PIXEL_11 = 0
AA_ALG_VECTOR_DST = 1
AA_ALG_VECTOR_DST_FILL = 2
AA_ALG_VECTOR_11 = 3
AA_ALG_VECTOR_11_FILL = 4

AA_PAL_NONE = 0
AA_PAL_MONOCHROME = 1
AA_PAL_FREE_MONO = 2
AA_PAL_FREE_16 = 3
AA_PAL_FREE_64 = 4
AA_PAL_FREE_256 = 5

FONT_SUBSET = " $^+*#&\"'|~(-)=345atiopqdfghjkl<,:AZTYJL%>X@VN?.\\/_"
FONT_TINY_SUBSET = " .o@/\\_'yT)(,-bd"


class AaImage:

    def __init__(self, width, height, content):
        self.width = width
        self.height = height
        self.content = content

    def __repr__(self):
        b = []
        for i in xrange(self.height):
            b.append(self.content[self.width*i:self.width*(i+1)])
        return "\n".join(b)

        

class ConvertParameters:

    def __init__(self, 
            font = AA_FT_COURIER,    # font to use as a comparision point
            font_subset = FONT_SUBSET, # alphabet we are allowed to use
            ascii_algorithm = AA_ALG_VECTOR_DST,        # ascii conversion algorithm
            color_palette = AA_PAL_NONE,    # color palette to use (only for color outputs)
            penality_white=0.5,     # Missing a lighted pixel penality (ratio between 0 and 1, 
                                    # 1 means few missed but lot of noise)
            working_height=768,     # Working internal image size, will impact over performances
            pixel_translation=1,    # Blocks will be translated left, right up and down from this many pixels 
                                    # to see if we can find a better suited character (too big will distord 
                                    # the image and slower the process)sigma, canny_min, canny_max)
            gauss_sigma=0.3,        # Strength of the Gauss filter, reduce noise but decrease performances
            canny_min=40,           # Value threshold (0..255) for a pixel to be qualified as a weak edge
            canny_max=70,           # Value threshold (0..255) for a pixel to be qualified as a strong edge
            meanshift_r2=6.5,       # R2 parameter for the meanshift algorithm (decreases performances,
                                    # the higher the smoother)
            meanshift_d2=7.0,       # D2 parameter for the meanshift algorithm (decreases performances,
                                    # the higher the smoother)
            meanshift_n=5,          # mask size for the meanshift algorithm (decreases performances,
                                    # the higher the smoother)
            meanshift_iterations=4, # Number of passes for the meanshift algorithm 
                                    # (decreases performances, the higher the smoother)
                                    ):
        self.font = font
        self.font_subset = font_subset
        self.ascii_algorithm = ascii_algorithm
        self.color_palette = color_palette
        self.penality_white = penality_white
        self.working_height = working_height
        self.pixel_translation = pixel_translation
        self.gauss_sigma = gauss_sigma
        self.canny_min = canny_min
        self.canny_max = canny_max
        self.penality_white = penality_white
        self.meanshift_r2 = meanshift_r2
        self.meanshift_d2 = meanshift_d2
        self.meanshift_n = meanshift_n
        self.meanshift_iterations = meanshift_iterations

    @staticmethod
    def optimize(quality=5):    # give you some parameters to get the wanted quality (0..10)
        p = ConvertParameters()
        p.working_height = 256 + ((quality+1)/2)*256
        p.canny_min = quality > 2 and 40 or 0   # no low-match propagation when quality < 3
        p.pixel_translation = quality > 3 and 1 or 0
        p.gauss_sigma = quality > 2 and 0.3 or 0
        p.meanshift_r2 = quality > 5 and 6.5 or 0
        p.meanshift_d2 = quality > 5 and 7.0 or 0
        p.meanshift_n = quality > 5 and int(2+(quality-6)*1.5) or 0
        p.meanshift_iterations = quality > 5 and int(2+(quality-6)*1.5) or 0
        p.font_subset = quality > 0 and FONT_SUBSET or FONT_TINY_SUBSET
        return p


class InputImage:

    def __init__(self, data, source=""):
        self.data = data
        self.source = source

    @staticmethod
    def from_file(path):
        with open(path, "rb") as f:
            return InputImage(f.read(), source="file://%s" % (path,))

    @staticmethod
    def from_google_clipart(search):
        query = urllib.urlencode({"q" : search, "googlehost":"google.com","imgtype":"clipart","hl":"fr","isc":"white","isz":"s","safe":"off"})
        url = "http://ajax.googleapis.com/ajax/services/search/images?v=1.0&%s" % (query)
        search_results = urllib.urlopen(url)
        jsonobj = json.loads(search_results.read())
        results = jsonobj["responseData"]["results"]
        ok = False
        retries = 0
        while not ok and retries < 4:
            retries +=1
            r = results.pop(0)
            try:
                url = urllib.urlopen(r["url"])
                data = url.read()
                ok = data and url.getcode() != 404 and len(data) > 100 and not "<html" in data[:100]
            except:
                ok = False
        if not ok:
            raise ValueError("Could not get image for search %s" % (search,))
        return InputImage(data, source=url.geturl())    


    @staticmethod
    def from_google(search):
        query = urllib.urlencode({"q" : search, "googlehost":"google.com","hl":"fr","isc":"white","isz":"s","safe":"off"})
        url = "http://ajax.googleapis.com/ajax/services/search/images?v=1.0&%s" % (query)
        search_results = urllib.urlopen(url)
        jsonobj = json.loads(search_results.read())
        results = jsonobj["responseData"]["results"]
        ok = False
        retries = 0
        while not ok and retries < 4:
            retries +=1
            r = results.pop(0)
            try:
                url = urllib.urlopen(r["url"])
                data = url.read()
                ok = data and url.getcode() != 404 and len(data) > 100 and not "<html" in data[:100]
            except:
                ok = False
        if not ok:
            raise ValueError("Could not get image for search %s" % (search,))
        return InputImage(data, source=url.geturl())    

    @staticmethod
    def from_open_clipart(search):
        query = urllib.urlencode({"query" : search, "page":"1"})
        url = "http://openclipart.org/search/json/?%s" % (query,)
        search_results = urllib.urlopen(url)
        jsonobj = json.loads(search_results.read())
        results = jsonobj["payload"]
        ok = False
        retries = 0
        while not ok and retries < 4:
            retries +=1
            r = results.pop(0)
            try:
                url = urllib.urlopen(r["svg"]["png_thumb"])
                data = url.read()
                ok = data and url.getcode() != 404 and len(data) > 100 and not "<html" in data[:100]
            except:
                ok = False
        if not ok:
            raise ValueError("Could not get image for search %s" % (search,))
        return InputImage(data, source=url.geturl())  

        
    
class AaConverter:
    def __init__(self):
        me = os.path.abspath(os.path.dirname(__file__))
        oldpath = os.environ['PATH'] 
        os.environ['PATH'] = me + ';' + os.environ['PATH']
        if os.name == "nt":
            dll = cdll.LoadLibrary(os.path.join(me, "aa.dll"))
        else:
            dll = cdll.LoadLibrary(os.path.join(me, "aa.so"))
        os.environ['PATH'] = oldpath

        self.__init_font_default = dll.aa_init_font_default
        self.__init_font_default.argtypes = [c_int, c_char_p, POINTER(AaFont)]
        self.__init_font_default.restype = c_bool     

        self.__load_file = dll.aa_load_file
        self.__load_file.argtypes = [c_char_p]
        self.__load_file.restype = c_void_p    

        self.__load_memory = dll.aa_load_memory
        self.__load_memory.argtypes = [POINTER(c_byte), c_int]
        self.__load_memory.restype = c_void_p    

        self.__convert = dll.aa_convert
        self.__convert.argtypes = [c_void_p, c_int, POINTER(AaFont), POINTER(AaInternalImage), c_int, c_int, 
                c_int, c_float, c_int, c_float, c_int, c_int, c_float, c_float, c_int, c_int]
        self.__convert.restype = c_bool    
        
        self.last_font = None
        self.last_subset = None
        self.font = None
        
        

    

    def convert(self, inputimage, lines=25, params=ConvertParameters()):

        if not isinstance(inputimage, InputImage):
            raise TypeError("inputimage must be of type InputImage")

        if self.font is None or self.last_font != params.font or self.last_subset != params.font_subset:
            self.font = AaFont()
            if not self.__init_font_default(params.font, params.font_subset, byref(self.font)):
                raise ValueError("Could not create font")
            self.last_font = params.font
            self.last_subset = params.font_subset

        buff = (c_byte * len(inputimage.data)).from_buffer_copy(inputimage.data)
        image = self.__load_memory(buff, len(inputimage.data))
        if not image:
            raise ValueError("Could not parse image data")

        res = AaInternalImage()
        if not self.__convert(image, params.ascii_algorithm, self.font, byref(res),
                lines, params.working_height, params.pixel_translation, params.penality_white,
                params.color_palette, params.gauss_sigma, params.canny_min, params.canny_max,
                params.meanshift_r2, params.meanshift_d2, params.meanshift_n, params.meanshift_iterations):
            raise ValueError("Could not convert image")
        return AaImage(res.cols, res.lines, res.characters)

        


if __name__ == "__main__":

    aa = AaConverter()
    print aa.convert(InputImage.from_open_clipart(sys.argv[1]), lines=15, params=ConvertParameters.optimize(int(sys.argv[2])))
    
