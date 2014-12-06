#include <stdio.h>
#include <assert.h>
#include <float.h>
#include "transform.h"

#define DEBUG

typedef BYTE pixel_t;

//RGB to LUV conversion
const double RGB2LUV[3][3] = {  {  0.4125,  0.3576,  0.1804 },
                            {  0.2125,  0.7154,  0.0721 },
                            {  0.0193,  0.1192,  0.9502 }   };

//LUV to RGB conversion
const double LUV2RGB[3][3] = {  {  3.2405, -1.5371, -0.4985 },
                            { -0.9693,  1.8760,  0.0416 },
                            {  0.0556, -0.2040,  1.0573 }   };

const double luv_Xn         = 0.95050;
const double luv_Yn         = 1.00000;
const double luv_Zn         = 1.08870;
const double luv_Un_prime   = 0.19784977571475;
const double luv_Vn_prime   = 0.46834507665248;
const double luv_Lt         = 0.008856;


bool DebugWriter(FIBITMAP* dib, const char* lpszPathName) {
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


inline int my_round(const double in_x) {
    if (in_x < 0)
        return (int)(in_x - 0.5);
    else
        return (int)(in_x + 0.5);
}

/*
 * dest should be 24bpp
 */
void luv_to_rgb(FIBITMAP* dest, const float* in_luv) {
    int width = FreeImage_GetWidth(dest);
    int height = FreeImage_GetHeight(dest);
    int pitch = FreeImage_GetPitch(dest);
    BYTE* out = FreeImage_GetBits(dest);
    WORD bpp = FreeImage_GetBPP(dest);
    assert(bpp = 24);
    int     r, g, b;
    double  x, y, z, u_prime, v_prime;
    unsigned int i, j, rgbloc, luvind, rgbind;

    for(j = 0, rgbloc = 0, luvind = 0; j < height; j++, rgbloc+=pitch) {
        for(i = 0; i < width; i++, luvind+=3) {
            rgbind = rgbloc+3*i;
            if(in_luv[luvind] < 0.1) {
                r = g = b = 0;
            }
            else {
                //convert luv to xyz...
                if(in_luv[luvind] < 8.0)
                    y = luv_Yn * in_luv[0] / 903.3;
                else {
                    y = (in_luv[luvind] + 16.0) / 116.0;
                    y *= luv_Yn * y * y;
                }

                u_prime = in_luv[luvind+1] / (13 * in_luv[luvind]) + luv_Un_prime;
                v_prime = in_luv[luvind+2] / (13 * in_luv[luvind]) + luv_Vn_prime;
                x = 9 * u_prime * y / (4 * v_prime);
                z = (12 - 3 * u_prime - 20 * v_prime) * y / (4 * v_prime);

                //convert xyz to rgb...
                r = my_round((LUV2RGB[0][0]*x + LUV2RGB[0][1]*y + LUV2RGB[0][2]*z)*255.0);
                g = my_round((LUV2RGB[1][0]*x + LUV2RGB[1][1]*y + LUV2RGB[1][2]*z)*255.0);
                b = my_round((LUV2RGB[2][0]*x + LUV2RGB[2][1]*y + LUV2RGB[2][2]*z)*255.0);

                //check bounds...
            }
            //assign rgb values to rgb vector rgbVal
            out[rgbind+FI_RGBA_RED] = r & 0xFF; 
            out[rgbind+FI_RGBA_GREEN] = g & 0xFF;   
            out[rgbind+FI_RGBA_BLUE] = b & 0xFF;
        }
    }
}

/*
 * source should be 24bpp
 */
void rgb_to_luv(FIBITMAP* source, float* out_luv) { 
    int width = FreeImage_GetWidth(source);
    int height = FreeImage_GetHeight(source);
    int pitch = FreeImage_GetPitch(source);
    BYTE* in = FreeImage_GetBits(source);
    WORD bpp = FreeImage_GetBPP(source);
    assert(bpp = 24);
    double	x, y, z, L0, u_prime, v_prime, constant;
    unsigned int i, j, rgbloc, luvind, rgbind;

    for(j = 0, rgbloc = 0, luvind = 0; j < height; j++, rgbloc+=pitch) {
        for(i = 0; i < width; i++, luvind+=3) {
            rgbind = rgbloc+3*i;
            x = RGB2LUV[0][0]*in[rgbind+FI_RGBA_RED] + RGB2LUV[0][1]*in[rgbind+FI_RGBA_GREEN] + RGB2LUV[0][2]*in[rgbind+FI_RGBA_BLUE];
            y = RGB2LUV[1][0]*in[rgbind+FI_RGBA_RED] + RGB2LUV[1][1]*in[rgbind+FI_RGBA_GREEN] + RGB2LUV[1][2]*in[rgbind+FI_RGBA_BLUE];
            z = RGB2LUV[2][0]*in[rgbind+FI_RGBA_RED] + RGB2LUV[2][1]*in[rgbind+FI_RGBA_GREEN] + RGB2LUV[2][2]*in[rgbind+FI_RGBA_BLUE];
            L0 = y / (255.0 * luv_Yn);
            if (L0 > luv_Lt)
                out_luv[luvind] = (float)(116.0 * (pow(L0, 1.0/3.0)) - 16.0);
            else
                out_luv[luvind]	= (float)(903.3 * L0);
            constant	= x + 15 * y + 3 * z;
            if(constant != 0) {
                u_prime	= (4 * x) / constant;       
                v_prime = (9 * y) / constant;
            }
            else {
                u_prime	= 4.0;		
                v_prime	= 9.0/15.0;
            }
            /**compute u* and v* */
            out_luv[luvind+1] = (float) (13 * out_luv[luvind] * (u_prime - luv_Un_prime));
            out_luv[luvind+2] = (float) (13 * out_luv[luvind] * (v_prime - luv_Vn_prime));
        }
    }
}

FIBITMAP* mean_shift_filter(FIBITMAP* source, float radiusr2, float radiusd2) {
    return NULL;
}

/*
 * http://rosettacode.org/wiki/Canny_edge_detector
 */
// if normalize is true, map pixels to range 0..MAX_BRIGHTNESS
void convolution(const BYTE* in, BYTE* out, const float *kernel,
                 const int nx, const int ny, const int pitch, int kn,
                 const bool normalize, int ignore_min, int ignore_max)
{
    assert(kn % 2 == 1);
    assert(nx > kn && ny > kn);
    const int khalf = kn / 2;
    float min = FLT_MAX, max = -FLT_MAX;

    if (normalize) {
        for (int m = khalf; m < nx - khalf; m++) {
            for (int n = khalf; n < ny - khalf; n++) {
                BYTE lu = in[n*pitch+m];
                if(lu >= ignore_min && lu <=  ignore_max)
                    continue;
                float pixel = 0.0;
                size_t c = 0;
                for (int j = -khalf; j <=  khalf; j++)
                    for (int i = -khalf; i <=  khalf; i++) {
                        pixel +=  in[(n - j) * pitch + m - i] * kernel[c];
                        c++;
                    }
                if (pixel < min)
                    min = pixel;
                if (pixel > max)
                    max = pixel;
            }
        }
    }
    
    for (int m = 0; m < nx; m++) {
        for (int n = 0; n < ny; n++) {
            int ind = n*pitch+m;
            pixel_t lu = in[ind];
            if ( m < khalf || m > nx-khalf || n < khalf || n > ny-khalf ) {
                out[ind] = lu;
            }
            else if(!(lu >=  ignore_min && lu <=  ignore_max)) {
                float pixel = 0.0;
                size_t c = 0;
                for (int j = -khalf; j <=  khalf; j++)
                    for (int i = -khalf; i <=  khalf; i++) {
                        pixel +=  in[(n - j) * pitch + m - i] * kernel[c];
                        c++;
                    }
     
                if (normalize)
                    pixel = MAX_BRIGHTNESS * (pixel - min) / (max - min);
                if(pixel>MAX_BRIGHTNESS)
                    pixel = MAX_BRIGHTNESS;
                if(pixel<0)
                    pixel = 0;
                out[ind] = (pixel_t)pixel;
            }
            else {
                out[ind] = lu;
            }
        }
    }
}
 

FIBITMAP* convolute(FIBITMAP* source, const float *kernel, const int kn, const bool normalize, int ignore_min, int ignore_max) {
    int nx = FreeImage_GetWidth(source);
    int ny = FreeImage_GetHeight(source);
    int pitch = FreeImage_GetPitch(source);
    BYTE* in = FreeImage_GetBits(source);
    BYTE* out = new BYTE[pitch*ny];
    convolution(in, out, kernel, nx, ny, pitch, kn, normalize, ignore_min, ignore_max);
    memcpy(in, out, pitch*ny);
    delete[] out;
    return source;
}




FIBITMAP* thinningsub(FIBITMAP* source, int iter) {
    int nx = FreeImage_GetWidth(source);
    int ny = FreeImage_GetHeight(source);
    int pitch = FreeImage_GetPitch(source);
    BYTE* in = FreeImage_GetBits(source);
    BYTE* mask = new BYTE[pitch*ny];

    const int threshold = 128;

    for (int y = 1; y < ny-1; y++) {
        for (int x = 1; x < nx-1; x++) {
            int ind = x+y*pitch;
            int p1 = in[ind]<threshold?0:1;
            int p2 = in[ind-pitch]<threshold?0:1;
            int p3 = in[ind-pitch+1]<threshold?0:1;
            int p4 = in[ind+1]<threshold?0:1;
            int p5 = in[ind+pitch+1]<threshold?0:1;
            int p6 = in[ind+pitch]<threshold?0:1;
            int p7 = in[ind+pitch-1]<threshold?0:1;
            int p8 = in[ind-1]<threshold?0:1;
            int p9 = in[ind-pitch-1]<threshold?0:1;
            int A  = (p2 == 0 && p3 == 1) + (p3 == 0 && p4 == 1) + 
                     (p4 == 0 && p5 == 1) + (p5 == 0 && p6 == 1) + 
                     (p6 == 0 && p7 == 1) + (p7 == 0 && p8 == 1) +
                     (p8 == 0 && p9 == 1) + (p9 == 0 && p2 == 1);
            int B  = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
            int m1 = iter == 0 ? (p2 * p4 * p6) : (p2 * p4 * p8);
            int m2 = iter == 0 ? (p4 * p6 * p8) : (p2 * p6 * p8);

            if (A == 1 && (B >=  2 && B <=  6) && m1 == 0 && m2 == 0)
                mask[ind] = 255;
            else
                mask[ind] = 0;
        }
    }
    for(int i = 0;i<pitch*ny;i++) {
        in[i] = in[i] & ~(mask[i]);
    }
    delete[] mask;
    return source;
}



FIBITMAP* thinning(FIBITMAP* source) {
    int nx = FreeImage_GetWidth(source);
    int ny = FreeImage_GetHeight(source);
    int pitch = FreeImage_GetPitch(source);
    bool cont = true;
    BYTE* old_buffer = new BYTE[pitch*ny];
    while(cont) {
        memcpy(old_buffer, FreeImage_GetBits(source), pitch*ny);
        source = thinningsub(source, 0);
        source = thinningsub(source, 1);
        cont = memcmp(FreeImage_GetBits(source), old_buffer, pitch*ny)!=0;
    }
    delete[] old_buffer;    
    return source;
}


FIBITMAP* threshold(FIBITMAP* source, BYTE threshold) {
    int ny = FreeImage_GetHeight(source);
    int pitch = FreeImage_GetPitch(source);
    BYTE* v = FreeImage_GetBits(source);
    for(int i = 0;i<pitch*ny;i++, v++) {
        if(*v<threshold)
            *v = 0;
        else
            *v = MAX_BRIGHTNESS;
    }
    return source;
}

FIBITMAP* normalize(FIBITMAP* source) {
    int nx = FreeImage_GetWidth(source);
    int ny = FreeImage_GetHeight(source);
    int pitch = FreeImage_GetPitch(source);
    BYTE* v = FreeImage_GetBits(source);
    float maximum = 0;
    for(int y = 0; y < ny; y++, v+= pitch-nx) {
        for(int x = 0; x < nx; x++, v++) {
            if(*v>maximum) {
                maximum = *v;
            }
        }
    }
    v = FreeImage_GetBits(source);
    for(int y = 0; y < ny; y++, v+= pitch-nx) {
        for(int x = 0; x < nx; x++, v++) {
           *v = (*v*MAX_BRIGHTNESS)/maximum;
        }
    }
    return source;
}

bool is_clipart(FIBITMAP* source) {
    int nx = FreeImage_GetWidth(source);
    int ny = FreeImage_GetHeight(source);
    int surface = nx*ny;
    DWORD histogram[255];
    FreeImage_GetHistogram(source, histogram, FICC_BLACK);
    int threshold = (1.0*surface)/255;
    int howmanygreater = 0;
    for(int i = 0;i<255;i++) {
        if(histogram[i] > threshold)
            howmanygreater++;
    }
    return howmanygreater<4;
}

/*
 * gaussianFilter:
 * http://www.songho.ca/dsp/cannyedge/cannyedge.html
 * determine surface of kernel (odd #)
 * 0.0 <=  sigma < 0.5 : 3
 * 0.5 <=  sigma < 1.0 : 5
 * 1.0 <=  sigma < 1.5 : 7
 * 1.5 <=  sigma < 2.0 : 9
 * 2.0 <=  sigma < 2.5 : 11
 * 2.5 <=  sigma < 3.0 : 13 ...
 * kernelSize = 2 * int(2*sigma) + 3;
 */
FIBITMAP* gaussian_filter(FIBITMAP* source, const float sigma)
{
    const int n = 2 * (int)(2 * sigma) + 3;
    const float mean = (float)floor(n / 2.0);
    float kernel[n * n]; // variable length array
 
    size_t c = 0;
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            kernel[c] = exp(-0.5 * (pow((i - mean) / sigma, 2.0) +
                                    pow((j - mean) / sigma, 2.0)))
                        / (2 * M_PI * sigma * sigma);
            c++;
        }
    }
    return convolute(source, kernel, n, true);
}


