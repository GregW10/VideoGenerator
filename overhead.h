//
// created by Gregor Hartl Watters on 13/09/2022
//

#pragma pack(push, 1)

typedef enum {
    false, true
} bool;

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
                    return *str1 - *str2 + 32;
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

void alphabetical_sort(const char **array) { // sorts an array of strings alphabetically - must end in NULL
    if (array == NULL || *array == NULL || *(array + 1) == NULL)
        return;
    const char *temp;
    bool sorted = false;
    int comp;
    const char **prev;
    const char **arr;
    size_t count = 1;
    while (!sorted) {
        arr = array;
        prev = arr++;
        printf("Entrance number: %d\n", count++);
        sorted = true;
        while (*arr) {
            comp = alfcmp_c(*prev, *arr);
            printf("*prev = %s, *array = %s, comp = %d\n", *prev, *arr, comp);
            if (comp > 0) {
                sorted = false;
                temp = *prev;
                *prev = *arr;
                *arr = temp;
                printf("switch\n");
            }
            ++prev;
            ++arr;
            printf("first element: *array: %s\n", *array);
        }
    }
    printf("End: %s\n", *array);
}

void print_array(const char *const *array) { // must end in NULL
    if (array == NULL || *array == NULL || **array == 0) {
        return;
    }
    while (*array) {
        printf("%s\n", *array++);
    }
}

#pragma pack(pop)