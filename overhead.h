//
// created by Gregor Hartl Watters on 13/09/2022
//

#pragma once

#define Y4M_VERSION "1.0.0"

#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h> // even though all these header files have been ported to Windows, they tend to be wrappers around
#include <dirent.h> // native Windows API functions, so I prefer to use the Windows API functions directly when on Win.
#include <unistd.h>
#endif

#include <signal.h>

#ifndef _WIN32
#define RESET_TXT_FLAGS "\033[0m"
#define BLACK_TXT_START "\033[30m"
#define RED_TXT_START "\033[31m"
#define GREEN_TXT_START "\033[32m"
#define YELLOW_TXT_START "\033[33m"
#define BLUE_TXT_START "\033[34m"
#define MAGENTA_TXT_START "\033[35m"
#define CYAN_TXT_START "\033[36m"
#define WHITE_TXT_START "\033[37m"
#define BOLD_TXT_START "\033[1m"
#define UNDERLINED_TXT_START "\033[4m"
#define BLACK_TXT(str) BLACK_TXT_START str RESET_TXT_FLAGS
#define RED_TXT(str) RED_TXT_START str RESET_TXT_FLAGS
#define GREEN_TXT(str) GREEN_TXT_START str RESET_TXT_FLAGS
#define YELLOW_TXT(str) YELLOW_TXT_START str RESET_TXT_FLAGS
#define BLUE_TXT(str) BLUE_TXT_START str RESET_TXT_FLAGS
#define MAGENTA_TXT(str) MAGENTA_TXT_START str RESET_TXT_FLAGS
#define CYAN_TXT(str) CYAN_TXT_START str RESET_TXT_FLAGS
#define WHITE_TXT(str) WHITE_TXT_START str RESET_TXT_FLAGS
#define BOLD_TXT(str) BOLD_TXT_START str RESET_TXT_FLAGS
#define UNDERLINED_TXT(str) UNDERLINED_TXT_START str RESET_TXT_FLAGS
#else
#define RESET_TXT_FLAGS ""
#define BLACK_TXT_START ""
#define RED_TXT_START ""
#define GREEN_TXT_START ""
#define YELLOW_TXT_START ""
#define BLUE_TXT_START ""
#define MAGENTA_TXT_START ""
#define CYAN_TXT_START ""
#define WHITE_TXT_START ""
#define BOLD_TXT_START ""
#define UNDERLINED_TXT_START ""
#define BLACK_TXT(str) str
#define RED_TXT(str) str
#define GREEN_TXT(str) str
#define YELLOW_TXT(str) str
#define BLUE_TXT(str) str
#define MAGENTA_TXT(str) str
#define CYAN_TXT(str) str
#define WHITE_TXT(str) str
#define BOLD_TXT(str) str
#define UNDERLINED_TXT(str) str
#endif

#define LL_MIN (((long long) 1) << (sizeof(long long)*8 - 1))
#define LL_MAX (((long long) -1) >> 1)

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

static inline bool is_whitespace(char c) {
    return c >= 9 && c <= 13;
}

size_t replace(char *str, char to_replace, char replacement) {
    if (!str || *str)
        return 0;
    size_t count = 0;
    while (*str) {
        if (*str == to_replace) {
            *str = replacement;
            ++count;
        }
        ++str;
    }
    return count;
}

bool is_numeric(const char *str, bool allow_whitespace) {
    if (!str || !*str) {
        return false;
    }
    if (allow_whitespace) {
        while (*str) {
            if (!is_digit_c(*str) && !is_whitespace(*str)) {
                return false;
            }
            ++str;
        }
        return true;
    }
    while (*str) {
        if (!is_digit_c(*str++)) {
            return false;
        }
    }
    return true;
}

