//
// created by Gregor Hartl Watters on 13/09/2022
//

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

static inline void delegate_flags(const char *arg, bool *del, bool *tim, bool *sz);
static inline void term_if_zero(size_t val);

int main(int argc, char **argv) {
    time_t beg_time = time(NULL);
    if (argc > 7 || argc < 2) {
        fprintf(stderr, "Invalid number of command-line arguments provided.\n");
        return -1;
    }
    bool delete = false;
    bool timed = false;
    bool give_size = false;
    long long rate = 30;
    const char *cmpr = "4:4:4";
    bool cmpr_set = false;
    if (argc >= 3) {
        const char *fr = *(argv + 2);
        if (!is_numeric(fr)) {
            if (strcmp_c(fr, "4:4:4") != 0 && strcmp_c(fr, "4:2:2") != 0 && strcmp_c(fr, "4:2:0") != 0 &&
                strcmp_c(fr, "4:1:1") != 0) {
                if (strcmp_c(fr, "-delete") != 0 && strcmp_c(fr, "-time") != 0 && strcmp_c(fr, "-size") != 0) {
                    fprintf(stderr, "Third argument is not numeric:\n%s\n", *(argv + 2));
                    while (*fr) {
                        if (!is_num(*fr++)) {
                            fprintf(stderr, "%c", '^');
                            continue;
                        }
                        fprintf(stderr, "~");
                    }
                    fprintf(stderr, "\n... (for frame rate), nor is a valid colour space or flag.");
                    return -1;
                }
                else {
                    delegate_flags(*(argv + 2), &delete, &timed, &give_size);
                }
            }
            else {
                cmpr = *(argv + 2);
                cmpr_set = true;
            }
        }
        else {
            rate = 0;
            while (*fr) {
                rate *= 10;
                rate += *fr++ - 48;
            }
        }
    }
    if (argc >= 4) {
        if (!cmpr_set) {
            if (strcmp_c(*(argv + 3), "4:4:4") != 0 && strcmp_c(*(argv + 3), "4:2:2") != 0 &&
                strcmp_c(*(argv + 3), "4:2:0") != 0 && strcmp_c(*(argv + 3), "4:1:1") != 0) {
                if (strcmp_c(*(argv + 3), "-delete") != 0 && strcmp_c(*(argv + 3), "-time") != 0 &&
                    strcmp_c(*(argv + 3), "-size") != 0) {
                    fprintf(stderr, "Invalid 4th argument: invalid YUV colour space (only 4:4:4, 4:2:2, 4:2:0 and 4:1:1"
                                    " are possible) and invalid flag.\n");
                    return -1;
                }
                else {
                    delegate_flags(*(argv + 3), &delete, &timed, &give_size);
                }
            }
            else {
                cmpr = *(argv + 3);
            }
        }
        else {
            delegate_flags(*(argv + 3), &delete, &timed, &give_size);
        }
    }
    if (argc >= 5) { // repeated flags are simply ignored (no harm done!)
        delegate_flags(*(argv + 4), &delete, &timed, &give_size);
        if (argc >= 6) {
            delegate_flags(*(argv + 5), &delete, &timed, &give_size);
            if (argc == 7) {
                delegate_flags(*(argv + 6), &delete, &timed, &give_size);
            }
        }
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
    struct stat buff = {0};
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
        fprintf(stderr, "Invalid BMP format, bit-depth expected: 24 bpp, depth found = %i bpp\n",
                info_header.pixel_depth);
        return -1;
    }
    fclose(bmp);
    unsigned long width = info_header.bmp_width;
    unsigned long height = info_header.bmp_height;
    unsigned char padding = PAD_24BPP(info_header.bmp_width);
    unsigned char lopped_off_w = 0; // cutting edge off images is required for clr sub-sampling other than C444 if the
    unsigned char lopped_off_h = 0; // width and height of the images are not a multiple of 4
    if (strcmp_c(clr_space, "C422") == 0 || strcmp_c(clr_space, "C420") == 0) {
        if (info_header.bmp_width % 2 != 0) {
            --width;
            lopped_off_w = 1;
        }
        term_if_zero(width);
    }
    if (strcmp_c(clr_space, "C420") == 0) {
        if (info_header.bmp_height % 2 != 0) {
            --height;
            lopped_off_h = 1;
        }
        term_if_zero(height);
    }
    if (strcmp_c(clr_space, "C411") == 0) {
        unsigned char rem = info_header.bmp_width % 4;
        if (rem != 0) {
            if (info_header.bmp_width <= rem) {
                term_if_zero(0);
            }
            width -= (lopped_off_w = rem);
        }
    }
    const char *curr_time = get_und_time();
    char t[sizeof(char)*(UND_TIME_MAX_LEN + 12)];
    strcpy_c(t, "CREATED_ON=");
    strcat_c(t, curr_time);
    const char *yuv_h = yuv_header(width, height, rate, 1, 'p', 1, 1, clr_space, t);
    if (yuv_h == NULL) {
        free(bmp_path);
        free_array(array);
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
        fprintf(stderr, "Error opening video output path: \"%s\"\n", vid_path);
        perror("Error type");
        free_ptrs(2, vid_path, (char *) yuv_h);
        free_array(array);
        return -1;
    }
    free(vid_path);
    fwrite(yuv_h, sizeof(char), strlen_c(yuv_h), vid);
    free((char *) yuv_h);
    const char *frame = "FRAME\n";
    colour col = {0};
    long int start_offset = -((long) (info_header.bmp_width*3 + padding)); // same for all colour sub-sampling cases
    long int repeat_offset;
    colour *colours = malloc(width*height*3); // I have opted for heap alloc. to avoid repeated calls to fread(), the
    if (colours == NULL) { // tests I have run have shown the comp. time to have been reduced by at least 60%
        fprintf(stderr, "Memory allocation error, likely due to overly large BMP file size.\n");
        return -1;
    }
    colour *clr_ptr;
    if (strcmp_c(clr_space, "C444") == 0) { // uncompressed case - BMP pixel array size = FRAME pixel array size
        repeat_offset = -((long) (2*width*3 + padding));
        while (*arr) { // lots of repetition below, but better to avoid function calls
            fputs(frame, vid); // each frame starts with "FRAME\n"
            bmp = fopen(*arr++, "rb");
            if (bmp == NULL) {
                fprintf(stderr, "File \"%s\" could not be opened.\n", *--arr);
                free_array(array);
                return -1;
            }
            fseek(bmp, start_offset, SEEK_END); // seek to end row of pixel array in BMP
            clr_ptr = colours;
            for (size_t i = 0; i < height; ++i) { // read in image in inverse row order
                fread(clr_ptr, sizeof(colour), width, bmp);
                fseek(bmp, repeat_offset, SEEK_CUR); // seek to previous row (y4m videos are inverted compared to BMPs)
                clr_ptr += width;
            }
            fclose(bmp);
            clr_ptr = colours;
            for (size_t i = 0; i < height; ++i) { // write Y plane
                for (size_t j = 0; j < width; ++j) {
                    fputc(get_Y(clr_ptr++), vid);
                }
            }
            clr_ptr = colours;
            for (size_t i = 0; i < height; ++i) { // write Cb plane
                for (size_t j = 0; j < width; ++j) {
                    fputc(get_Cb(clr_ptr++), vid);
                }
            }
            clr_ptr = colours;
            for (size_t i = 0; i < height; ++i) { // write Cr plane
                for (size_t j = 0; j < width; ++j) {
                    fputc(get_Cr(clr_ptr++), vid);
                }
            }
        }
        free(colours);
    }
    else if (strcmp_c(clr_space, "C422") == 0) { // video frame size = (2/3) * BMP pixel array size
        repeat_offset = -((long) ((width + info_header.bmp_width)*3 + padding));
        unsigned int half_width = width/2; // no 0.5 truncation, width is guaranteed to be even by here
        while (*arr) {
            fputs(frame, vid);
            bmp = fopen(*arr++, "rb");
            if (bmp == NULL) {
                fprintf(stderr, "File \"%s\" could not be opened.\n", *--arr);
                free_array(array);
                return -1;
            }
            fseek(bmp, start_offset, SEEK_END);
            clr_ptr = colours;
            for (size_t i = 0; i < height; ++i) {
                fread(clr_ptr, sizeof(colour), width, bmp);
                fseek(bmp, repeat_offset, SEEK_CUR);
                clr_ptr += width;
            }
            clr_ptr = colours;
            for (size_t i = 0; i < height; ++i) { // write full Y plane
                for (size_t j = 0; j < width; ++j) {
                    fputc(get_Y(clr_ptr++), vid);
                }
            }
            clr_ptr = colours;
            for (size_t i = 0; i < height; ++i) { // write 'half' Cb plane, since two pixels share same Cb component
                for (size_t j = 0; j < half_width; ++j) {
                    fputc(get_Cb_avg2(clr_ptr, clr_ptr + 1), vid); // can't use ++ inside func call here - UB
                    clr_ptr += 2;
                }
            }
            clr_ptr = colours;
            for (size_t i = 0; i < height; ++i) { // same as for Cb plane
                for (size_t j = 0; j < half_width; ++j) {
                    fputc(get_Cr_avg2(clr_ptr, clr_ptr + 1), vid);
                    clr_ptr += 2;
                }
            }
            fclose(bmp);
        }
        free(colours);
    }
    else if (strcmp_c(clr_space, "C420") == 0) { // video frame size = (1/2) * BMP pixel array size
        if (height != info_header.bmp_height) {
            start_offset -= (long) (padding + 3*info_header.bmp_width);
        }
        repeat_offset = -((long) ((width + info_header.bmp_width)*3 + padding));
        unsigned int half_width = width/2;
        unsigned int half_height = height/2; // no 0.5 truncation, height is guaranteed to be even by here
        printf("width: %i\n", width);
        printf("height: %i\n", height);
        printf("half_width: %i\n", half_width);
        printf("half height: %i\n", half_height);
        colour *next;
        size_t count = 0;
        while (*arr) {
            fputs(frame, vid);
            bmp = fopen(*arr++, "rb");
            fseek(bmp, start_offset, SEEK_END);
            clr_ptr = colours;
            for (size_t i = 0; i < height; ++i) {
                fread(clr_ptr, sizeof(colour), width, bmp);
                fseek(bmp, repeat_offset, SEEK_CUR);
                clr_ptr += width;
            }
            fclose(bmp);
            clr_ptr = colours;
            for (size_t i = 0; i < height; ++i) { // write full Y plane
                for (size_t j = 0; j < width; ++j) {
                    fputc(get_Y(clr_ptr++), vid);
                    ++count;
                }
            }
            printf("Y count: %zu\n", count);
            count = 0;
            clr_ptr = colours;
            next = colours + width; // pointer to the pixel 'below' the pixel pointed to by clr_ptr
            for (size_t i = 0; i < half_height; ++i) { // write 1/4 Cb plane, since 4 pixels share same Cb component
                for (size_t j = 0; j < half_width; ++j) {
                    fputc(get_Cb_avg4(clr_ptr, clr_ptr + 1, next, next + 1), vid);
                    clr_ptr += 2;
                    next += 2;
                    ++count;
                }
            }
            printf("Cb count: %zu\n", count);
            count = 0;
            clr_ptr = colours;
            next = colours + width;
            for (size_t i = 0; i < half_height; ++i) { // same as for Cb plane
                for (size_t j = 0; j < half_width; ++j) {
                    fputc(get_Cr_avg4(clr_ptr, clr_ptr + 1, next, next + 1), vid);
                    clr_ptr += 2;
                    next += 2;
                    ++count;
                }
            }
            printf("Cr count: %zu\n", count);
        }
    }
    else { // C411 - video frame size = (1/2) * BMP pixel array size

    }
    size_t y4m_file_size = ftell(vid);
    fclose(vid);
    arr = array;
    if (delete) { // more efficient to create separate loop, rather than re-evaluating condition within above loop
        while (*arr) {
            if (remove(*arr++) != 0) {
                free_array(array);
                fprintf(stderr, "Error occurred when trying to delete file \"%s\".\n", *--arr);
                return -1;
            }
        }
    }
    free_array(array);
    if (give_size) {
        printf("File size: %zu bytes\n", y4m_file_size);
    }
    if (timed) {
        time_t total_time = time(NULL) - beg_time;
        printf(total_time == 1 ? "Elapsed time: %zu second\n" : "Elapsed time: %zu seconds\n", (size_t) total_time);
    }
    return 0;
}

static inline void delegate_flags(const char *arg, bool *del, bool *tim, bool *sz) {
    if (strcmp_c(arg, "-delete") == 0) {
        *del = true;
    }
    else if (strcmp_c(arg, "-time") == 0) {
        *tim = true;
    }
    else if (strcmp_c(arg, "-size") == 0) {
        *sz = true;
    }
    else {
        fprintf(stderr, "Invalid flag: \"%s\"\nPossible flags are:\n\"-delete\" "
                        "(to delete all bmp files used to generate the video),\n\"-time\" "
                        "(to show the length of time taken to generate the video) and\n\"-size\" "
                        "(to give the generated video's file size).\n", arg);
        exit(-1);
    }
}

static inline void term_if_zero(size_t val) {
    if (val == 0) {
        fprintf(stderr, "Resultant video would have zero size. Provide larger bitmaps.\n");
        exit(-1);
    }
}
