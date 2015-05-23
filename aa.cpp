#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "aa.h"
#include "transform.h"

#define PALETTE_MAX_SIZE 256

BYTE font_consola[] = "" 
#include "consola.h"
;
BYTE font_lucon[] = "" 
#include "lucon.h"
;
BYTE font_cour[] = "" 
#include "cour.h"
;

static AaColor pal_mono[] = { {0,0,0}, {255,255,255} };
static AaColor pal_ansi16[] = {{0,0,0}, {128,0,0}, {0,128,0}, {128,128,0}, {0,0,128}, {128,0,128}, {0,128,128}, {192,192,192}, {127,127,127}, {255,0,0}, {0,255,0}, {255,255,0}, {0,0,255}, {255,0,255}, {0,255,255}, {255,255,255}};

static AaPalette hardcoded_palettes[] = { \
    { pal_mono, 2 }, \
    { pal_mono, 2 }, \
    { pal_ansi16, 16 }, \
    { NULL, 2 }, \
    { NULL, 16 }, \
    { NULL, 64 }, \
    { NULL, 256 } \
};


bool GenericWriter(FIBITMAP* dib, const char* lpszPathName, int flag) {
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	BOOL bSuccess = FALSE;

	if(dib) {
		// try to guess the file format from the file extension
		fif = FreeImage_GetFIFFromFilename(lpszPathName);
		if(fif != FIF_UNKNOWN ) {
			// check that the plugin has sufficient writing and export capabilities ...
			WORD bpp = FreeImage_GetBPP(dib);
			if(FreeImage_FIFSupportsWriting(fif) && FreeImage_FIFSupportsExportBPP(fif, bpp)) {
				// ok, we can save the file
				bSuccess = FreeImage_Save(fif, dib, lpszPathName, flag);
				// unless an abnormal bug, we are done !
			}
		}
	}
	return (bSuccess == TRUE) ? true : false;
}





void propagate_distance_bloc(int* matrix, int x, int y, int ind) {
	int val = matrix[ind];
	for(int dy = -1, ry = -PASX; dy<= 1;dy++, ry+= PASX) {
		for(int dx = -1; dx<= 1;dx++) {
			if(x+dx >=  PASX || x+dx < 0)
				continue;
			if(y+dy >=  PASY || y+dy < 0)
				continue;
			if(dx==0 && dy==0)
				continue;
			int nind = ind+ry+dx;
			int lval = matrix[nind];
			if(lval > val+1 && val < PASX) {
				matrix[nind] = val+1;
				propagate_distance_bloc(matrix, x+dx, y+dy, nind);
			}
		}
	}
}


void compute_matrix_bloc(BYTE* bloc, int pitch, int* matrix_full, int* matrix_empty) {
	BYTE* b = bloc;
	for(int y = 0, ind = 0;y<PASY;y++, b+= pitch-PASX) {
		for(int x = 0;x<PASX;x++, ind++, b++) {
			if(*b < 128) {
				matrix_full[ind] = PASX;
				matrix_empty[ind] = 0;
			}
			else {
				matrix_full[ind] = 0;
				matrix_empty[ind] = PASX;			
			}
		}
	}
	for(int y = 0, ind = 0;y<PASY;y++) {
		for(int x = 0;x<PASX;x++, ind++) {
			if(matrix_full[ind]==0) {
				propagate_distance_bloc(matrix_full, x, y, ind);
			}
			else if(matrix_empty[ind]==0){		
				propagate_distance_bloc(matrix_empty, x, y, ind);
			}
		}
	}	
}


