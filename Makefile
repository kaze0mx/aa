
CC=g++ 
LNK=g++

FreeImage=.

OBJS=transform.o aa.o

all: convert.exe video.exe

%.o: %.cpp %.h 
	$(CC) -I. -c -g $< -o $@

%.o: %.cpp 
	$(CC) -I. -c $< -o $@

convert.exe: $(OBJS) convert.o
	$(LNK) -static-libstdc++ -static-libgcc -o convert.exe -L$(FreeImage) $(OBJS) convert.o -lFreeImage 

video.exe: $(OBJS) video.o 
	$(LNK) -static-libstdc++ -static-libgcc -o video.exe -L$(FreeImage) $(OBJS) video.o -lFreeImage 

video.o: video.cpp video_pre.h video_post.h
	$(CC) -I. -c $< -o $@

video_pre.h: video_pre.html
	python raw2h.py video_pre.html > video_pre.h

video_post.h: video_post.html
	python raw2h.py video_post.html > video_post.h

clean:
	rm -f $(OBJS) convert.exe video.exe convert.o video.o 

up:	res.html
	pscp res.html kaze@rhino9.fr:/volume1/homes/kaze/www/res.html
