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
		fprintf(stderr, "Usage: %s <image_path> [num_ascii_lines] [quality 0-10] [mode (i,j,h or a)]\n", argv[0]);
		exit(1);
	} 
	char* filename = argv[1];
    bool res;
	
    FIBITMAP* image = aa_load_bitmap_from_file(argv[1]);
    FREE_IMAGE_TYPE type = FreeImage_GetImageType(image);
    if(image==NULL) {
		fprintf(stderr, "Could not open image\n");
		exit(1);
    }

    // parameters
    unsigned int ascii_num_lines = 25;
    unsigned int quality = 5;
    unsigned char mode = 'i';   // i = image, j = json (video-enabled), h = html(video enabled), a = ansii (video-enabled)
    if(argc >= 3) {
        ascii_num_lines = strtol(argv[2], NULL, 10);
    }
    if(argc >= 4) {
        quality = strtol(argv[3], NULL, 10);
        if(quality > 10) {
            fprintf(stderr, "Quality parameter must be between 0 and 10\n");
            exit(1);
        }
    }
    if(argc >= 5) {
        mode = argv[4][0];
        if ( mode != 'i' && mode != 'a' && mode != 'j' && mode != 'h' ) {
            fprintf(stderr, "Mode parameter must be i, j, h or a\n");
            exit(1);
        }
    }
    unsigned int working_height = 256 + (quality/2)*256;
    unsigned int canny_min = quality > 2?DEFAULT_CANNY_HYS_MIN:0;   // no low-match propagation when quality < 4
    unsigned int canny_max = DEFAULT_CANNY_HYS_MAX;
    unsigned int ascii_translation = quality > 3?1:0;
    float ascii_black_white_penalty_ratio = DEFAULT_PENALTY;
    unsigned int prefilter_gauss_sigma = quality > 2?DEFAULT_SIGMA:0;
    unsigned int meanshift_r2 = quality > 5?DEFAULT_MEANSHIFT_R2:0;
    unsigned int meanshift_d2 = quality > 5?DEFAULT_MEANSHIFT_D2:0;
    unsigned int meanshift_n = quality > 5?2+(quality-6)*1.5:0;
    unsigned int meanshift_iterations = quality > 5?2+(quality-6)*1.5:0;
    const char* alphabet = FONT_SUBSET;
    if ( quality == 0 )
        alphabet = FONT_TINY_SUBSET;
    else if ( quality > 4 ) {
         alphabet = FONT_MEGA_SUBSET;
    }

    AaFont font;
    if(!aa_init_font_default(AA_FT_COURIER, alphabet, &font)) {
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
    
    if ( mode == 'i' ) {
        // Convert single image and output to stdout
        AaImage aaimage;
        aa_convert(image, AA_ALG_VECTOR_DST, &font, &aaimage, ascii_num_lines, working_height, ascii_translation, ascii_black_white_penalty_ratio, AA_PAL_NONE, prefilter_gauss_sigma, canny_min, canny_max, meanshift_r2, meanshift_d2, meanshift_n, meanshift_iterations);
        aa_output_ascii(&aaimage, stdout);
        aa_dispose_image(&aaimage);
        aa_dispose_bitmap(image);
    }
    else {  
        char tmp[1024];
        // create video in filename.ans and filename.html
        int start = 0;
        int skip = 0;
        //num_images = start+100;
        int waiting = 300;
        bool got_transparent_on_first_frame = false;
        
        AaPaletteId palette_id = AA_PAL_NONE;
        if ( mode == 'a' ) {
            printf("\x1b[2J\x1b[1;1H\x1b[?25l");
            palette_id = AA_PAL_ANSI_16;
        }
        else if ( mode == 'h' ) {
            printf("%s", video_html_pre);
            palette_id = AA_PAL_NONE;
        }
        FIBITMAP* previous = NULL;
        for(int i = start, j = 0; i < num_images; i += 1+skip, j++) {
            fprintf(stderr, "Converting image %d/%d ...\n", i, num_images);
            FIBITMAP* frame;
            if ( num_images > 1 )
                frame = FreeImage_LockPage(video, i);
            else
                frame = image;
            int width = FreeImage_GetWidth(frame);
            int height = FreeImage_GetHeight(frame);
            int pitch = FreeImage_GetPitch(frame);
            int surface = pitch*height;
            WORD bpp = FreeImage_GetBPP(frame);
            bool transparent = FreeImage_IsTransparent(frame);
            if ( previous != NULL ) {
                if ( transparent && bpp == 8 ) {
                    // the transparent color in animated gifs means: use the last bitmap's color
                    BYTE bgcolor = FreeImage_GetTransparentIndex(frame);
                    BYTE* buffer = FreeImage_GetBits(frame);
                    BYTE* prev = FreeImage_GetBits(previous);
                    for ( int i=0; i < surface; i++ ) {
                         if ( buffer[i] == bgcolor ) {
                             if( got_transparent_on_first_frame )
                                 buffer[i] = 0;
                             else
                                 buffer[i] = prev[i];
                         }
                     }
                }
            }
            else {
                // first frame
                if ( transparent && bpp == 8 ) {
                    // the transparent color in animated gifs means: use the last bitmap's color
                    BYTE bgcolor = FreeImage_GetTransparentIndex(frame);
                    BYTE* buffer = FreeImage_GetBits(frame);
                    for ( int i=0; i < surface && !got_transparent_on_first_frame; i++ ) {
                        if ( buffer[i] == bgcolor ) { 
                            got_transparent_on_first_frame = true;

                        }
                    }
                }
            }
                
            AaImage aaimage;
            if ( !aa_convert(frame, AA_ALG_VECTOR_DST, &font, &aaimage, ascii_num_lines, working_height, ascii_translation, ascii_black_white_penalty_ratio, palette_id, prefilter_gauss_sigma, canny_min, canny_max, meanshift_r2, meanshift_d2, meanshift_n, meanshift_iterations) ) {
                fprintf(stderr, "Could not convert image\n");
                exit(2);
            }

            if ( mode == 'h' ) {
                printf("<div id = 'frame_%d' style = 'display:none'>\n", j);
                aa_output_html(&aaimage, stdout, false);
                printf("</div>\n");
            }
            else if ( mode == 'a' ) {
                aa_output_ansi16(&aaimage, stdout);
                printf("\x1b[1;1H");
                for(int k = 0;k<waiting;k++) {
                    printf("\x1b[1H");
                }
            }

            if ( previous != NULL )
                FreeImage_Unload(previous);
            aa_dispose_image(&aaimage);
            if ( num_images > 1 ) {
                previous = FreeImage_Clone(frame);
                FreeImage_UnlockPage(video, frame, false);
            }
        }
        if ( previous != NULL )
            FreeImage_Unload(previous);
        if ( mode == 'a' )
            printf("\x1b[?25h");
        else if ( mode == 'h' )
            printf("%s", video_html_post);
    }
    aa_dispose_font(&font);
    return 0;
}

