#include <string.h>
#include <math.h>
#include <FreeImage.h>

#define MAX_BRIGHTNESS 255

void rgb_to_luv(FIBITMAP* source, float* out_luv);
void luv_to_rgb(FIBITMAP* dest, const float* in_luv);

FIBITMAP* mean_shift_filter(FIBITMAP* source, float radiusr2, float radiusd2, int n, int max_iterations);
FIBITMAP* canny_edge_detection(FIBITMAP* source, int tmin, int tmax);
FIBITMAP* gaussian_filter(FIBITMAP* source, const float sigma);
FIBITMAP* thinning(FIBITMAP* source);
FIBITMAP* convolute(FIBITMAP* source, const float *kernel, const int kn, const bool normalize, int ignore_min=-1, int ignore_max=-1);
FIBITMAP* threshold(FIBITMAP* source, BYTE threshold);
FIBITMAP* normalize(FIBITMAP* source);
char* base64(FIBITMAP* image, FREE_IMAGE_FORMAT format=FIF_PNG);


bool is_clipart(FIBITMAP* source);