void propagate_max(int X, int Y, int pitch, BYTE* buffer)
{
    int ind = X+Y*pitch;
    int ind_ym1 = ind-pitch;
    int ind_yp1 = ind+pitch;

    BYTE val = buffer[ind];

    //1
    if (buffer[ind+1] > 0 && buffer[ind+1] < val) {
        buffer[ind+1] = val;
        propagate_max(X + 1, Y, pitch, buffer);
    }
    //2
    if (buffer[ind_ym1+1] > 0 && buffer[ind_ym1+1]  < val) {
        buffer[ind_ym1+1] = val;
        propagate_max(X + 1, Y - 1, pitch, buffer);
    }
    //3
    if (buffer[ind_ym1] > 0 && buffer[ind_ym1]  < val) {
        buffer[ind_ym1] = val;
        propagate_max(X, Y - 1, pitch, buffer);
    }
    //4
    if (buffer[ind_ym1-1] > 0 && buffer[ind_ym1-1]  < val) {
        buffer[ind_ym1-1] = val;
        propagate_max(X - 1, Y - 1, pitch, buffer);
    }
    //5
    if (buffer[ind-1] > 0 && buffer[ind-1]  < val) {
        buffer[ind-1] = val;
        propagate_max(X - 1, Y, pitch, buffer);
    }
    //6
    if (buffer[ind_yp1-1] > 0 && buffer[ind_yp1-1]  < val) {
        buffer[ind_yp1-1] = val;
        propagate_max(X - 1, Y + 1, pitch, buffer);
    }
    //7
    if (buffer[ind_yp1] > 0 && buffer[ind_yp1]  < val) {
        buffer[ind_yp1] = val;
        propagate_max(X , Y + 1, pitch, buffer);
    }
    //8
    if (buffer[ind_yp1+1] > 0 && buffer[ind_yp1+1]  < val) {
        buffer[ind_yp1+1] = val;
        propagate_max(X + 1, Y + 1, pitch, buffer);
    }
    return;
}