long long to_ll(const char *str, const char **end_char) {
    /* end_char (if not NULL) will point to the address of the first non-convertible character (the null byte if all
     * could be converted) */
    if (!str) {
        if (end_char)
            *end_char = NULL;
        return LL_MIN;
    }
    if (!*str) {
        if (end_char)
            *end_char = str;
        return LL_MIN;
    }
    bool negative = false;
    if (*str == '-') {
        ++str;
        if (!*str) {
            if (end_char)
                *end_char = str - 1;
            return LL_MIN;
        }
        negative = true;
    }
    long long ret = 0;
    size_t count = 0;
    while (*str) {
        if (!is_digit_c(*str)) {
            if (end_char)
                *end_char = str;
            return count == 0 ? LL_MIN : (negative ? -ret : ret);
        }
        ret *= 10;
        ret += *str++ - 48;
        ++count;
    }
    *end_char = str;
    return negative ? -ret : ret;
}

size_t strlen_c(const char *str) {
    if (!str || !*str)
        return 0;
    size_t count = 0;
    while (*str++) ++count;
    return count;
}

int strcmp_c(const char *str1, const char *str2) {
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

static inline bool equal(const char *str1, const char *str2) { // convenience function
    return strcmp_c(str1, str2) == 0;
}

static inline bool startswith_c(const char *str1, char c) {
    return str1 && *str1 == c;
}

bool startswith(const char *str, const char *substr) {
    if (!str || !substr)
        return false;
    while (*str && *substr) {
        if (*str++ != *substr++)
            return false;
    }
    return *substr == 0; // in case substr was longer than str
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
    for (size_t i = 0; i < count; ++i, arr += element_size)
        func(arr);
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

void output_444(const colour *input, unsigned char *output, size_t num_pixels){
    static const colour *ptr;
    static size_t i;
    ptr = input;
    for (i = 0; i < num_pixels; ++i, ++ptr) {
        *output++ = get_Y(*ptr);
    }
    ptr = input;
    for (i = 0; i < num_pixels; ++i, ++ptr) {
        *output++ = get_Cb(*ptr);
    }
    for (i = 0; i < num_pixels; ++i, ++input) {
        *output++ = get_Cr(*input);
    }
}

void output_422(const colour *input, unsigned char *output, size_t num_pixels){
    static const colour *ptr;
    static size_t i;
    static size_t half;
    half = num_pixels/2; // num_pixels is guaranteed to be even
    ptr = input;
    for (i = 0; i < num_pixels; ++i, ++ptr) {
        *output++ = get_Y(*ptr);
    }
    ptr = input;
    for (i = 0; i < half; ++i, ptr += 2) {
        *output++ = get_Cb_avg2(*ptr, *(ptr + 1));
    }
    for (i = 0; i < half; ++i, input += 2) {
        *output++ = get_Cr_avg2(*input, *(input + 1));
    }
}

void output_420(const colour *input, unsigned char *output, unsigned int width, unsigned int height){
    static const colour *ptr;
    static const colour *nxt;
    static unsigned int half_w;
    static unsigned int half_h;
    static unsigned int num_pixels;
    static unsigned int j;
    static unsigned int i;
    num_pixels = width*height;
    half_w = width/2; // both width and height are guaranteed to be divisible by 2
    half_h = height/2;
    ptr = input;
    for (i = 0; i < num_pixels; ++i, ++ptr) {
        *output++ = get_Y(*ptr);
    }
    ptr = input;
    nxt = input + width; // points to row "below" ptr
    for (j = 0; j < half_h; ++j) {
        for (i = 0; i < half_w; ++i, ptr += 2, nxt += 2) {
            *output++ = get_Cb_avg4(*ptr, *(ptr + 1), *nxt, *(nxt + 1));
        }
        ptr += width;
        nxt += width;
    }
    ptr = input;
    nxt = input + width;
    for (j = 0; j < half_h; ++j) {
        for (i = 0; i < half_w; ++i, ptr += 2, nxt += 2) {
            *output++ = get_Cr_avg4(*ptr, *(ptr + 1), *nxt, *(nxt + 1));
        }
        ptr += width;
        nxt += width;
    }
}

void output_411(const colour *input, unsigned char *output, size_t num_pixels){
    static const colour *ptr;
    static size_t i;
    static size_t quarter;
    quarter = num_pixels/4; // num_pixels is guaranteed to be divisible by 4
    ptr = input;
    for (i = 0; i < num_pixels; ++i, ++ptr) {
        *output++ = get_Y(*ptr);
    }
    ptr = input;
    for (i = 0; i < quarter; ++i, ptr += 4) {
        *output++ = get_Cb_avg4(*ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3));
    }
    for (i = 0; i < quarter; ++i, input += 4) {
        *output++ = get_Cr_avg4(*input, *(input + 1), *(input + 2), *(input + 3));
    }
}

/* 4:1:0 is not a good format - it has little support; not even VLC and ffmpeg can deal with it */
void output_410(const colour *input, unsigned char *output, unsigned int width, unsigned int height){
    static const colour *ptr;
    static const colour *nxt;
    static unsigned int quarter_w;
    static unsigned int half_h;
    static unsigned int num_pixels;
    static unsigned int j;
    static unsigned int i;
    num_pixels = width*height;
    quarter_w = width/4; // width will be divisible by 4
    half_h = height/2; // height will be divisible by 2
    ptr = input;
    for (i = 0; i < num_pixels; ++i, ++ptr) {
        *output++ = get_Y(*ptr);
    }
    ptr = input;
    nxt = input + width; // points to row "below" ptr
    for (j = 0; j < half_h; ++j) {
        for (i = 0; i < quarter_w; ++i, ptr += 4, nxt += 4) {
            *output++ = get_Cb_avg8(*ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3), *nxt, *(nxt + 1), *(nxt + 2), *(nxt + 3));
        }
        ptr += width;
        nxt += width;
    }
    ptr = input;
    nxt = input + width;
    for (j = 0; j < half_h; ++j) {
        for (i = 0; i < quarter_w; ++i, ptr += 4, nxt += 4) {
            *output++ = get_Cr_avg8(*ptr, *(ptr + 1), *(ptr + 2), *(ptr + 3), *nxt, *(nxt + 1), *(nxt + 2), *(nxt + 3));
        }
        ptr += width;
        nxt += width;
    }
}