bool aa_init_font(FIBITMAP* font, const char* font_carmap, const char* subset, AaFont* res) {
	FreeImage_FlipVertical(font);
	res->font = FreeImage_ConvertToGreyscale(font);
    res->font = threshold(res->font, THRESHOLD_FONT);
	res->font_buffer = FreeImage_GetBits(res->font);
	res->font_buffer_pitch = FreeImage_GetPitch(res->font);
	res->font_carmap = font_carmap;
	res->font_carmap_len = strlen(font_carmap);
    res->font_subset_len = strlen(subset);
    res->font_subset = new int[res->font_subset_len];
	res->font_subset_matrix_full = new int[res->font_subset_len*PASX*PASY];
	res->font_subset_matrix_empty = new int[res->font_subset_len*PASX*PASY];
	for(int i = 0, curchr = 0, curmat = 0;i<res->font_subset_len;i++, curchr+= PASX, curmat+= PASX*PASY) {
        const char* indice = strchr(font_carmap, subset[i]);
		if(indice==NULL) {
			return false;
		}
		res->font_subset[i] = indice-font_carmap;
		compute_matrix_bloc(res->font_buffer+(indice-font_carmap)*PASX, res->font_buffer_pitch, res->font_subset_matrix_full+curmat, res->font_subset_matrix_empty+curmat);
	}
    return true;
}

void aa_dispose_font(AaFont* res) {
    FreeImage_Unload(res->font);
    delete[] res->font_subset;
    delete[] res->font_subset_matrix_full;
    delete[] res->font_subset_matrix_empty;
}

bool aa_init_font_default(AaFontId id, const char* subset, AaFont* res) {
    FIBITMAP* myfont = NULL;
    switch(id) {
        case AA_FT_CONSOLA:
            myfont = aa_load_bitmap_from_memory(font_consola, sizeof(font_consola)-1);
            break;
        case AA_FT_LUCIDA:
            myfont = aa_load_bitmap_from_memory(font_lucon, sizeof(font_consola)-1);
            break;
        case AA_FT_COURIER:
            myfont = aa_load_bitmap_from_memory(font_cour, sizeof(font_consola)-1);
            break;
    }
    if(myfont==NULL) {
        fprintf(stderr, "Could not open font file");
        return false;
    }
    bool ret = aa_init_font(myfont, FONT_CARMAP, subset, res);
    FreeImage_Unload(myfont);
    return ret;
}

bool aa_init_font_from_picture(const char* font_picture_path, const char* font_carmap, const char* subset, AaFont* res) {
    FIBITMAP* myfont = aa_load_bitmap_from_file(font_picture_path);
    if(myfont==NULL) {
        fprintf(stderr, "Could not open font file");
        return false;
    }
    bool ret = aa_init_font(myfont, font_carmap, subset, res);
    FreeImage_Unload(myfont);
    return ret;
}

bool aa_dispose_image(AaImage* image) {
    delete[] image->characters;
    delete[] image->colors;
    delete[] image->palette->raw;
    delete image->palette;
    return true;
}



void display_bloc(BYTE* bloc, int image_width) {
	for(int y = 0;y<PASY;y++) {
		for(int x = 0;x<PASX;x++, bloc++) {
			if(*bloc > 200)
				printf("@");
			else if(*bloc > 150)
				printf("#");
			else if(*bloc > 50)
				printf("o");
			else
				printf(".");
		}
		printf("\n");
		bloc+= image_width-PASX;
	}
}

void display_matrix(int* bloc) {
	for(int y = 0;y<PASY;y++) {
		for(int x = 0;x<PASX;x++, bloc++) {
			printf("%d", *bloc);
		}
		printf("\n");
	}
}

float diff_bloc_simple(BYTE* bloc1, BYTE* bloc2, int bloc1_image_width, int bloc2_image_width, float penalty, float discard_if_greater)
{
	float error = 0;
	for(int y = 0;y<PASY;y++, bloc1+= bloc1_image_width-PASX, bloc2+= bloc2_image_width-PASX) {
		for(int x = 0;x<PASX;x++, bloc1++, bloc2++) {
			if (abs(*bloc1-*bloc2) > 128) {
				if(*bloc1 < 128)
					error+= (1-penalty);
				else
					error+= penalty;
			}
		}
		if(error>discard_if_greater)
			return error;
	}
	return error;
}



