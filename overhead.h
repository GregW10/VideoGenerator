//
// created by Gregor Hartl Watters on 13/09/2022
//

#include <stdarg.h>
#include <time.h>
#include <errno.h>

#ifndef errno
extern int errno;
#endif

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
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int px_arr_offset; // offset of where the pixel array starts
} bmp_header;

typedef struct {
    unsigned int header_size; // must be 40
    unsigned int bmp_width;
    unsigned int bmp_height;
    unsigned short num_panes; // must be 1
    unsigned short pixel_depth; // bpp
    unsigned int compression_method; // currently only supporting BI_RGB (no compression)
    unsigned int image_size; // should be zero for BI_RGB files
    unsigned int hor_res; // horizontal resolution (can be zero)
    unsigned int ver_res; // vertical resolution (can be zero)
    unsigned int clr_palette; // can be zero
    unsigned int important_clrs; // can be zero
} bmp_info_header;

static inline unsigned char get_Y(const colour *col) { // get Luma (luminance) component from RGB values
    long double ld = 0.299l*((long double) col->r) + 0.587l*((long double) col->g) + 0.114l*((long double) col->b);
    unsigned char retval = (unsigned char) ld;
    if (ld - retval >= 0.5) { // to avoid the function call to round() (in math.h)
        ++retval;
    }
    return retval;
}

static inline unsigned char get_Cb(const colour *col) { // get Cb (blue chrominance) component from RGB values
    long double ld = -0.169l*((long double)col->r) + -0.331l*((long double)col->g) + 0.500l*((long double)col->b) + 128;
    unsigned char retval = (unsigned char) ld;
    if (ld - retval >= 0.5) { // to avoid the function call to round() (in math.h)
        ++retval;
    }
    return retval;
}

static inline unsigned char get_Cr(const colour *col) { // get Cr (red chrominance) component from RGB values
    long double ld = 0.500l*((long double) col->r) - 0.419l*((long double) col->g) - 0.081l*((long double)col->b) + 128;
    unsigned char retval = (unsigned char) ld;
    if (ld - retval >= 0.5) { // to avoid the function call to round() (in math.h)
        ++retval;
    }
    return retval;
}

void zero(void *str, size_t n) {
    if (str == NULL)
        return;
    char *ptr = str;
    while (n --> 0) {
        *ptr++ = 0;
    }
}

size_t strlen_c(const char *str) {
    if (str == NULL || *str == 0)
        return 0;
    size_t count = 0;
    while (*str++) ++count;
    return count;
}

int strcmp_c(const char *restrict str1, const char *str2) {
    if (str1 == NULL || str2 == NULL) {
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
    if (str1 == NULL || str2 == NULL) {
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
    if (dst == NULL || src == NULL || *src == 0) {
        return dst;
    }
    char *org = dst;
    while ((*dst++ = *src++));
    return org;
}

char *strcat_c(char *restrict dst, const char *src) {
    if (dst == NULL || src == NULL || *src == 0) {
        return dst;
    }
    char *org = dst;
    if (*dst == 0)
        goto start;
    while (*++dst);
    start:
    while ((*dst++ = *src++));
    return org;
}

char *chrcat_c(char *restrict dst, char ch) {
    if (dst == NULL || ch == 0) {
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
    if (str == NULL || suffix == NULL) {
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
    if (array == NULL || *array == NULL || *(array + 1) == NULL)
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
    if (array == NULL || *array == NULL || **array == 0) {
        return;
    }
    while (*array) {
        printf("%s\n", *array++);
    }
}

void free_array(const char **array) {
    if (array == NULL) {
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
    if (num == 0) {
        return;
    }
    va_list ptr;
    va_start(ptr, num);
    for (size_t i = 0; i < num; ++i) {
        free((char *) (va_arg(ptr, const char *)));
    }
    va_end(ptr);
}

const char *get_und_time() {
    time_t t = time(NULL);
    char *ptr = ctime(&t);
    const char *org = ptr;
    *(ptr + 13) = 'h'; // replace first colon
    *(ptr + 16) = 'm'; // replace second colon
    *(ptr + 19) = 's'; // replace last underscore
    char *prev = ptr + 23;
    char *end = ptr + 24;
    printf("*prev: %c, *end: %c\n", *prev, *end);
    while (*prev != 's') { // shift year forward by one character to make space for new underscore
        *end-- = *prev--;
    }
    *end = '_';
    while (*ptr) {
        if (*ptr == ' ')
            *ptr = '_';
        ++ptr;
    }
    // *--ptr = 0; // ctime returned string ends in a newline '\n'
    return org;
}

char *append_integer(char *str, long long num) {
    if (str == NULL) {
        return str;
    }
    if (num == 0) {
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
    if (str == NULL || *str == 0) {
        return false;
    }
    while (*str) {
        if (*str < 33 || *str == 127)
            return false;
        ++str;
    }
    return true;
}

static inline bool is_num(char c) {
    return c >= 48 && c <= 57;
}

bool is_numeric(const char *str) {
    if (str == NULL || *str == 0) {
        return false;
    }
    while (*str) {
        if (!is_num(*str++)) {
            return false;
        }
    }
    return true;
}

const char *yuv_header(unsigned short width, unsigned short height, long long fr_num,
                       long long fr_denom, char interlacing, long long pix_asp_ratio_num,
                       long long pix_asp_ratio_denom, const char *clr_space, const char *x_param) {
    if (width == 0 || height == 0 || fr_num <= 0 || fr_denom <= 0 || (interlacing != 'p' && interlacing != 't' &&
    interlacing != 'b' && interlacing != 'm') || pix_asp_ratio_num <= 0 || pix_asp_ratio_denom <= 0 || clr_space == NULL
    || *clr_space == 0 || (strcmp_c(clr_space, "C420mpeg2") != 0 && strcmp_c(clr_space, "C444alpha") != 0 &&
    strcmp_c(clr_space, "C420jpeg") != 0 && strcmp_c(clr_space, "C420paldv") != 0 && strcmp_c(clr_space, "C420") != 0 &&
    strcmp_c(clr_space, "C422") != 0 && strcmp_c(clr_space, "C444") && strcmp_c(clr_space, "Cmono") != 0)) {
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
#pragma pack(pop)