import cv2
import time
import optparse
from aa import *



if __name__ == "__main__":
    usage = 'usage: %prog [options] <video.avi>'
    parser = optparse.OptionParser(usage=usage, description="""Plays a video in ascii art mode""")
    parser.add_option('-q', '--quality', dest='quality', action="store", type=int, default=3, help='Overall quality for the convertion (0-10), the higher the slower and better')
    parser.add_option('-a', '--algorithm', dest='algorithm', action="store", default="blocmindist", help='asciisation algorithm to use (blocmindist, bloc11, pixel, combined), defaults to blocmindist')
    parser.add_option('-s', '--size', dest='size', action="store", type=int, default=25, help='Height of the ascii art in characters (default is 25)')
    parser.add_option('--loop', dest='loop', action="store_true", default=False, help='Play the video in loopback')
    parser.add_option('--start', dest='start', action="store", type=int, default=0, help='Starts the video playback at this part of the video (0-1000)')
    parser.add_option('--stop', dest='stop', action="store", type=int, default=1000, help='Stops the video playback at this part of the video (0-1000)')
    parser.add_option('--sleep', dest='sleep', action="store", type=int, default=10, help='Delay in milliseconds between each redraw to go easy on CPU (video speed will stay the same but we may drop frames)')

    (options, args) = parser.parse_args()
    if len(args) != 1:
        parser.error('Incorrect number of arguments')
    
    cap = cv2.VideoCapture(args[0])
    aa = AaConverter()
    params = ConvertParameters.optimize(options.quality)
    algorithms_dic = {
        "blocmindist": AA_ALG_VECTOR_DST,
        "bloc11": AA_ALG_VECTOR_11,
        "pixel": AA_ALG_PIXEL_11,
        "combined": AA_ALG_VECTOR_OR_PIXEL,
    }
    params.ascii_algorithm = algorithms_dic.get(options.algorithm, None)
    if params.ascii_algorithm is None:
        parser.error("Unknown algorithm %s" % (repr(options.algorithm),))
    options.sleep = options.sleep/1000.0

    cap.set(cv2.CAP_PROP_POS_AVI_RATIO, options.stop/1000.0)
    stop = cap.get(cv2.CAP_PROP_POS_FRAMES)
    cap.set(cv2.CAP_PROP_POS_AVI_RATIO, options.start/1000.0)
    start = cap.get(cv2.CAP_PROP_POS_FRAMES)
    video_fpms = cap.get(cv2.CAP_PROP_FPS)/1000.0
    
    cont = True

    while cont:
        nframe = start
        cont = options.loop
        while nframe < stop:
            time_1 = int(round(time.time() * 1000))
            if not cap.set(cv2.CAP_PROP_POS_FRAMES, nframe):
                raise ValueError("Could not seek in video")
            if not cap.grab():
                raise ValueError("Could ot grab frame")
            flag, frame = cap.retrieve()
            if not flag:
                raise ValueError
            flag, buf = cv2.imencode(".bmp", frame)
            if not flag:
                raise ValueError
            inputimage = InputImage(buf)
            print "\x1b[1;1H"
            print aa.convert(inputimage, lines=options.size, params=params)
            time.sleep(options.sleep)
            #ensure that the video plays at a proper speed
            time_elapsed = int(round(time.time() * 1000)) - time_1
            to_skip = int(video_fpms*time_elapsed)
            nframe += to_skip
        nframe = start

