#include <stdlib.h>
#include <stdio.h>
#include "aa.h"
#include "transform.h"

static const char* aa_alg_names[] = {"AA_ALG_PIXEL_11", "AA_ALG_VECTOR_DST", "AA_ALG_VECTOR_DST_FILL", "AA_ALG_VECTOR_11", "AA_ALG_VECTOR_11_FILL"};

const char* video_html_pre = ""
#include "video_pre.h"
;
const char* video_html_post = ""
#include "video_post.h"
;


int main(int argc, char** argv) {
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
    FIBITMAP* image = aa_load_file(argv[1]);
    FREE_IMAGE_TYPE type = FreeImage_GetImageType(image);
    if(image==NULL) {
		fprintf(stderr, "Could not open image\n");
		exit(1);
    }

    int size = 25;
    if(argc>= 3) {
        size = strtol(argv[2], NULL, 10);
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
    
    if ( num_images == 1 ) {
        // Convert single image and output to stdout
        AaImage aaimage;
        aa_convert(image, AA_ALG_VECTOR_DST, &font, &aaimage, size, 1024, 1, DEFAULT_PENALTY, AA_PAL_NONE, DEFAULT_SIGMA, DEFAULT_CANNY_HYS_MIN, DEFAULT_CANNY_HYS_MAX);
        aa_output_ascii(&aaimage, stdout);
        aa_unload(&aaimage);
    }
    else {
        char tmp[1024];
        // create video in filename.ans and filename.html
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
        FIBITMAP* previous = NULL;
        for(int i = start, j = 0; i < num_images; i += 1+skip, j++) {
            fprintf(stderr, "** Converting image %d/%d\n", i, num_images);
            FIBITMAP* frame = FreeImage_LockPage(video, i);
            if ( previous != NULL ) {
                int width = FreeImage_GetWidth(frame);
                int height = FreeImage_GetHeight(frame);
                int pitch = FreeImage_GetPitch(frame);
                int surface = pitch*height;
                WORD bpp = FreeImage_GetBPP(frame);
                bool transparent = FreeImage_IsTransparent(frame);
                if ( transparent && bpp == 8 ) {
                    // the transparent color in animated gifs means: use the last bitmap's color
                    BYTE bgcolor = FreeImage_GetTransparentIndex(frame);
                    BYTE* buffer = FreeImage_GetBits(frame);
                    BYTE* prev = FreeImage_GetBits(previous);
                    for ( int i=0; i < surface; i++ ) {
                         if ( buffer[i] == bgcolor )
                             buffer[i] = prev[i];
                     }
                }
            }
            AaImage aaimage;
            aa_convert(frame, AA_ALG_VECTOR_DST, &font, &aaimage, size, 768, DEFAULT_TRANSLATION, DEFAULT_PENALTY, AA_PAL_NONE, DEFAULT_SIGMA, DEFAULT_CANNY_HYS_MIN, DEFAULT_CANNY_HYS_MAX);

            fprintf(outhtml, "<div id = 'frame_%d' style = 'display:none'>\n", j);
            aa_output_html_mono(&aaimage, outhtml);
            fprintf(outhtml, "</div>\n");

            aa_output_ansi16(&aaimage, outansi);
            fprintf(outansi, "\x1b[1;1H");
            for(int k = 0;k<waiting;k++) {
                fprintf(outansi, "\x1b[1H");
            }
            if ( previous != NULL )
                FreeImage_Unload(previous);
            previous = FreeImage_Clone(frame);
            aa_unload(&aaimage);
            FreeImage_UnlockPage(video, frame, false);
            fflush(outansi);
            fflush(outhtml);
        }
        if ( previous != NULL )
            FreeImage_Unload(previous);
        fprintf(outansi, "\x1b[?25h");
        fprintf(outhtml, "%s", video_html_post);
        fclose(outansi);
        fclose(outhtml);
    }
    return 0;
}

