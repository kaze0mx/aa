#include <stdlib.h>
#include <stdio.h>
#include "aa.h"
#include "transform.h"


bool write_image(FIBITMAP* dib, const char* lpszPathName) {
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
                bSuccess = FreeImage_Save(fif, dib, lpszPathName, 0);
                // unless an abnormal bug, we are done !
            }
        }
    }
    return (bSuccess == TRUE) ? true : false;
}

int main(int argc, char** argv) {
	if(argc != 2)  {
		fprintf(stderr, "Usage: %s <image_path>\n", argv[0]);
		exit(1);
	} 

	
    FIBITMAP* image = aa_load_bitmap_from_file(argv[1]);
    FREE_IMAGE_TYPE type = FreeImage_GetImageType(image);
    if(image==NULL) {
		fprintf(stderr, "Could not open image\n");
		exit(1);
    }    

    FIBITMAP* res = pixel_convert(image, 64, 480, AA_PAL_PARANO, DEFAULT_MEANSHIFT_R2, DEFAULT_MEANSHIFT_D2, 9, 15);
    if (res==NULL) {
        fprintf(stderr, "Could not convert image\n");
		exit(1);
    }

    if (!write_image(res, "res.png")) {
        fprintf(stderr, "Could not write image\n");
		exit(1);
    }
    return 0;
}
