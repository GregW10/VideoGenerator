//
// created by Gregor Hartl Watters on 13/09/2022
//

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>

#ifndef errno
extern int errno;
#endif

#define PAD_24BPP(w) ((w)*3 % 4 == 0 ? 0 : 4 - ((3*(w)) % 4))

#define UND_TIME_MAX_LEN 25 // exact length of str. returned by get_und_time() func. (not including '\0')
#define PIX_OFFSET 54 // required starting address of the pixel offset in the BMP images

#pragma pack(push, 1)

typedef enum {
    false, true
} bool;

typedef struct {
    unsigned char b;
    unsigned char g;
    unsigned char r;
} colour;

typedef struct {
    unsigned char bm[2]; // should always be chars 'B' and 'M'
    unsigned int fileSize;
    unsigned short reserved1; // 0
    unsigned short reserved2; // 0
    unsigned int px_arr_offset; // offset of where the pixel array starts (54)
} bmp_header;

typedef struct {
    unsigned int header_size; // must be 40
    unsigned int bmp_width;
    unsigned int bmp_height;
    unsigned short num_panes; // must be 1
    unsigned short pixel_depth; // bpp
    unsigned int compression_method; // currently only supporting BI_RGB (no compression - so zero)
    unsigned int image_size; // can be zero for BI_RGB files
    unsigned int hor_res; // horizontal resolution (can be zero)
    unsigned int ver_res; // vertical resolution (can be zero)
    unsigned int clr_palette; // can be zero
    unsigned int important_clrs; // can be zero
} bmp_info_header;
#pragma pack(pop)
// no NULL checks in the bottom functions to reduce comp. time (UB if NULL passed)
static inline unsigned char get_Y(const colour col) { // get Luma (luminance) component from RGB values
    long double ld = 0.299l*((long double) col.r) + 0.587l*((long double) col.g) + 0.114l*((long double) col.b);
    unsigned char retval = (unsigned char) ld;
    if (ld - retval >= 0.5 && retval != 255) { // to avoid the function call to round() (in math.h)
        ++retval;
    }
    return retval;
}
// variadic arguments (<stdarg.h>) not used for the below functions in favour of computing time, since these functions
// can easily be called billions of times during one execution of the program
static inline unsigned char get_Cb(const colour col) { // get Cb (blue chrominance) component from RGB values
    long double ld = -0.169l*((long double)col.r) - 0.331l*((long double)col.g) + 0.500l*((long double)col.b) + 128;
    unsigned char retval = (unsigned char) ld;
    if (ld - retval >= 0.5 && retval != 255) {
        ++retval;
    }
    return retval;
}

