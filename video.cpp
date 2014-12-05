#include "aa.h"
#include <stdlib.h>
#include <stdio.h>


const char* video_html_pre = ""
#include "video_pre.h"
;
const char* video_html_post = ""
#include "video_post.h"
;

int main(int argc, char** argv) {
	char tmp[256];

	if(argc < 2)  {
		fprintf(stderr, "No image name provided\n");
		exit(1);
	}

    char* filename = argv[1];
    bool res;
	AaFont font;
    if(!aa_init_font_default(AA_FT_CONSOLA, FONT_SUBSET, &font)) {
		fprintf(stderr, "Could not initialize font\n");
		exit(1);
	}

    FIMULTIBITMAP* video;
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileType(filename, 0);
	if(fif == FIF_UNKNOWN) {
		fif = FreeImage_GetFIFFromFilename(filename);
	}
	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		video = FreeImage_OpenMultiBitmap(fif, filename, false, true, true, 0);
	}
    if(video == NULL) {
        fprintf(stderr, "Could not open image file");
        exit(1);
    }
    
    int num_images = FreeImage_GetPageCount(video);
    fprintf(stderr, "Video has %d images\n", num_images);

	bool ansi = true;
	int start = 0;
	int skip = 0;
	//num_images = start+100;
	int waiting = 300;
	
	sprintf(tmp, "%s.ans", filename);
	FILE* outansi = fopen(tmp, "w");
	sprintf(tmp, "%s.html", filename);
	FILE* outhtml = fopen(tmp, "w");
	fprintf(outansi, "\x1b[2J\x1b[1;1H\x1b[?25l");
	fprintf(outhtml, "%s", video_html_pre);
	for(int i = start, j = 0; i < num_images; i += 1+skip, j++) {
		fprintf(stderr, "** Converting image %d/%d\n", i, num_images);
		FIBITMAP* frame = FreeImage_LockPage(video, i);
		AaImage aaimage;

		aa_convert(frame, AA_ALG_VECTOR_DST, &font, &aaimage, 40, DEFAULT_WORKING_HEIGHT, DEFAULT_TRANSLATION, DEFAULT_PENALTY, AA_PAL_NONE, DEFAULT_CANNY_SIGMA, DEFAULT_CANNY_HYS_MIN, DEFAULT_CANNY_HYS_MAX);

		fprintf(outhtml, "<div id = 'frame_%d' style = 'display:none'>\n", j);
		aa_output_html_mono(&aaimage, outhtml);
		fprintf(outhtml, "</div>\n");

		aa_output_ansi16(&aaimage, outansi);
		fprintf(outansi, "\x1b[1;1H");
		for(int k = 0;k<waiting;k++) {
			fprintf(outansi, "\x1b[1H");
		}
		aa_unload(&aaimage);
		FreeImage_UnlockPage(video, frame, false);
		fflush(outansi);
		fflush(outhtml);
	}
	fprintf(outansi, "\x1b[?25h");
	fprintf(outhtml, "%s", video_html_post);
	fclose(outansi);
	fclose(outhtml);
}
