INSTALL
=======

Linux
-----

1) Install libfreeimage-dev
2) make -f Makefile.linux

Windows
-------

1) Download FreeImage from http://freeimage.sourceforge.net/download.html
2) Copy .lib, .h and .dll in this directory
3) Install Mingw compiler (hi cygwin)
4) make

Run
===

1) aaconvert (executable) 
Debug tool to convert a single image, not a lot of options.
aaconvert <image_path> [num_ascii_lines] [quality 0-10] [mode (i,j,h or v)]

2) aalib.so / aalib.dll
shared library, use it in your program !

3) aa.py
Python bindings, use the shared library (just leave the .so/.dll in the same dir). More options, also a command line tool:

Usage: aa.py [options] <what>

Draws an image in compact ascii art

Options:
  -h, --help            show this help message and exit
  -f, --file            The command line argument is a filepath
  -u, --url             The command line argument is an url
  -g, --google          The command line argument is a google image search
  -i, --googleclipart   The command line argument is a google clipart search
  -c, --openclipart     The command line argument is a openclipart image
                        search
  -s SIZE, --size=SIZE  Height of the ascii art in characters (default is 32)
  -q QUALITY, --quality=QUALITY
                        Overall quality for the convertion (0-10), the higher
                        the slower and better
  -a ALGORITHM, --algorithm=ALGORITHM
                        asciisation algorithm to use (blocmindist, bloc11,
                        pixel, combined), defaults to blocmindist

4) video.py
Tool to convert videos on the fly in ascii art. Requires opencv2 for python (sudo apt-get install python-opencv). Uses aa.py

Usage: video.py [options] <video.avi>

Plays a video in ascii art mode

Options:
  -h, --help            show this help message and exit
  -q QUALITY, --quality=QUALITY
                        Overall quality for the convertion (0-10), the higher
                        the slower and better
  -a ALGORITHM, --algorithm=ALGORITHM
                        asciisation algorithm to use (blocmindist, bloc11,
                        pixel, combined), defaults to blocmindist
  -s SIZE, --size=SIZE  Height of the ascii art in characters (default is 25)
  -t, --status          Add a status line at the bottom of the screen
  --loop                Play the video in loopback
  --start=START         Starts the video playback at this part of the video
                        (0-1000)
  --stop=STOP           Stops the video playback at this part of the video
                        (0-1000)
  --cpu=CPU             CPU max charge in % (defaults to 100)
  --fps=FPS             Play the video at this framerate (overwrites video's
                        FPS)
  --dps=DPS             Draw this many frames per second (may sleep or skip
                        frames)