static inline unsigned char get_Cb_avg2(const colour col, const colour col2) { // Cb average from two RGB colours
    long double ld = -0.169l*((long double)col.r) - 0.331l*((long double)col.g) + 0.500l*((long double)col.b) + 128;
    long double ld2 = -0.169l*((long double)col2.r)-0.331l*((long double)col2.g)+0.500l*((long double)col2.b) + 128;
    long double avg = ((ld + ld2)/2); // exactly the same as individ. taking avgs of R,G,&B and then doing calc.
    unsigned char retval = (unsigned char) avg;
    if (avg - retval >= 0.5 && retval != 255) {
        ++retval;
    }
    return retval;
}
// average Cb value for four RGB colours:
static inline unsigned char get_Cb_avg4(const colour col, const colour col2, const colour col3, const colour col4) {
    long double ld = -0.169l*((long double)col.r) - 0.331l*((long double)col.g) + 0.500l*((long double)col.b) + 128;
    long double ld2 = -0.169l*((long double)col2.r)-0.331l*((long double)col2.g)+0.500l*((long double)col2.b) + 128;
    long double ld3 = -0.169l*((long double)col3.r)-0.331l*((long double)col3.g)+0.500l*((long double)col3.b) + 128;
    long double ld4 = -0.169l*((long double)col4.r)-0.331l*((long double)col4.g)+0.500l*((long double)col4.b) + 128;
    long double avg = ((ld + ld2 + ld3 + ld4)/4);
    unsigned char retval = (unsigned char) avg;
    if (avg - retval >= 0.5 && retval != 255) {
        ++retval;
    }
    return retval;
}
// average Cb value for 8 RGB colours: (again, no varargs to increase efficiency)
static inline unsigned char get_Cb_avg8(const colour col, const colour col2, const colour col3, const colour col4,
                                        const colour col5, const colour col6, const colour col7, const colour col8){
    long double ld = -0.169l*((long double)col.r) - 0.331l*((long double)col.g) + 0.500l*((long double)col.b) + 128;
    long double ld2 = -0.169l*((long double)col2.r)-0.331l*((long double)col2.g)+0.500l*((long double)col2.b) + 128;
    long double ld3 = -0.169l*((long double)col3.r)-0.331l*((long double)col3.g)+0.500l*((long double)col3.b) + 128;
    long double ld4 = -0.169l*((long double)col4.r)-0.331l*((long double)col4.g)+0.500l*((long double)col4.b) + 128;
    long double ld5 = -0.169l*((long double)col5.r)-0.331l*((long double)col5.g)+0.500l*((long double)col5.b) + 128;
    long double ld6 = -0.169l*((long double)col6.r)-0.331l*((long double)col6.g)+0.500l*((long double)col6.b) + 128;
    long double ld7 = -0.169l*((long double)col7.r)-0.331l*((long double)col7.g)+0.500l*((long double)col7.b) + 128;
    long double ld8 = -0.169l*((long double)col8.r)-0.331l*((long double)col8.g)+0.500l*((long double)col8.b) + 128;
    long double avg = ((ld + ld2 + ld3 + ld4 + ld5 + ld6 + ld7 + ld8)/4);
    unsigned char retval = (unsigned char) avg;
    if (avg - retval >= 0.5 && retval != 255) {
        ++retval;
    }
    return retval;
}

static inline unsigned char get_Cr(const colour col) { // get Cr (red chrominance) component from RGB values
    long double ld = 0.500l*((long double) col.r) - 0.419l*((long double) col.g) - 0.081l*((long double)col.b) + 128;
    unsigned char retval = (unsigned char) ld;
    if (ld - retval >= 0.5 && retval != 255) {
        ++retval;
    }
    return retval;
}

static inline unsigned char get_Cr_avg2(const colour col, const colour col2) { // Cr average from two RGB colours
    long double ld = 0.500l*((long double)col.r) - 0.419l*((long double)col.g) - 0.081l*((long double)col.b) + 128;
    long double ld2 = 0.500l*((long double)col2.r)-0.419l*((long double)col2.g)-0.081l*((long double)col2.b) + 128;
    long double avg = ((ld + ld2)/2);
    unsigned char retval = (unsigned char) avg;
    if (avg - retval >= 0.5 && retval != 255) {
        ++retval;
    }
    return retval;
}
// for 4 RGB colours:
static inline unsigned char get_Cr_avg4(const colour col, const colour col2, const colour col3, const colour col4) {
    long double ld = 0.500l*((long double)col.r) - 0.419l*((long double)col.g) - 0.081l*((long double)col.b) + 128;
    long double ld2 = 0.500l*((long double)col2.r)-0.419l*((long double)col2.g)-0.081l*((long double)col2.b) + 128;
    long double ld3 = 0.500l*((long double)col3.r)-0.419l*((long double)col3.g)-0.081l*((long double)col3.b) + 128;
    long double ld4 = 0.500l*((long double)col4.r)-0.419l*((long double)col4.g)-0.081l*((long double)col4.b) + 128;
    long double avg = ((ld + ld2 + ld3 + ld4)/4);
    unsigned char retval = (unsigned char) avg;
    if (avg - retval >= 0.5 && retval != 255) {
        ++retval;
    }
    return retval;
}
// for 8 RGB colours
static inline unsigned char get_Cr_avg8(const colour col, const colour col2, const colour col3, const colour col4,
                                        const colour col5, const colour col6, const colour col7, const colour col8){
    long double ld = 0.500l*((long double)col.r) - 0.419l*((long double)col.g) - 0.081l*((long double)col.b) + 128;
    long double ld2 = 0.500l*((long double)col2.r)-0.419l*((long double)col2.g)-0.081l*((long double)col2.b) + 128;
    long double ld3 = 0.500l*((long double)col3.r)-0.419l*((long double)col3.g)-0.081l*((long double)col3.b) + 128;
    long double ld4 = 0.500l*((long double)col4.r)-0.419l*((long double)col4.g)-0.081l*((long double)col4.b) + 128;
    long double ld5 = 0.500l*((long double)col5.r)-0.419l*((long double)col5.g)-0.081l*((long double)col5.b) + 128;
    long double ld6 = 0.500l*((long double)col6.r)-0.419l*((long double)col6.g)-0.081l*((long double)col6.b) + 128;
    long double ld7 = 0.500l*((long double)col7.r)-0.419l*((long double)col7.g)-0.081l*((long double)col7.b) + 128;
    long double ld8 = 0.500l*((long double)col8.r)-0.419l*((long double)col8.g)-0.081l*((long double)col8.b) + 128;
    long double avg = ((ld + ld2 + ld3 + ld4 + ld5 + ld6 + ld7 + ld8)/4);
    unsigned char retval = (unsigned char) avg;
    if (avg - retval >= 0.5 && retval != 255) {
        ++retval;
    }
    return retval;
}

