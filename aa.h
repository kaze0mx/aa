#include <FreeImage.h>
#include <stdio.h>

//#define DEBUG 
#define FONT_FILE "fnt.png"
#define FONT_CARMAP " ²$^¨°+*#&é\"'`|[]{}~(-_à)=123456789azertyuiopqsdfghjklm<wxcvbn,;:!AZERTYUIOPQSDFGHJKLM%µ>WXC@VBN?.\\/§%"
#define FONT_MEGA_SUBSET " $^+*#&\"'`|[]{}~(-_à)=123456789azertyuiopqsdfghjklm<wxcvbn,;:!AZERTYUIOPQSDFGHJKLM%>WXC@VBN?.\\/%"
#define FONT_SUBSET " $^+*#&\"'|~(-)=345atiopqdfghjkl<,:AZTYJL%>X@VN?.\\/_"
#define FONT_TINY_SUBSET " .o@/\\_'yT)(,-bd"

#define PASX 8
#define PASY 16
#define FILL_BACKGROUND '.'
#define DEFAULT_MEANSHIFT_R2 6.5
#define DEFAULT_MEANSHIFT_D2 7.0
#define DEFAULT_MEANSHIFT_N 5
#define DEFAULT_MEANSHIFT_ITERATIONS 4
#define DEFAULT_SIGMA 0.2
#define DEFAULT_CANNY_HYS_MIN 40
#define DEFAULT_CANNY_HYS_MAX 50
#define DEFAULT_PENALTY 0.65
#define DEFAULT_TRANSLATION 2
#define DEFAULT_WORKING_HEIGHT 1024
#define DEFAULT_LINES 25
#define THRESHOLD_FONT 31
#define DEFAULT_PALETTEID AA_PAL_MONOCHROME
#define DEFAULT_ALGORITHM AA_ALG_VECTOR_DST


typedef enum {AA_FT_COURIER, AA_FT_LUCIDA, AA_FT_CONSOLA} AaFontId;
typedef enum {AA_PAL_NONE, AA_PAL_MONOCHROME, AA_PAL_ANSI_16, AA_PAL_FREE_MONO, AA_PAL_FREE_16, AA_PAL_FREE_64, AA_PAL_FREE_256, AA_PAL_MAX} AaPaletteId;
typedef enum {AA_ALG_PIXEL_11, AA_ALG_VECTOR_DST, AA_ALG_VECTOR_DST_FILL, AA_ALG_VECTOR_11, AA_ALG_VECTOR_11_FILL, AA_ALG_MAX} AaAlgorithmId;


typedef struct {
    BYTE rgbRed;
    BYTE rgbGreen;
    BYTE rgbBlue;
} AaColor;

typedef struct {
	FIBITMAP* font;
	BYTE* font_buffer;
	int font_buffer_pitch;
	const char* font_carmap;
	unsigned int font_carmap_len;
	int* font_subset;
    int font_subset_len;
	int* font_subset_matrix_empty;
	int* font_subset_matrix_full;
} AaFont;

typedef struct {
    AaColor* raw;
    int size;
} AaPalette;

typedef struct {
    int cols;
    int lines;
    char* characters;
    BYTE* colors;
    AaPalette* palette;
} AaImage;



bool aa_init_font_default(AaFontId id, const char* subset, AaFont* res);
bool aa_init_font_from_picture(const char* font_picture_path, const char* font_carmap, const char* subset, AaFont* res);

bool aa_convert(FIBITMAP* image, AaAlgorithmId algorithm, AaFont* font, AaImage* res, int lines = DEFAULT_LINES, int working_height = DEFAULT_WORKING_HEIGHT, int translation = DEFAULT_TRANSLATION, float penalty = DEFAULT_PENALTY, AaPaletteId palette_id = DEFAULT_PALETTEID, float sigma = DEFAULT_SIGMA, int canny_min = DEFAULT_CANNY_HYS_MIN, int canny_max = DEFAULT_CANNY_HYS_MAX, float meanshift_r2 = DEFAULT_MEANSHIFT_R2, float meanshift_d2 = DEFAULT_MEANSHIFT_D2, int meanshift_n = DEFAULT_MEANSHIFT_N, int meanshift_iterations = DEFAULT_MEANSHIFT_ITERATIONS);

FIMULTIBITMAP* aa_load_animated_file(const char* path);
FIBITMAP* aa_load_file(const char* path);
FIBITMAP* aa_load_memory(BYTE* data, unsigned int size);
bool aa_unload(AaImage* image);

bool aa_output_ascii(AaImage* image, FILE* out);
bool aa_output_ansi16(AaImage* image, FILE* out);
bool aa_output_html_palette(AaImage* image, FILE* out);
bool aa_output_html(AaImage* image, FILE* out, bool palette = true);
bool aa_output_html_mono(AaImage* image, FILE* out);