static inline void start_frame(FILE *fp) {
    // static const char frame[] = {'F', 'R', 'A', 'M', 'E', '\n'};
    // fwrite(frame, sizeof(char), 6, fp);
    fputs("FRAME\n", fp);
}

static inline char file_sep(void) {
#ifndef _WIN32
    return '/';
#else
    return '\\';
#endif
}

const char *get_cur_dir(void) {
#ifndef _WIN32
    static char *cwd;
    static long MAX_CWD;
    if ((MAX_CWD = pathconf(".", _PC_PATH_MAX)) == -1) {
        MAX_CWD = 4096;
    }
    cwd = malloc(MAX_CWD);
    if (!getcwd(cwd, MAX_CWD)) {
        fprintf(stderr, "Directory was not specified, and current directory could not be obtained.\n");
        abort();
    }
#else
    static char cwd[MAX_PATH];
    if (!GetCurrentDirectory(MAX_PATH, cwd)) {
        fprintf(stderr, "Directory was not specified, and current directory could not be obtained.\n");
        abort();
    }
#endif
    return cwd;
}

_Noreturn void print_help(void) {
    printf("Help\n");
    exit(0);
}

_Noreturn void print_version(void) {
    printf("Version\n");
    exit(0);
}

_Noreturn void log_non_numeric_error(const char *str) {//, size_t start_index) {
    if (!str || !*str)
        abort();
    const char *ptr = str;
    fputs("\t\t", stderr);
    while (*ptr) {
        if (!is_digit_c(*ptr))
            fprintf(stderr, BOLD_TXT(RED_TXT("%c")), *ptr);
        else
            fprintf(stderr, GREEN_TXT("%c"), *ptr);
        ++ptr;
    }
    fputs("\n\t\t", stderr);
    while (*str) {
        if (!is_digit_c(*str))
            fprintf(stderr, BOLD_TXT(YELLOW_TXT("^")));
        else
            fprintf(stderr, UNDERLINED_TXT(CYAN_TXT("~")));
        ++str;
    }
    fputc('\n', stderr);
    abort();
}

