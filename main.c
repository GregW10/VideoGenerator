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

#define PAD_24BPP(w) ((w)*3 % 4 == 0 ? 0 : 4 - ((3*(w)) % 4))

#define MIN_ARR_SIZE 128 // starting size of array to store bmp paths
#define UND_TIME_MAX_LEN 25 // exact length of str. returned by get_und_time() func. (not including '\0')
#define PIX_OFFSET 54 // required starting address of the pixel offset in the BMP images

int main(int argc, char **argv) {
    if (argc > 5) {
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
    long long rate = 0;
    while (*fr) {
        rate *= 10;
        rate += *fr++ - 48;
    }
    const char *cmpr = *(argv + 3);
    if (strcmp_c(*(argv+3), "4:4:4") != 0 && strcmp_c(*(argv+3), "4:2:2") != 0 && strcmp_c(*(argv+3), "4:2:0") != 0) {
        fprintf(stderr, "Invalid YUV format (only 4:4:4, 4:2:2 and 4:2:0 are possible).\n");
        return -1;
    }
    bool delete = false;
    if (argc == 5) {
        if (strcmp_c(*(argv + 4), "delete") != 0) {
            fprintf(stderr, "If provided, 5th argument must be \"delete\". Argument provided was: \"%s\"\n", *(argv+4));
            return -1;
        }
        delete = true;
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
    WIN32_FIND_DATAA find = {0};
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
    alphabetical_sort(array); // in case paths found are not in alph. order, this sorts them
    const char **arr = array;
    char clr_space[5];
    clr_space[0] = 'C';
    clr_space[1] = *cmpr++;
    clr_space[2] = *++cmpr;
    clr_space[3] = *(cmpr += 2);
    clr_space[4] = 0;
    FILE *bmp = fopen(*array, "rb");
    if (bmp == NULL) {
        fprintf(stderr, "Error trying to open file: %s\n", *array);
        free_array(array);
        free(bmp_path);
        return -1;
    }
    bmp_header header;
    bmp_info_header info_header;
    fread(&header, sizeof(char), sizeof(header), bmp);
    if (header.px_arr_offset != PIX_OFFSET) {
        free_array(array);
        free(bmp_path);
        fprintf(stderr, "Invalid BMP format, offset expected = 54 bytes, offset found = %i\n", header.px_arr_offset);
        return -1;
    }
    fread(&info_header, sizeof(char), sizeof(info_header), bmp);
    if (info_header.pixel_depth != 24 && info_header.pixel_depth != 32) {
        free_array(array);
        free(bmp_path);
        fprintf(stderr, "Invalid BMP format, bit-depth expected: 24 bpp or 32 bpp, depth found = %i bpp\n",
                info_header.pixel_depth);
        return -1;
    }
    fclose(bmp);
    const char *curr_time = get_und_time();
    char *t = malloc(sizeof(char)*(UND_TIME_MAX_LEN + 12));
    strcpy_c(t, "CREATED_ON=");
    strcat_c(t, curr_time);
    const char *yuv_h = yuv_header(info_header.bmp_width, info_header.bmp_height, rate, 1, 'p', 1, 1, clr_space, t);
    free(t);
    if (yuv_h == NULL) {
        fprintf(stderr, "Invalid parameters for YUV4MPEG2 file.\n");
        return -1;
    }
    char *vid_path = malloc(sizeof(char)*(len + UND_TIME_MAX_LEN + 15)); // path for generated .y4m file
    strcpy_c(vid_path, bmp_path);
    free(bmp_path);
    strcat_c(vid_path, "Y4M_Video_");
    strcat_c(vid_path, curr_time);
    strcat_c(vid_path, ".y4m");
    FILE *vid = fopen(vid_path, "wb");
    if (vid == NULL) {
        fprintf(stderr, "Error opening video output path: %s\n", vid_path);
        free_ptrs(2, vid_path, (char *) yuv_h);
        free_array(array);
        return -1;
    }
    free(vid_path);
    fwrite(yuv_h, sizeof(char), strlen_c(yuv_h), vid);
    const char *frame = "FRAME\n";
    colour col = {0};
    unsigned char padding = PAD_24BPP(info_header.bmp_width);
    unsigned int max_h = info_header.bmp_height - 1;
    if (strcmp_c(clr_space, "C444") == 0) { // uncompressed case - BMP pixel array size = FRAME pixel array size
        while (*arr) {
            fputs(frame, vid); // each frame starts with "FRAME\n"
            bmp = fopen(*arr++, "rb");
            for (size_t i = 0; i < info_header.bmp_height; ++i) { // loop for writing all Y component bytes to video
                for (size_t j = 0; j < info_header.bmp_width; ++j) {
                    fread(&col, sizeof(char), 3, bmp); // get RGB from bitmap and...
                    fputc(get_Y(&col), vid); // convert to Y and write to video file
                }
                fseek(bmp, 3 + padding, SEEK_CUR); // seek to next row
            }
            fseek(bmp, 0, SEEK_SET); // seek to beginning of bmp file (still missing Cb and Cr planes)
            for (size_t i = 0; i < info_header.bmp_height; ++i) { // write Cb plane
                for (size_t j = 0; j < info_header.bmp_width; ++j) {
                    fread(&col, sizeof(char), 3, bmp);
                    fputc(get_Cb(&col), vid);
                }
                fseek(bmp, 3 + padding, SEEK_CUR);
            }
            fseek(bmp, 0, SEEK_SET); // still missing Cr plane
            for (size_t i = 0; i < info_header.bmp_height; ++i) { // write Cr plane
                for (size_t j = 0; j < info_header.bmp_width; ++j) {
                    fread(&col, sizeof(char), 3, bmp);
                    fputc(get_Cr(&col), vid);
                }
                fseek(bmp, 3 + padding, SEEK_CUR);
            }
            fclose(bmp);
        }
    }
    else if (strcmp_c(clr_space, "C422") == 0) {

    }
    else { // C420

    }
    fclose(vid);
    arr = array;
    if (delete) { // more efficient to create separate loop, rather than re-evaluating condition within above loop
        while (*arr) {
            if (remove(*arr++) != 0) {
                free((char *) yuv_h);
                free_array(array);
                fprintf(stderr, "Error occurred when trying to delete file \"%s\".\n", *--arr);
                return -1;
            }
        }
    }
    free((char *) yuv_h);
    free_array(array);
    printf("At end.\n");
    return 0;
}