static inline void print_colour(const colour *col) {
    if (!col) {
        return;
    }
    printf("R: %i, G: %i, B: %i\n", col->r, col->g, col->b);
}

void zero(void *str, size_t n) {
    if (!str)
        return;
    char *ptr = str;
    while (n --> 0) {
        *ptr++ = 0;
    }
}

static inline bool is_digit_c(char c) {
    return c >= 48 && c <= 57;
}

bool is_numeric(const char *str) {
    if (!str || !*str) {
        return false;
    }
    while (*str) {
        if (!is_digit_c(*str++)) {
            return false;
        }
    }
    return true;
}

size_t strlen_c(const char *str) {
    if (!str || !*str)
        return 0;
    size_t count = 0;
    while (*str++) ++count;
    return count;
}

int strcmp_c(const char *restrict str1, const char *str2) {
    if (!str1 || !str2) {
        return -128;
    }
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            return *str1 - *str2;
        }
        ++str1;
        ++str2;
    }
    return *str1 - *str2;
}

int alfcmp_c(const char *restrict str1, const char *str2) { // treats upper and lower-case letters equally
    if (!str1 || !str2) {
        return -128;
    }
    while (*str1 && *str2) {
        if (*str1 != *str2) {
            if (*str1 >= 65 && *str1 <= 90) {
                if (*str2 - *str1 == 32) {
                    goto inc;
                }
                if (*str2 >= 97 && *str2 <= 122) {
                    return *str1 - *str2 + 32;
                }
            }
            if (*str1 >= 97 && *str1 <= 122) {
                if (*str1 - *str2 == 32) {
                    goto inc;
                }
                if (*str2 >= 65 && *str2 <= 97) {
                    return *str1 - *str2 - 32;
                }
            }
            return *str1 - *str2;
        }
        inc:
        ++str1;
        ++str2;
    }
    return *str1 - *str2;
}

char *strcpy_c(char *restrict dst, const char *src) {
    if (!dst || !src || !*src) {
        return dst;
    }
    char *org = dst;
    while ((*dst++ = *src++));
    return org;
}

char *strcat_c(char *restrict dst, const char *src) {
    if (!dst || !src || !*src) {
        return dst;
    }
    char *org = dst;
    if (!*dst)
        goto start;
    while (*++dst);
    start:
    while ((*dst++ = *src++));
    return org;
}

char *chrcat_c(char *restrict dst, char ch) {
    if (!dst || !ch) {
        return dst;
    }
    if (*dst == 0) {
        *dst = ch;
        goto end;
    }
    while (*++dst);
    *dst = ch;
    end:
    *++dst = 0;
    return dst;
}

