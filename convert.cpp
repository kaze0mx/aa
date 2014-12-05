#include <stdlib.h>
#include <stdio.h>
#include "aa.h"
#include "transform.h"

static const char* aa_alg_names[] = {"AA_ALG_PIXEL_11", "AA_ALG_VECTOR_DST", "AA_ALG_VECTOR_DST_FILL", "AA_ALG_VECTOR_11", "AA_ALG_VECTOR_11_FILL"};


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
    AaImage aaimage;

    bool bench = false;
    
    if(bench) {
        FILE* f = fopen("res.html", "w");
        fprintf(f, "<html><head></head><body>\n");
        fprintf(f, "<h1>Original image</h1>\n");
        char* imageb64 = base64(image, FIF_PNG);
        fprintf(f, "<img src = \"data:image/png;base64, %s\"/>\n", imageb64);
        delete[] imageb64;


        fprintf(f, "<h1>Ascii art</h1>\n");
        fprintf(f, "<table width = '100%%' border = '1px' padding = '5px'>\n<tr><th>modif</th>\n");
        for(int algo = 0; algo < AA_ALG_MAX; algo++) {
            if(algo==AA_ALG_VECTOR_11_FILL || algo==AA_ALG_VECTOR_DST_FILL)
                continue;        
            fprintf(f, "<th>%s</th>", aa_alg_names[algo]);
        }

        for(int fsize = 15; fsize<= 45; fsize+= 15) {
            for(int wsize = 1024; wsize <=  1024; wsize+= 512) {
                for(float penalty = 0.01; penalty <=  0.65; penalty+= 0.2) {
                    fprintf(f, "\n</tr>\n<tr><td>Working height = %d pixels, final height = %d lines, ratio full/empty = %f</td>\n", wsize, fsize, penalty);

                    for(int algo = 0; algo < AA_ALG_MAX; algo++) {
                        if(algo==AA_ALG_VECTOR_11_FILL || algo==AA_ALG_VECTOR_DST_FILL)
                            continue;
                        if(!aa_convert(image, (AaAlgorithmId)algo, &font, &aaimage, fsize, wsize, DEFAULT_TRANSLATION, penalty, AA_PAL_FREE_64, DEFAULT_CANNY_SIGMA, DEFAULT_CANNY_HYS_MIN, DEFAULT_CANNY_HYS_MAX)) {
                            fprintf(stderr, "Could not convert image\n");
                            exit(1);    
                        }
                        fprintf(f, "<td>");
                        aa_output_html(&aaimage, f, false);
                        fprintf(f, "</td>");
                        aa_unload(&aaimage);
                    }
                }
            }
        }
        fprintf(f, "\n</tr></table>\n");


        fprintf(f, "</body></html>");
        fclose(f);
    }
    else {
        //aa_convert(image, AA_ALG_VECTOR_DST, &font, &aaimage, size, 1024, DEFAULT_TRANSLATION, DEFAULT_PENALTY, AA_PAL_FREE_64, DEFAULT_CANNY_SIGMA, DEFAULT_CANNY_HYS_MIN, DEFAULT_CANNY_HYS_MAX, DEFAULT_BLUR_PASS);
        //aa_output_ascii(&aaimage, stdout);
        aa_convert(image, AA_ALG_PIXEL_11, &font, &aaimage, size, 1024*1.5, 1, 0.4, AA_PAL_ANSI_16, DEFAULT_CANNY_SIGMA, DEFAULT_CANNY_HYS_MIN, DEFAULT_CANNY_HYS_MAX);
        aa_output_ascii(&aaimage, stdout);
        aa_convert(image, AA_ALG_VECTOR_DST, &font, &aaimage, size, 1024, 1, 0.6, AA_PAL_NONE, DEFAULT_CANNY_SIGMA, DEFAULT_CANNY_HYS_MIN, DEFAULT_CANNY_HYS_MAX);
        aa_output_ascii(&aaimage, stdout);
        aa_unload(&aaimage);
    }
    return 0;
}