char get_best_char(FIBITMAP* image, AaFont* font, int x, int y, int translation, float penalty) {
	int image_width = FreeImage_GetWidth(image);
	int image_height = FreeImage_GetHeight(image);
	int pitch = FreeImage_GetPitch(image);
	BYTE* buffer = FreeImage_GetBits(image);
	float minerr = 1000000000;
	char best = ' ';

	for(int dy = -translation; dy<= translation; dy++) {
		if(y+dy < 0 || y+dy > image_height-PASY)
			continue;
		for(int dx = -translation; dx<= translation; dx++) {
			if(x+dx < 0 || x+dx >image_width-PASX)
				continue;
			BYTE* bloc = &buffer[(y+dy)*pitch+x+dx];
			for(int ii = 0;ii<font->font_subset_len;ii++) {
				int i = font->font_subset[ii];
				BYTE* bloc_c = font->font_buffer+PASX*i;
				float err = diff_bloc_simple(bloc, bloc_c, pitch, font->font_buffer_pitch, penalty, minerr);
				if(err<minerr) {
					minerr = err;
					best = font->font_carmap[i];
					if(err == 0)
						return best;
				}
			}
		}
	}

	return best;
}

char get_best_char_distance(FIBITMAP* image, AaFont* font, int x, int y, int translation, float penalty) {
	int image_width = FreeImage_GetWidth(image);
	int image_height = FreeImage_GetHeight(image);
	int pitch = FreeImage_GetPitch(image);
	BYTE* buffer = FreeImage_GetBits(image);
	float minerr = 1000000000;
	char best = ' ';
	int bestind;

	for(int dy = -translation; dy<= translation; dy++) {
		if(y+dy < 0 || y+dy > image_height-PASY)
			continue;
		for(int dx = -translation; dx<= translation; dx++) {
			if(x+dx < 0 || x+dx > image_width-PASX)
				continue;
			BYTE* bloc = &buffer[(y+dy)*pitch+x+dx];
			BYTE* obloc = bloc;
			int mat_ind = 0;
#ifdef VDEBUG
			printf("--------NEWCAR--------\n");
			display_bloc(obloc, pitch);
#endif			
			int matbloc_full[PASX*PASY];
			int matbloc_empty[PASX*PASY];
			compute_matrix_bloc(obloc, pitch, matbloc_full, matbloc_empty);
			for(int ii = 0;ii<font->font_subset_len;ii++, mat_ind+= PASX*PASY) {
				float err = 0;
				bloc = obloc;
#ifdef VDEBUG
				printf("\n--------TEST  %c ---------\n", font->font_carmap[font->font_subset[ii]]);
#endif				
				for(int col = 0, dep = 0;col<PASY && err < minerr;col++, bloc+= pitch-PASX) {
					for(int lin = 0;lin<PASX && err<minerr;lin++, dep++, bloc++) {
                        if(font->font_subset_matrix_full[mat_ind+dep]==0) {
                            int d = matbloc_full[dep]*font->font_subset_matrix_empty[mat_ind+dep];
                            d=d*d;
                            err+= (1.0-penalty)*d;
#ifdef VDEBUG
                            printf("%d", d);
#endif							
                        }
                        else if(*bloc > 128) {
                            int d = font->font_subset_matrix_full[mat_ind+dep]*matbloc_empty[dep];
                            d=d*d;
                            err+= penalty*d;
#ifdef VDEBUG							
                            printf("%d", d);
#endif							
                        }
#ifdef VDEBUG							
                        else {
                            printf(" ");
                        }
#endif
					}
#ifdef VDEBUG					
					printf("\n");
#endif					
				}
#ifdef VDEBUG
				printf("%c = %2.2f\n", font->font_carmap[font->font_subset[ii]], err);
#endif				
				if(err<minerr) {
					minerr = err;
					bestind = mat_ind;
					best = font->font_carmap[font->font_subset[ii]];
					if(err == 0)
						return best;
				}
			}
#ifdef VDEBUG
			printf("---------RESULT-----------------\n");
			display_bloc(obloc, pitch);
			printf("\n");
			display_matrix(&font->font_subset_matrix_empty[bestind]);
			printf("--------------------------------\n\n\n\n");
#endif
		}
	}
	return best;
}