/*
 * only works with greyscale images
 */
FIBITMAP* canny_edge_detection(FIBITMAP* source, int MinHysteresisThresh, int MaxHysteresisThresh) 
{

    int width = FreeImage_GetWidth(source);
    int height = FreeImage_GetHeight(source);
    int pitch = FreeImage_GetPitch(source);
    int surface = pitch*height;

    BYTE* buffer = FreeImage_GetBits(source);
    
    int limit = 2;
    int i, j;

#ifdef DEBUG
    fprintf(stderr, "Smoothed image\n");
    DebugWriter(source, "gaussed.png");
#endif    

    //Sobel Masks
    float SobelX1[] = {1,0,-1, 1,0,-1,  1,0,-1};
    float SobelX2[] = {-1,0,1, -1,0,1,  -1,0,1};
    float SobelY1[] = {1,1,1,  0,0,0,  -1,-1,-1};
    float SobelY2[] = {-1,-1,-1,  0,0,0,  1,1,1};
    BYTE* DerivativeX1 = new BYTE[surface];
    BYTE* DerivativeX2 = new BYTE[surface];
    BYTE* DerivativeY1 = new BYTE[surface];
    BYTE* DerivativeY2 = new BYTE[surface];
    convolution(buffer, DerivativeX1, SobelX1, width, height, pitch, 3, false, -1, -1);
    convolution(buffer, DerivativeY1, SobelY1, width, height, pitch, 3, false, -1, -1);
    convolution(buffer, DerivativeX2, SobelX2, width, height, pitch, 3, false, -1, -1);
    convolution(buffer, DerivativeY2, SobelY2, width, height, pitch, 3, false, -1, -1);
#ifdef DEBUG
    fprintf(stderr, "Computed derivatives using sobel masks\n");
#endif        

    float* NonMax = new float[surface];
    //Compute the gradient magnitude based on derivatives in x and y:
    for (i = 0; i <  width; i++) {
        for (j = 0; j < height; j++) {
            int ind = j*pitch+i;
            if ( i > limit && i < width - limit && j > limit && j < height - limit) {
                float a = (float)sqrt((((float)DerivativeX1[ind]) * DerivativeX1[ind]) + (((float)DerivativeY1[ind]) * DerivativeY1[ind]));
                float b = (float)sqrt((((float)DerivativeX2[ind]) * DerivativeX2[ind]) + (((float)DerivativeY2[ind]) * DerivativeY2[ind]));
                NonMax[ind] = a>b?a:b;
            }
            else {
                NonMax[ind] = 0;
            }
        }
    }
 
    
#ifdef DEBUG
    fprintf(stderr, "Computed gradients\n");
#endif     
    int r, c;
    float Tangent;
    
    for (i = limit; i <=  (width - limit) - 1; i++) {
        for (j = limit; j <=  (height - limit) - 1; j++) {

            if (DerivativeX1[i + j*pitch] == 0)
                Tangent = 90.0;
            else
                Tangent = (float)(atan(((float)DerivativeY1[i + j*pitch]) / DerivativeX1[i + j*pitch]) * 180.0 / M_PI); //rad to degree
            //Horizontal Edge
            if (((-22.5 < Tangent) && (Tangent <= 22.5)) || (157.5 < Tangent) || (Tangent <=  -157.5)) {
                if ((NonMax[i + j*pitch] < NonMax[i + (j + 1)*pitch]) || (NonMax[i + j*pitch] < NonMax[i + (j - 1)*pitch]))
                    NonMax[i + j*pitch] = 0;
            }
            //Vertical Edge
            else if (((-112.5 < Tangent) && (Tangent <=  -67.5)) || ((67.5 < Tangent) && (Tangent <=  112.5))) {
                if ((NonMax[i + j*pitch] < NonMax[i + 1 + j*pitch]) || (NonMax[i + j*pitch] < NonMax[i - 1 + j*pitch]))
                    NonMax[i + j*pitch] = 0;
            }
            //+45 Degree Edge
            else if (((-67.5 < Tangent) && (Tangent <=  -22.5)) || ((112.5 < Tangent) && (Tangent <=  157.5))) {
                if ((NonMax[i + j*pitch] < NonMax[i + 1 + (j - 1)*pitch]) || (NonMax[i + j*pitch] < NonMax[i - 1 + (j + 1)*pitch]))
                    NonMax[i + j*pitch] = 0;
            }
            //-45 Degree Edge
            else if (((-157.5 < Tangent) && (Tangent <=  -112.5)) || ((67.5 < Tangent) && (Tangent <=  22.5))) {
                if ((NonMax[i + j*pitch] < NonMax[i + 1, j + 1]) || (NonMax[i + j*pitch] < NonMax[i - 1 + (j - 1)*pitch]))
                    NonMax[i + j*pitch] = 0;
            }
        }
    }
    
#ifdef DEBUG
    fprintf(stderr, "Suppressed non-maximals\n");
#endif  


    //Histery
#ifdef DEBUG
    float* GNH = new float[surface];
    float* GNL = new float[surface]; 
#endif

    for(int i = 0;i<surface;i++) {
        if (NonMax[i] >=  MaxHysteresisThresh) {
            buffer[i] = 2;  //high
#ifdef DEBUG
            GNH[i] = 255;
#endif
        }
        else if ((NonMax[i] < MaxHysteresisThresh) && (NonMax[i] >=  MinHysteresisThresh)) {
            buffer[i] = 1;  //low
#ifdef DEBUG
            GNL[i] = 255;
#endif
        }
        else {
            buffer[i] = 0;
        }
    }
#ifdef DEBUG
    fprintf(stderr, "Thresholded with hysteris\n");
#endif      

    //keep only low-edges connected to high edges
    for (i = limit; i <=  (width - 1) - limit; i++) {
        for (j = limit; j <=  (height  - 1) - limit; j++) {
            int ind = i+j*pitch;
            if(buffer[ind] == 2) {
                buffer[ind] = 3;
                propagate_max(i, j, pitch, buffer);
            }
        }
    }
#ifdef DEBUG
    fprintf(stderr, "Connected edges\n");
#endif 

#ifdef DEBUG
    FIBITMAP* clone = FreeImage_Clone(source);
    BYTE* test = FreeImage_GetBits(clone);
    for (i = 0; i < surface; i++) {
        test[i] = (BYTE)GNL[i];
    }
    DebugWriter(clone, "GNL.png");
    for (i = 0; i < surface; i++) {
        test[i] = (BYTE)GNH[i];
    }
    DebugWriter(clone, "GNH.png");
    FreeImage_Unload(clone);
#endif  

    for (i = 0; i < surface; i++) {
        if(buffer[i]==3)
            buffer[i] = MAX_BRIGHTNESS;
        else if(buffer[i]==1)
            buffer[i] = 0;//MAX_BRIGHTNESS/8;
    }
    delete[] NonMax;
#ifdef DEBUG
    delete[] GNL;
    delete[] GNH;
#endif
    delete[] DerivativeX1;
    delete[] DerivativeY1;
    delete[] DerivativeX2;
    delete[] DerivativeY2;
    return source;
}




