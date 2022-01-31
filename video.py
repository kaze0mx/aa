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
    parser.add_option('-t', '--status', dest='status', action="store_true", default=False, help='Add a status line at the bottom of the screen')
    parser.add_option('--loop', dest='loop', action="store_true", default=False, help='Play the video in loopback')
    parser.add_option('--start', dest='start', action="store", type=int, default=0, help='Starts the video playback at this part of the video (0-1000)')
    parser.add_option('--stop', dest='stop', action="store", type=int, default=1000, help='Stops the video playback at this part of the video (0-1000)')
    parser.add_option('--cpu', dest='cpu', action="store", type=int, default=100, help='CPU max charge in % (defaults to 100)')
    parser.add_option('--fps', dest='fps', action="store", type=int, default=0, help='Play the video at this framerate (overwrites video\'s FPS)')
    parser.add_option('--dps', dest='dps', action="store", type=int, default=0, help='Draw this many frames per second (may sleep or skip frames)')
    parser.add_option('--noskip', dest='noskip', action="store_true", default=False, help='Realtime mode, i.e do not skip any frame (cannot guarantee anything about the framerate then). Some video format also do not support skipping frames')

    (options, args) = parser.parse_args()
    if len(args) != 1:
        parser.error('Incorrect number of arguments')
    
    if args[0] == "webcam":
        cap = cv2.VideoCapture(0)
    else:
        if not os.path.exists(args[0]):
            raise ValueError("Could not open file %s" % args[0])
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

    try:
        cap.set(cv2.CAP_PROP_POS_AVI_RATIO, options.stop/1000.0)
        stop = cap.get(cv2.CAP_PROP_POS_FRAMES)
    except: 
        stop = 0
    if stop == 0:
        if options.noskip:
            stop = None
        else:
            raise ValueError("Could not seek in video, maybe realtime ? try with --noskip option")
    if stop < 0:        # webcam
        stop = None
    if options.start:
        cap.set(cv2.CAP_PROP_POS_AVI_RATIO, options.start/1000.0)
        start = cap.get(cv2.CAP_PROP_POS_FRAMES)
    else:
        start = 0
    if start < 0:       # webcam
        start = 0
    try:
        video_fps = options.fps or cap.get(cv2.CAP_PROP_FPS) or 24
    except:
        video_fps = 24
    if options.dps:
        frame_ideal_time = 1.0/options.dps
    else:
        frame_ideal_time = 1.0/video_fps
    cpu_rate = options.cpu/100.0 or 1.0
    cont = True
    
    while cont:
        nframe = start
        cont = options.loop
        while stop is None or nframe < stop:
            time_1 = time.perf_counter()
            if not options.noskip:
                if not cap.set(cv2.CAP_PROP_POS_FRAMES, int(nframe)):
                    raise ValueError("Could not seek in video, maybe realtime ? try with --noskip option")
            if not cap.grab():
                raise ValueError("Could not grab frame")
            flag, frame = cap.retrieve()
            if not flag:
                raise ValueError
            flag, buf = cv2.imencode(".bmp", frame)
            if not flag:
                raise ValueError
            inputimage = InputImage(buf)
            print("\x1b[1;1H")
            print(aa.convert(inputimage, lines=options.size, params=params))
            time_elapsed = time.perf_counter() - time_1
           
            # ensure that the video plays at a proper speed
            render_fps = 1.0/time_elapsed
            if time_elapsed < frame_ideal_time:
                to_sleep = frame_ideal_time - time_elapsed
                render_charge = int(100.0*time_elapsed/(time_elapsed+to_sleep))
                if options.cpu != 100:
                    to_sleep = max(to_sleep, (time_elapsed-cpu_rate*time_elapsed)/cpu_rate)
            else:
                if options.cpu != 100:
                    to_sleep = (time_elapsed-cpu_rate*time_elapsed)/cpu_rate
                else:
                    to_sleep = 0
                render_charge = 100
            # tempo for respecting the video's fps and/or specified dps
            if to_sleep:
                time.sleep(to_sleep)
            time_elapsed = time.perf_counter() - time_1
            real_fps = 1.0/time_elapsed
            real_charge = int(100.0*time_elapsed/(time_elapsed+to_sleep))
            # frame skip
            to_skip = video_fps*time_elapsed
            if to_skip < 1 or options.noskip:
                to_skip = 1
            nframe += to_skip
            if options.status:
                print("[%s] -- %3.2fDPS (%3d%%) -> %2.2fSLP+%2.2fSKP -> %3.2fFPS (%3d%%)" % (args[0], render_fps, render_charge, to_sleep, to_skip-1, real_fps, real_charge))
        nframe = start