BYTE get_best_color(FIBITMAP* paletized, int x, int y) {
	int pitch = FreeImage_GetPitch(paletized);
	int palette_size = FreeImage_GetColorsUsed(paletized);
	BYTE* buffer_palette = FreeImage_GetBits(paletized);
	BYTE* blocp = &buffer_palette[y*pitch+x];
	BYTE histo[PALETTE_MAX_SIZE];
	for(int i = 0;i<palette_size;i++)
		histo[i] = 0;
	for(int col = 0, dep = 0;col<PASY;col++, blocp+= pitch-PASX) {
		for(int lin = 0;lin<PASX;lin++, blocp++) {
			histo[*blocp]+= 1;
		}
	}
	int max = 0;
	BYTE best = 0;
	for(int i = 0;i<palette_size;i++) {
		if(histo[i]>max) {
			max = histo[i];
			best = i;
		}
	}
	return best;
}

/*
 *
 */
bool aa_convert(FIBITMAP* image, AaAlgorithmId algorithm, AaFont* font, AaImage* res, int lines, int working_height, int translation, float penalty, AaPaletteId palette_id, float sigma, int canny_min, int canny_max, float meanshift_r2, float meanshift_d2, int meanshift_n, int meanshift_iterations) {
	int real_width = FreeImage_GetWidth(image);
	int real_height = FreeImage_GetHeight(image);
    int real_bpp = FreeImage_GetBPP(image);
    int working_width;
    bool greyscale = false;
    bool rgb = false;
#ifdef DEBUG
	GenericWriter(image, "original.png", 0);
#endif
	
    // preprocessing
    // resize to working size
	FIBITMAP* preprocessed = NULL;
	if(working_height && working_height < real_height) {
		int height = working_height;
		float percent = (height/float(real_height));
		int width = int(real_width*percent);
#ifdef DEBUG
		fprintf(stderr, "Resizing image to %d*%d\n", width, height);
#endif  
		preprocessed = FreeImage_Rescale(image, width, height, FILTER_BILINEAR);
        working_width = FreeImage_GetWidth(preprocessed);
        working_height = FreeImage_GetHeight(preprocessed);
	}
	else {
		preprocessed = FreeImage_Clone(image);		
        working_width = real_width;
        working_height = real_height;
	}
	if(preprocessed==NULL) {
		return false;
	}     
    // mean shift filter to segmentize the image and erase light differences (very slow)
    if(meanshift_r2 && meanshift_d2) {
        if ( real_bpp != 24 ) {
            FIBITMAP* rgb = FreeImage_ConvertTo24Bits(preprocessed);
            FreeImage_Unload(preprocessed);
            preprocessed = rgb;
        }
        preprocessed = mean_shift_filter(preprocessed, meanshift_r2, meanshift_d2, meanshift_n, meanshift_iterations);
#ifdef DEBUG
        fprintf(stderr, "Mean-shifted image\n");
        GenericWriter(preprocessed, "meanshift.png", 0);
#endif    
    }
    // use a soft blur to erase small differences (fast)
    else if(sigma) {
        FIBITMAP* grey = FreeImage_ConvertToGreyscale(preprocessed);
        FreeImage_Unload(preprocessed);
        preprocessed = gaussian_filter(grey, sigma);
        greyscale = true;
#ifdef DEBUG
        fprintf(stderr, "Gaussed image\n");
        GenericWriter(preprocessed, "gaussed.png", 0);
#endif    
    }
    
    // edge detection
    FIBITMAP* for_vector = NULL;
    bool edge_detected = false;
	if(algorithm >=  AA_ALG_VECTOR_DST && algorithm <=  AA_ALG_VECTOR_OR_PIXEL) {
        edge_detected = true;
		//edges
#ifdef DEBUG
		fprintf(stderr, "Performing canny edge detection\n");
#endif 
        if( !greyscale ) {
            // convert to greyscale
            for_vector = FreeImage_ConvertToGreyscale(preprocessed);
            greyscale = true;
        }
        else {
            for_vector = FreeImage_Clone(preprocessed);	
        }

		for_vector = canny_edge_detection(for_vector, canny_min, canny_max);
		if ( for_vector==NULL ) {
			return false;
		}
#ifdef DEBUG
		fprintf(stderr, "Blurring out small differences\n");
		GenericWriter(for_vector, "edges.png", 0);
#endif 
        float postsigma;
        if ( working_width <= 128 )
            postsigma = 0.1;
        else if ( working_width <= 512 )
            postsigma = 0.3;
        else
            postsigma = 0.6;
        if ( postsigma )
            for_vector = gaussian_filter(for_vector, postsigma);
		for_vector = threshold(for_vector, 1);
#ifdef DEBUG
		GenericWriter(for_vector, "edges_gaussed.png", 0);
#endif 

		//thighten
#ifdef DEBUG
		fprintf(stderr, "Thinning image\n");
#endif 
		for_vector = thinning(for_vector);
#ifdef DEBUG
		GenericWriter(for_vector, "edges_thinned.png", 0);
#endif 
		FreeImage_Invert(for_vector);
	}

    FIBITMAP* for_pixel = NULL;
    //resize for_pixel: 1 pixel = 1 character
    int end_height, end_width, end_pitch, end_dif_pitch;
    end_height = lines;
    float percent = (end_height/float(real_height));
    end_width = int(real_width*percent)*2;
    for_pixel = FreeImage_Rescale(preprocessed, end_width, end_height, FILTER_BOX);
    FreeImage_Unload(preprocessed);
#ifdef DEBUG
    fprintf(stderr, "Resizing image from %dx%d to %dx%d to fit in %d lines\n", real_width, real_height, end_width, end_height, lines);
#endif        
    FreeImage_FlipVertical(for_pixel);
    //resize for_vector: 1 bloc = 1 character
    if ( for_vector != NULL ) {
        int height = PASY*end_height;
        float percent = (height/float(real_height));
        int width = end_width*PASX;
        FIBITMAP* resized = FreeImage_Rescale(for_vector, width, height, FILTER_BOX);
        FreeImage_Unload(for_vector);
        for_vector = resized;
    }
    

    //threshold
    if (for_vector != NULL && edge_detected ) { 
        FreeImage_Invert(for_vector);
        for_vector = threshold(for_vector, 1);
        FreeImage_FlipVertical(for_vector);
    }

    
    //color map
    if(palette_id < 0 || palette_id >=  AA_PAL_MAX) {
        fprintf(stderr, "Invalid palette id\n");
        return false;
    }
    res->colors = new BYTE[end_width*end_height];
    if ( palette_id != AA_PAL_NONE ) {
        AaPalette* palette = &hardcoded_palettes[palette_id];
        RGBQUAD freeimage_palette[PALETTE_MAX_SIZE];
        //converts to 24bits
        FIBITMAP* color_24 = FreeImage_ConvertTo24Bits(for_pixel);
        //LSB / MSB aware
        int effective_size = palette->size;
        if(palette->raw == NULL) {
            effective_size = 0;
        }
        for(int i = 0;i<effective_size;i++) {
            freeimage_palette[i].rgbRed = palette->raw[i].rgbRed;
            freeimage_palette[i].rgbGreen = palette->raw[i].rgbGreen;
            freeimage_palette[i].rgbBlue = palette->raw[i].rgbBlue;
        }
        FIBITMAP* paletized = FreeImage_ColorQuantizeEx(color_24, FIQ_NNQUANT, palette->size, effective_size, freeimage_palette);
        FreeImage_Unload(color_24);
        //copy effective palette
        res->palette = new AaPalette;
        res->palette->size = palette->size;
        res->palette->raw = new AaColor[res->palette->size];
        RGBQUAD* effective_palette = FreeImage_GetPalette(paletized);
        for(int i = 0;i<res->palette->size;i++) {
            res->palette->raw[i].rgbRed = effective_palette[i].rgbRed;
            res->palette->raw[i].rgbGreen = effective_palette[i].rgbGreen;
            res->palette->raw[i].rgbBlue = effective_palette[i].rgbBlue;
        }
#ifdef DEBUG
        GenericWriter(paletized, "paletized.png", 0);
#endif        
        BYTE* coul = res->colors;
        BYTE* pixelc = FreeImage_GetBits(for_pixel);
        for(int y = 0; y < end_height; y++, pixelc+= end_dif_pitch) {
            for(int x = 0; x < end_width; x++) {
                *coul++= *pixelc++;
            }
        }
        FreeImage_Unload(paletized);
    }
    else {
        res->palette = new AaPalette;
        res->palette->size = 0;
        res->palette->raw = NULL;
    }

    res->cols = end_width;
    res->lines = end_height;
    res->characters = new char[res->cols*res->lines+1];
	//bloc->char
    if(algorithm >=  AA_ALG_VECTOR_DST && algorithm <=  AA_ALG_VECTOR_OR_PIXEL) {
        int width = FreeImage_GetWidth(for_vector);
        int height = FreeImage_GetHeight(for_vector);
        BYTE* coul = res->colors;
        char* cur = res->characters;
        for(int y = 0; y <= height-PASY; y+= PASY) {
            for(int x = 0; x <= width-PASX; x+= PASX) {
                char bestcar;
                if(algorithm == AA_ALG_VECTOR_DST || algorithm == AA_ALG_VECTOR_DST_FILL || algorithm == AA_ALG_VECTOR_OR_PIXEL) {
                    bestcar = get_best_char_distance(for_vector, font, x, y, translation, penalty);
                }
                else if(algorithm == AA_ALG_VECTOR_11 || algorithm == AA_ALG_VECTOR_11_FILL) {
                    bestcar = get_best_char(for_vector, font, x, y, translation, penalty);
                }
                BYTE color = *coul++;
                if(palette_id != AA_PAL_NONE && color && bestcar==' ' && (algorithm == AA_ALG_VECTOR_DST_FILL || algorithm == AA_ALG_VECTOR_11_FILL)) {
                    bestcar = FILL_BACKGROUND;
                }
                *cur++ = bestcar;
            }
        }
        *cur = '\0';
	}
  
    //pixel->char 
    if(algorithm == AA_ALG_PIXEL_11 || algorithm == AA_ALG_VECTOR_OR_PIXEL) {
        FIBITMAP* grey = FreeImage_ConvertToGreyscale(for_pixel);
        if (grey == NULL)
            return false;
        FreeImage_Unload(for_pixel);
        for_pixel = grey;
        FreeImage_Invert(for_pixel);
        BYTE* pixel = FreeImage_GetBits(for_pixel);
        char* cur = res->characters;
        static const char* map = " .:+og@ ";
        int end_dif_pitch = FreeImage_GetPitch(for_pixel) - FreeImage_GetWidth(for_pixel);
        for(int y = 0; y < end_height; y++, pixel+= end_dif_pitch) {
            for(int x = 0; x < end_width; x++) {
                BYTE intensity = *pixel++;
                if(algorithm == AA_ALG_PIXEL_11 || *cur==' ')
                    *cur++ = map[intensity>>5];
                else
                    cur++;
            }
        }
        *cur = '\0';
    }
    if (for_pixel != NULL)
        FreeImage_Unload(for_pixel);
    if (for_vector != NULL)
        FreeImage_Unload(for_vector);
#ifdef DEBUG
	FILE* f = fopen("final.txt", "w");
	aa_output_ascii(res, f);
	fclose(f);
#endif
	return true;
}

