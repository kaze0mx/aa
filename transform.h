#include <string.h>
#include <math.h>
#include <FreeImage.h>

#define MAX_BRIGHTNESS 255

FIBITMAP* canny_edge_detection(FIBITMAP* source, float sigma, int tmin, int tmax);
FIBITMAP* canny_edge_detection2(FIBITMAP* source, float sigma, int tmin, int tmax);

FIBITMAP* gaussian_filter(FIBITMAP* source, const float sigma);
FIBITMAP* thinning(FIBITMAP* source);
FIBITMAP* convolute(FIBITMAP* source, const float *kernel, const int kn, const bool normalize, int ignore_min=-1, int ignore_max=-1);
FIBITMAP* threshold(FIBITMAP* source, BYTE threshold);
FIBITMAP* normalize(FIBITMAP* source);
char* base64(FIBITMAP* image, FREE_IMAGE_FORMAT format=FIF_PNG);


bool is_clipart(FIBITMAP* source);
