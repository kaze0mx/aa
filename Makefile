
CC=g++ -g -O9
LNK=g++ -g

FreeImage=.

OBJS=transform.o aa.o

all: convert.exe 

%.o: %.cpp %.h 
	$(CC) -I. -c $< -o $@

%.o: %.cpp 
	$(CC) -I. -c $< -o $@

convert.o: convert.cpp video_pre.h video_post.h aa.h 
	$(CC) -I. -c $< -o $@

convert.exe: $(OBJS) convert.o
	$(LNK) -static-libstdc++ -static-libgcc -o convert.exe -L$(FreeImage) $(OBJS) convert.o -lFreeImage 

video.exe: $(OBJS) video.o 
	$(LNK) -static-libstdc++ -static-libgcc -o video.exe -L$(FreeImage) $(OBJS) video.o -lFreeImage 

video_pre.h: video_pre.html
	python raw2h.py video_pre.html > video_pre.h

video_post.h: video_post.html
	python raw2h.py video_post.html > video_post.h

clean:
	rm -f $(OBJS) convert.exe convert.o 