bool endswith(const char *str, const char *suffix) {
    if (!str || !suffix) {
        return false;
    }
    if ((*str == 0 && *suffix == 0) || *suffix == 0) {
        return true;
    }
    size_t str_len = strlen_c(str);
    size_t suff_len = strlen_c(suffix);
    if (suff_len > str_len) {
        return false;
    }
    if (str_len == suff_len) {
        if (strcmp_c(str, suffix) == 0) {
            return true;
        }
        return false;
    }
    const char *start = str + str_len - suff_len;
    while (*start) {
        if (*start++ != *suffix++)
            return false;
    }
    return true;
}

void alphabetical_sort(const char **array) { // sorts an array of strings alphabetically - must end in NULL
    if (!array || !*array || !*(array + 1))
        return;
    const char *temp;
    bool sorted = false;
    int comp;
    const char **prev;
    const char **arr;
    while (!sorted) {
        arr = array;
        prev = arr++;
        sorted = true;
        while (*arr) {
            comp = alfcmp_c(*prev, *arr);
            if (comp > 0) {
                sorted = false;
                temp = *prev;
                *prev = *arr;
                *arr = temp;
            }
            ++prev;
            ++arr;
        }
    }
}

void print_array(const char *const *array) { // must end in NULL
    if (!array || !*array || !**array) {
        return;
    }
    while (*array) {
        printf("%s\n", *array++);
    }
}

void free_array(const char **array) {
    if (!array) {
        return;
    }
    const char **ptr = array;
    while (*ptr) {
        free((char *) *ptr++);
    }
    free((char *) *ptr); // frees final NULL
    free(array); // free array of pointers
}

void free_ptrs(size_t num, ...) {
    if (!num) {
        return;
    }
    va_list ptr;
    va_start(ptr, num);
    const char *p;
    for (size_t i = 0; i < num; ++i) {
        p = va_arg(ptr, const char *);
        if (p)
            free((char *) p);
    }
    va_end(ptr);
}

const char *get_und_time(void) {
    time_t t = time(NULL);
    char *ptr = ctime(&t);
    const char *org = ptr;
    if (!is_digit_c(*(ptr + 8))) // case for first 9 days of the month
        *(ptr + 8) = 48;
    *(ptr + 13) = 'h'; // replace first colon
    *(ptr + 16) = 'm'; // replace second colon
    *(ptr + 19) = 's'; // replace last underscore
    char *prev = ptr + 23;
    char *end = ptr + 24;
    while (*prev != 's') { // shift year forward by one character to make space for new underscore
        *end-- = *prev--;
    }
    *end = '_';
    while (*ptr) {
        if (*ptr == ' ')
            *ptr = '_';
        ++ptr;
    }
    return org;
}

char *append_integer(char *str, long long num) {
    if (!str) {
        return str;
    }
    if (!num) {
        strcat_c(str, "0");
        return str;
    }
    if (num < 0) {
        strcat_c(str, "-");
        num = -num;
    }
    char *ptr = str;
    size_t reverse = 0; // get reverse of the number
    char n = 0;
    while (num > 0) {
        reverse *= 10;
        reverse += num % 10;
        num /= 10;
        ++n;
    }
    if (*str == 0)
        goto start;
    while (*++str);
    start:
    while (n --> 0) {
        *str++ = ((char) (reverse % 10)) + 48; // get ASCII char
        reverse /= 10;
    }
    *str = 0;
    return ptr;
}

bool is_normal_ascii(const char *str) { // checks for 'normal' ASCII characters
    if (!str || !*str) {
        return false;
    }
    while (*str) {
        if (*str < 33 || *str == 127)
            return false;
        ++str;
    }
    return true;
}

