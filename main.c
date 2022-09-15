//
// created by Gregor Hartl Watters on 13/09/2022
//

#include <stdio.h>
#include <stdlib.h>
#include "overhead.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h> // even though all these header files have been ported to Windows, they tend to be wrappers around
#include <dirent.h> // native Windows API functions, so I prefer to use the Windows API functions directly when on Win.
#include <unistd.h>
#endif

#define MIN_ARR_SIZE 32

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "Invalid number of command-line arguments provided.\n");
        return -1;
    }
    const char *fr = *(argv + 2);
    if (!is_numeric(*(argv + 2))) {
        fprintf(stderr, "Third argument (frame rate) is not numeric:\n%s\n", *(argv + 2));
        while (*fr) {
            if (!is_num(*fr++)) {
                fprintf(stderr, "%c", '^');
                continue;
            }
            fprintf(stderr, "~");
        }
        return -1;
    }
    size_t rate = 0;
    while (*fr) {
        rate *= 10;
        rate += *fr++ - 48;
    }
    const char *cmpr = *(argv + 3);
    if (strcmp_c(*(argv + 3), "4:4:4") != 0 && strcmp_c(*(argv + 3), "4:2:2") != 0 && strcmp_c(*(argv + 3), "4:2:0") != 0) {
        fprintf(stderr, "Invalid YUV format (only 4:4:4, 4:2:2 and 4:2:0 are possible).\n");
        return -1;
    }
#ifdef _WIN32
    DWORD fileAttr = GetFileAttributesA(*(argv + 1));
    if (fileAttr == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "Invalid directory provided.\n");
        return -1;
    }
    if ((fileAttr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) {
        fprintf(stderr, "Second argument provided is not a directory.\n");
        return -1;
    }
#else
    struct stat buff{};
    if (stat(*(argv + 1), &buff) == -1) {
        fprintf(stderr, "Invalid directory provided.\n");
        return -1;
    }
    if (!S_ISDIR(buff.st_mode)) {
        fprintf(stderr, "Second argument provided is not a directory.\n");
        return -1;
    }
#endif
    char *path;
    char *bmp_path = malloc(sizeof(char)*(strlen_c(*(argv + 1)) + 2));
    strcpy_c(bmp_path, *(argv + 1));
    strcat_c(bmp_path, "\\");
    size_t len = strlen_c(bmp_path);
    size_t size = 0; // have to leave space for NULL at the end of the array
#ifdef _WIN32
    path = malloc(sizeof(char)*(strlen_c(bmp_path) + 6));
    strcpy_c(path, *(argv + 1));
    strcat_c(path, "\\*.bmp");
    WIN32_FIND_DATAA find = {};
    HANDLE first = FindFirstFileA(path, &find);
    free(path);
    if (first == INVALID_HANDLE_VALUE) {
        free(bmp_path);
        fprintf(stderr, "No .bmp files found in specified directory.\n");
        return -1;
    }
#else
    struct dirent *entry;
    DIR *dir = opendir(bmp_path);
#endif
    const char **array = malloc(sizeof(char*)*MIN_ARR_SIZE);
    const char **ptr = array;
    char *str;
#ifdef _WIN32
    do {
        str = malloc(sizeof(char)*(len + MAX_PATH)); // MAX_PATH includes null-terminator
        strcpy_c(str, bmp_path);
        strcat_c(str, find.cFileName);
        *ptr++ = str;
        ++size;
        if (size >= MIN_ARR_SIZE && (size & (size - 1)) == 0) {
            array = realloc(array, size*2*sizeof(char*));
            ptr = array + size;
        }
    } while(FindNextFileA(first, &find) != 0);
#else
    while((entry = readdir(dir)) != NULL) {
        if (endswith(entry->d_name, ".bmp")) {
            str = malloc(sizeof(char)*(len + 256));
            strcpy_c(str, bmp_path);
            strcat_c(str, entry->d_name);
            *ptr++ = str;
            ++size;
            if (size >= MIN_ARR_SIZE && (size & (size - 1)) == 0) {
                array = realloc(array, size*2*sizeof(char*));
            }
        }
    }
#endif
    *ptr = NULL;
    array = realloc(array, (size + 1)*(sizeof(char*)));
    char *vid_path = malloc(sizeof(char)*(len + 39)); // path for generated .y4m file
    strcpy_c(vid_path, bmp_path);
    free(bmp_path);
    strcat_c(vid_path, "Y4M_Video_");
    const char *curr_time = get_und_time();
    strcat_c(vid_path, curr_time);
    strcat_c(vid_path, ".y4m");
    alphabetical_sort(array);
    const char **arr = array;
    char clr_space[5];
    clr_space[0] = 'C';
    clr_space[1] = *cmpr++;
    clr_space[2] = *++cmpr;
    clr_space[3] = *(cmpr += 2);
    clr_space[4] = 0;
    FILE *vid;
    FILE *bmp = fopen(*array, "rb");
    if (bmp == NULL) {
        fprintf(stderr, "Error trying to open file: %s\n", *array);
        return -1;
    }
    bmp_header header;
    bmp_info_header info_header;
    fread(&header, sizeof(char), sizeof(header), bmp);
    fread(&info_header, sizeof(char), sizeof(info_header), bmp);
    fclose(bmp);
    char *t = malloc(sizeof(char)*(strlen_c(curr_time) + 13));
    strcpy_c(t, "XCREATED_ON=");
    strcat_c(t, curr_time);
    const char *yuv_h = yuv_header(info_header.bmp_width, info_header.bmp_height, rate, 1, 'p', 1, 1, clr_space, t);
    free(t);
    // if (yuv_h == NULL) {
    //     fprintf(stderr, "Invalid parameters for YUV4MPEG2 file.\n");
    //     return -1;
    // }
    // while (*arr) {
    //     ++arr;
    // }
    free((char *) yuv_h);
    free(vid_path);
    free_array(array);
    printf("At end.\n");
    return 0;
}