///////////////////////////////////////////////////////////////////////////////////////

bool aa_output_ascii(AaImage* image, FILE* out) {
    char* cur = image->characters;
    for(int y = 0; y<image->lines;y++) {
        for(int x = 0; x < image->cols; x++) {
            fprintf(out, "%c", *cur++);
        }
        fprintf(out, "\n");
    }
    return true;
}

bool aa_output_ansi16(AaImage* image, FILE* out) {  
    if(image->palette->size > 16) {
        fprintf(stderr, "aa_output_ansi16 cannot deal with more than 16 colors, there are %d", image->palette->size);
        return false;
    }
    char* cur = image->characters;
    BYTE* coul = image->colors;
    for(int y = 0; y<image->lines;y++) {
        for(int x = 0; x < image->cols; x++) {
            char bestcar = *cur++;
            int fg_color = *coul++;
            int bg_color = 0;
            if(bestcar!=' ' && fg_color==0) 
                fg_color = 7;
            fprintf(out, "\x1b[%s%d;%s%dm%c\x1b[0m", bg_color>7?"5;":"", 40+(bg_color&7), fg_color>7?"1;":"", 30+(fg_color&7), bestcar);
        }
        fprintf(out, "\n");
    }
    fprintf(out, "\x1b[0m");
    return true;
}