const char *yuv_header(unsigned short width, unsigned short height, long long fr_num,
                       long long fr_denom, char interlacing, long long pix_asp_ratio_num,
                       long long pix_asp_ratio_denom, const char *clr_space, const char *x_param) {
    if (!width || !height || fr_num <= 0 || fr_denom <= 0 || (interlacing != 'p' && interlacing != 't' &&
    interlacing != 'b' && interlacing != 'm') || pix_asp_ratio_num <= 0 || pix_asp_ratio_denom <= 0 || !clr_space
    || !*clr_space || (strcmp_c(clr_space, "C420mpeg2") != 0 && strcmp_c(clr_space, "C444alpha") != 0 &&
    strcmp_c(clr_space, "C420jpeg") != 0 && strcmp_c(clr_space, "C420paldv") != 0 && strcmp_c(clr_space, "C411") != 0
    && strcmp_c(clr_space, "C420") != 0 && strcmp_c(clr_space, "C422") != 0 && strcmp_c(clr_space, "C444")
    && strcmp_c(clr_space, "Cmono") != 0 && strcmp_c(clr_space, "C410") != 0)) {
        errno = EINVAL;
        return NULL;
    }
    size_t xlen = strlen_c(x_param); // will just be zero if x_param is NULL
    bool include_x = true;
    if (xlen == 0 || xlen > 200 || !is_normal_ascii(x_param)) { // x_param is optional
        include_x = false;
    }
    size_t len = 112 + strlen_c(clr_space) + (include_x ? xlen : 0);  // max. len. of header
    char *str = malloc(sizeof(char)*len);
    zero(str, sizeof(char)*len);
    strcpy_c(str, "YUV4MPEG2 W");
    append_integer(str, width);
    strcat_c(str, " H");
    append_integer(str, height);
    strcat_c(str, " F");
    append_integer(str, fr_num);
    chrcat_c(str, ':');
    append_integer(str, fr_denom);
    strcat_c(str, " I");
    chrcat_c(str, interlacing);
    strcat_c(str, " A");
    append_integer(str, pix_asp_ratio_num);
    chrcat_c(str, ':');
    append_integer(str, pix_asp_ratio_denom);
    chrcat_c(str, ' ');
    strcat_c(str, clr_space);
    if (include_x) {
        strcat_c(str, " X"); // x_param passed should not begin with X, as this is taken care of here
        strcat_c(str, x_param);
    }
    chrcat_c(str, '\n'); // is necessary
    return str;
}

static inline void check_dim(FILE *fp, const char *str) {
    static unsigned int expected_width = 0;
    static unsigned int expected_height;
    static unsigned int width;
    static unsigned int height;
    if (expected_width == 0) {
        fseek(fp, 18, SEEK_SET);
        fread(&expected_width, sizeof(unsigned int), 1, fp);
        fread(&expected_height, sizeof(unsigned int), 1, fp);
        return;
    }
    fseek(fp, 18, SEEK_SET);
    fread(&width, sizeof(unsigned int), 1, fp);
    fread(&height, sizeof(unsigned int), 1, fp);
    if (width != expected_width || height != expected_height) {
        fprintf(stderr, "The dimensions of BMP image \"%s\" do not match that of the first BMP.\n", str);
        abort();
    }
}

void for_each(void *arr, size_t element_size, size_t count, void (*func)(void*)) {
    if (!arr || !func || !element_size || !count)
        return;
    for (size_t i = 0; i < count; ++i, arr += element_size) {
        func(arr);
    }
}

void to_ycbcr(void *bgr) {
    static colour *col;
    static unsigned char Y;
    static unsigned char Cb;
    col = bgr;
    Y = get_Y(*col);
    Cb = get_Cb(*col);
    col->r = get_Cr(*col);
    col->b = Y;
    col->g = Cb;
}

void correct_order(const unsigned char *input_ycbcr, unsigned char *output, size_t num_elements) {
    static const unsigned char *ptr;
    static size_t i;
    ptr = input_ycbcr;
    for (i = 0; i < num_elements; ++i, ptr += 3) {
        *output++ = *ptr;
    }
    printf("i: %zu\n", i);
    printf("output: %p, ptr: %p", output, ptr);
    ptr = input_ycbcr + 1;
    for (i = 0; i < num_elements; ++i, ptr += 3) {
        *output++ = *ptr;
    }
    printf("i: %zu\n", i);
    printf("output: %p, ptr: %p", output, ptr);
    ptr = input_ycbcr + 2;
    for (i = 0; i < num_elements; ++i, ptr += 3) {
        *output++ = *ptr;
    }
    printf("i: %zu\n", i);
    printf("output: %p, ptr: %p", output, ptr);
}
