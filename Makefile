
CC=g++ -g
LNK=g++ -g

FreeImage=.

OBJS=transform.o aa.o

all: aaconvert.exe aa.dll

%.o: %.cpp %.h 
	$(CC) -I. -c $< -o $@

%.o: %.cpp 
	$(CC) -I. -c $< -o $@

convert.o: convert.cpp video_pre.h video_post.h aa.h 
	$(CC) -I. -c $< -o $@

aaconvert.exe: $(OBJS) convert.o
	$(LNK) -static-libstdc++ -static-libgcc -o $@ -L$(FreeImage) $(OBJS) convert.o -lFreeImage 

aa.dll: $(OBJS) 
	$(LNK) -static-libstdc++ -static-libgcc -s -shared -o $@ -L$(FreeImage) $(OBJS)  -lFreeImage 

video_pre.h: video_pre.html
	python raw2h.py video_pre.html > video_pre.h

video_post.h: video_post.html
	python raw2h.py video_post.html > video_post.h

clean:
	rm -f $(OBJS) aaconvert.exe convert.o 