bool aa_output_html_palette(AaImage* image, FILE* out) {
    //palette
    fprintf(out, "<style media = \"screen\" type = \"text/css\">\n");
    AaPalette* palette = image->palette;
    for(int i = 0;i<palette->size;i++) {
        fprintf(out, ".c%d {color:rgb(%d, %d, %d)}\n", i, palette->raw[i].rgbRed, palette->raw[i].rgbGreen, palette->raw[i].rgbBlue);
    }
    fprintf(out, "</style>\n");
}

bool aa_output_html(AaImage* image, FILE* out, bool palette) {  
    fprintf(out, "<div>\n");
    if(palette) {
        aa_output_html_palette(image, out);
    }
    fprintf(out, "<pre style = 'background-color:black;font-family:\"Lucida Console\";font-size:10pt'>\n");
    char* cur = image->characters;
    BYTE* coul = image->colors;
    int lastcolor = -1;
    //image
    for(int y = 0; y < image->lines;y++) {
        for(int x = 0; x < image->cols; x++) {
			char bestcar = *cur++;
			char color = *coul++;
            if(lastcolor!=color) {
                if(lastcolor!=-1)
                    fprintf(out, "</span>");
                if(palette) {
                    fprintf(out, "<span class='c%d'>", color);
                }
                else {
                    fprintf(out, "<span style = 'color:rgb(%d,%d,%d)'>", image->palette->raw[color].rgbRed, image->palette->raw[color].rgbGreen, image->palette->raw[color].rgbBlue);
                }
                lastcolor = *coul;
            }
            switch(bestcar) {
                case '"':
                    fprintf(out, "&quot;");
                    break;
                case '<':
                    fprintf(out, "&lt;");
                    break;
                case '>':
                    fprintf(out, "&gt;");
                    break;  
                case '&':
                    fprintf(out, "&amp;");
                    break;                            
                default:
                    fprintf(out, "%c", bestcar);
                    break;
            }
        }
        if(y+1 < image->lines)
            fprintf(out, "\n");
    }
    fprintf(out, "</span>\n</pre></div>\n");
    return true;
}