///////////////////////////////////////////////////////////////////////////////////////



static unsigned char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                '4', '5', '6', '7', '8', '9', '+', '/'};
static int mod_table[] = {0, 2, 1};


char *base64_encode(const unsigned char *data, size_t input_length) {

    int output_length = 4 * ((input_length + 2) / 3);

    char *encoded_data = new char[output_length];
    if (encoded_data == NULL) return NULL;

    for (int i = 0, j = 0; i < input_length;) {

        uint32_t octet_a = i < input_length ? data[i++] : 0;
        uint32_t octet_b = i < input_length ? data[i++] : 0;
        uint32_t octet_c = i < input_length ? data[i++] : 0;

        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;

        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }

    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';

    return encoded_data;
}

char* base64(FIBITMAP* image, FREE_IMAGE_FORMAT format) {
    FIMEMORY* hmem = FreeImage_OpenMemory();
    FreeImage_SaveToMemory(format, image, hmem, 0);
    long size = FreeImage_TellMemory(hmem);
    FreeImage_SeekMemory(hmem, 0L, SEEK_SET);
    BYTE* data;
    DWORD buf_len;
    FreeImage_AcquireMemory(hmem, &data, &buf_len);
    char* res = base64_encode(data, size);
    FreeImage_CloseMemory(hmem);
    return res;
}

///////////////////////////////////////////////////////////////////////////////////////