_Noreturn void print_nonnum_fps(const char *str) {
    if (!str || !*str)
        abort();
    const char *ptr = str;
    fputs("\t\t", stderr);
    size_t count = 0;
    bool init_slash = *(str + 5) == '/' || *(str + strlen_c(str) - 1) == '/';
    bool have_slash = init_slash;
    while (*ptr) {
        if (!is_digit_c(*ptr) && count >= 5)
            fprintf(stderr, *ptr != '/' ? BOLD_TXT(RED_TXT("%c")) :
            (!have_slash ? have_slash = true, GREEN_TXT("%c") : BOLD_TXT(RED_TXT("%c"))), *ptr);
        else
            fprintf(stderr, GREEN_TXT("%c"), *ptr);
        ++ptr;
        ++count;
    }
    fputs("\n\t\t", stderr);
    count = 0;
    while (*str) {
        if (!is_digit_c(*str) && count >= 5)
            fprintf(stderr, (init_slash ? BOLD_TXT(YELLOW_TXT("^")) :
            (*str == '/' ? init_slash = true, CYAN_TXT("~") : BOLD_TXT(YELLOW_TXT("^")))));
        else
            fprintf(stderr, CYAN_TXT("~"));
        ++str;
        ++count;
    }
    fputc('\n', stderr);
    abort();
}

_Noreturn void log_floating_error(const char *str, int pos) {
    if (!str || !*str)
        abort();
    fprintf(stderr, GREEN_TXT("\t\t%*c") BOLD_TXT(RED_TXT(" %s\n")), pos, '^', str);
    abort();
}