bool aa_output_html_mono(AaImage* image, FILE* out) {  
    fprintf(out, "<div>\n");
    fprintf(out, "<pre style = 'background-color:black;color:white;font-family:\"Lucida Console\";font-size:10pt'>\n");
    char* cur = image->characters;
    //image
    for(int y = 0; y < image->lines;y++) {
        for(int x = 0; x < image->cols; x++) {
			char bestcar = *cur++;
            switch(bestcar) {
                case '"':
                    fprintf(out, "&quot;");
                    break;
                case '<':
                    fprintf(out, "&lt;");
                    break;
                case '>':
                    fprintf(out, "&gt;");
                    break;  
                case '&':
                    fprintf(out, "&amp;");
                    break;                            
                default:
                    fprintf(out, "%c", bestcar);
                    break;
            }
        }
        if(y+1 < image->lines)
            fprintf(out, "\n");
    }
    fprintf(out, "</pre></div>\n");
    return true;
}
///////////////////////////////////////////////////////////////////////////////////////

FIBITMAP* aa_load_bitmap_from_memory(BYTE* data, unsigned int size) {
	FIBITMAP* res = NULL;
	FIMEMORY* mem = FreeImage_OpenMemory(data, size);
	FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	fif = FreeImage_GetFileTypeFromMemory(mem, 0);
	if(fif == FIF_UNKNOWN) {
		fprintf(stderr, "Could not open image memory");
		return NULL;
	}
	res = FreeImage_LoadFromMemory(fif, mem, 0);
	FreeImage_CloseMemory(mem);
	return res;
}


FIBITMAP* aa_load_bitmap_from_file(const char* path) {
    FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;
	fif = FreeImage_GetFileType(path, 0);
	if(fif == FIF_UNKNOWN) {
		fif = FreeImage_GetFIFFromFilename(path);
	}
	if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
		FIBITMAP *dib = FreeImage_Load(fif, path, 0);
        if(dib)
            return dib;
	}
    fprintf(stderr, "Could not open image file");
	return NULL;
}

void aa_dispose_bitmap(FIBITMAP* bitmap) {
    FreeImage_Unload(bitmap);
}