void process_argv(int argc, char **argv, bool *del, bool *timed, bool *sized, bool *prog, const char **path_to_vid,
                  const char **path_to_folder, long long *rate_num, long long *rate_denom, const char **subsampling) {
    static char sub[] = "420";
    *del = false;
    *timed = false;
    *sized = false;
    *prog = false;
    *path_to_vid = NULL;
    *path_to_folder = NULL;
    *rate_num = 30;
    *rate_denom = 1;
    *subsampling = sub;
    if (argc == 1) {
        *path_to_folder = get_cur_dir();
        return;
    }
    ++argv;
    bool have_path = false;
    for (unsigned int i = 1; i < argc; ++i, ++argv) {
        if (have_path) {
            *path_to_vid = *argv;
            have_path = false;
            continue;
        }
        if (equal(*argv, "-o")) {
            if (i == argc - 1) {
                fprintf(stderr, BOLD_TXT(RED_TXT("Error:")) UNDERLINED_TXT(BLUE_TXT(" no file specified after the"))
                                YELLOW_TXT(" \"-o\" ") UNDERLINED_TXT(BLUE_TXT(" option.\n")));
                abort();
            }
            have_path = true;
            continue;
        }
        if (startswith_c(*argv, '-')) {
            if (*(*argv + 1) == 0) {
                fprintf(stderr, BOLD_TXT(RED_TXT("Error:")) UNDERLINED_TXT(BLUE_TXT(" invalid option specified:"))
                                YELLOW_TXT(" \"%s\" ") UNDERLINED_TXT(BLUE_TXT(".\n")), *argv);
                abort();
            }
            if (*(*argv + 1) == '-') {
                if (equal(*argv + 2, "help") || equal(*argv + 2, "h")) {
                    print_help(); // exits program
                }
                if (equal(*argv + 2, "version") || equal(*argv + 2, "v")) {
                    print_version();
                }
                fprintf(stderr, BOLD_TXT(RED_TXT("Error:")) UNDERLINED_TXT(BLUE_TXT(" invalid option specified:\n")));
                int char_count =
                fprintf(stderr, GREEN_TXT("\t\t\"%s\"\n"), *argv);
                log_floating_error("Expected flag", char_count - 2);
            }
            if (*(*argv + 1) == 'h') {
                if (*(*argv + 2) != 0) {
                    fprintf(stderr, BOLD_TXT(RED_TXT("Error:"))
                                    YELLOW_TXT(" \"-h\" ")
                                    UNDERLINED_TXT(BLUE_TXT(" option must be specified on its own.\nInstead found:"))
                                    YELLOW_TXT(" \"%s\" ") UNDERLINED_TXT(BLUE_TXT("after")) YELLOW_TXT(" \"-h\" "),
                                    *argv + 2);
                    abort();
                }
                print_help();
            }
            if (startswith(*argv, "-fps")) {
                if (*(*argv + 4) == 0 || *(*argv + 4) != '=' || *(*argv + 5) == 0) {
                    fprintf(stderr, BOLD_TXT(RED_TXT("Error:"))
                                    YELLOW_TXT(" \"-fps\" ")
                                    UNDERLINED_TXT(BLUE_TXT(" (frames-per-second) option must be specified in the "
                                                            "following format:\n")) YELLOW_TXT(" \"-fps=<num>\" ")
                                    UNDERLINED_TXT(BLUE_TXT("or")) YELLOW_TXT(" \"-fps=<num>/<num>\" ")
                                    UNDERLINED_TXT(BLUE_TXT("\nwhere ")) YELLOW_TXT(" \"<num>\" ")
                                    UNDERLINED_TXT(BLUE_TXT("tag/s is/are replaced by a/two positive integer/s.\n"
                                                            "Instead found: ")) YELLOW_TXT(" \"%s\"\n"), *argv);
                    abort();
                }
                const char *end_char;
                *rate_num = to_ll(*argv + 5, &end_char);
                if (end_char == *argv + 5 || (*end_char != 0 && *end_char != '/')) {
                    fprintf(stderr, BOLD_TXT(RED_TXT("Error:"))
                                    UNDERLINED_TXT(BLUE_TXT(" non-numeric characters were passed in the first "))
                                    YELLOW_TXT(" \"<num>\" ")
                                    UNDERLINED_TXT(BLUE_TXT("tag in the ")) YELLOW_TXT(" \"-fps=<num>\" ")
                                    UNDERLINED_TXT(BLUE_TXT("or"))
                                    YELLOW_TXT(" \"-fps=<num>/<num>\" ")
                                    UNDERLINED_TXT(BLUE_TXT("argument:\n")));
                    print_nonnum_fps(*argv);
                }
                if (*end_char == '/') {
                    if (*(end_char + 1) == 0) {
                        fprintf(stderr,
                                BOLD_TXT(RED_TXT("Error:"))
                                UNDERLINED_TXT(BLUE_TXT(" no denominator value passed in"))
                                YELLOW_TXT(" \"-fps=<num>/<num>\" ")
                                UNDERLINED_TXT(BLUE_TXT("argument:\n")));
                        int char_count =
                        fprintf(stderr, "\t\t%s\n", *argv);
                        log_floating_error("Expected value", char_count - 2);
                    }
                    const char *second_end;
                    *rate_denom = to_ll(end_char + 1, &second_end);
                    if (end_char + 1 == second_end || *second_end != 0) {
                        fprintf(stderr, BOLD_TXT(RED_TXT("Error:"))
                                        UNDERLINED_TXT(BLUE_TXT(" non-numeric characters were passed in the second "))
                                        YELLOW_TXT(" \"<num>\" ")
                                        UNDERLINED_TXT(BLUE_TXT("tag in the "))
                                        YELLOW_TXT(" \"-fps=<num>/<num>\" ")
                                        UNDERLINED_TXT(BLUE_TXT("argument:\n")));
                        print_nonnum_fps(*argv);
                    }
                }
                if (!*rate_num || rate_num < 0) {
                    fprintf(stderr,
                            BOLD_TXT(RED_TXT("Error:"))
                            UNDERLINED_TXT(BLUE_TXT(" frame rate numerator determined to be zero or negative:"))
                            GREEN_TXT(" %lld ")
                            UNDERLINED_TXT(BLUE_TXT("\nDo not enter a value less than or equal to zero,"))
                            UNDERLINED_TXT(BLUE_TXT(" or greater than \n"))
                            YELLOW_TXT(" %lld\n"), *rate_num, LL_MAX);
                    abort();
                }
                if (!*rate_denom || rate_denom < 0) {
                    fprintf(stderr,
                            BOLD_TXT(RED_TXT("Error:"))
                            UNDERLINED_TXT(BLUE_TXT(" frame rate denominator determined to be zero or negative:"))
                            GREEN_TXT(" %lld ")
                            UNDERLINED_TXT(BLUE_TXT("\nDo not enter a value less than or equal to zero,"))
                            UNDERLINED_TXT(BLUE_TXT(" or greater than \n"))
                            YELLOW_TXT(" %lld\n"), *rate_num, LL_MAX);
                    abort();
                }
                continue;
            }
            if (startswith(*argv, "-sub") || startswith(*argv, "-clr")) {
                if (*(*argv + 4) != '=' || *(*argv + 5) == 0) {
                    fprintf(stderr, BOLD_TXT(RED_TXT("Error:"))
                                    YELLOW_TXT(" \"-sub\" ") UNDERLINED_TXT(BLUE_TXT("or")) YELLOW_TXT(" \"-clr\" ")
                                    UNDERLINED_TXT(BLUE_TXT(" (color-subsampling) option must be specified in the "
                                                            "following format:\n")) YELLOW_TXT(" \"-fps=<num>\" ")
                                    UNDERLINED_TXT(BLUE_TXT("\nwhere the ")) YELLOW_TXT(" \"<num>\" ")
                                    UNDERLINED_TXT(BLUE_TXT("tag is replaced by one of:\n"))
                                    YELLOW_TXT("\t\"444\n\"")
                                    YELLOW_TXT("\t\"422\n\"")
                                    YELLOW_TXT("\t\"420\n\"")
                                    YELLOW_TXT("\t\"411\"") BLUE_TXT(" or\n")
                                    YELLOW_TXT("\t\"410\n\"") UNDERLINED_TXT(BLUE_TXT(" (not recommended)\n"))
                                    UNDERLINED_TXT(BLUE_TXT("Instead found: "))
                                    YELLOW_TXT(" \"%s\"\n"), *argv + 4);
                    abort();
                }
                if (!equal(*argv + 5, "444") && !equal(*argv + 5, "422") && !equal(*argv + 5, "420") &&
                    !equal(*argv + 5, "411") && !equal(*argv + 5, "410")) {
                    fprintf(stderr, BOLD_TXT(RED_TXT("Error:"))
                    UNDERLINED_TXT(BLUE_TXT("invalid color-subsampling option:\n")));
                    fprintf(stderr, "\t\t%s", *argv);
                    log_floating_error("Expected "
                                       YELLOW_TXT("\"444\"") BLUE_TXT(", ")
                                       YELLOW_TXT("\"422\"") BLUE_TXT(", ")
                                       YELLOW_TXT("\"420\"") BLUE_TXT(", ")
                                       YELLOW_TXT("\"411\"") BLUE_TXT(" or ")
                                       YELLOW_TXT("\"410\"\n"), 5);
                }
                strcpy_c(sub, *argv + 5);
                continue;
            }
            char *ptr = *argv + 1;
            for (size_t count = 1; *ptr; ++count, ++ptr) {
                if (*ptr == 'd') {
                    *del = true;
                    continue;
                }
                if (*ptr == 'p') {
                    *prog = true;
                    continue;
                }
                if (*ptr == 't') {
                    *timed = true;
                    continue;
                }
                if (*ptr == 's') {
                    *sized = true;
                    continue;
                }
                if (*ptr == 'h') {
                    fprintf(stderr, RED_TXT(BOLD_TXT("Error:")) GREEN_TXT("\t\t%s\n"), *argv);
                    log_floating_error("must be specified as single option\n", count + 1);
                }
                fprintf(stderr, RED_TXT(BOLD_TXT("Error:")) GREEN_TXT("\t\t%s\n"), *argv);
                log_floating_error("invalid flag\n", count + 1);
            }
            continue;
        }
        *path_to_folder = *argv;
    }
    if (!*path_to_folder)
        *path_to_folder = get_cur_dir();
}
