//
// created by Gregor Hartl Watters on 13/09/2022
//

#include "overhead.h"

#define MIN_ARR_SIZE 128 // starting size of array to store bmp paths

char *bmp_path = NULL; // global pointers, so they can be easily freed with a func. passed to atexit()
char *path = NULL;
const char **array = NULL;
char *vid_path = NULL;
const char *yuv_h = NULL;
colour *colours = NULL;
unsigned char *final_clrs = NULL;

void clean(void) {
    if (array)
        free_array(array);
    free_ptrs(6, bmp_path, path, vid_path, yuv_h, colours, final_clrs);
}

_Noreturn void handler(int signal) {
    fprintf(stderr, RED_TXT(BOLD_TXT("Program terminated due to abnormal circumstances.\n"))
                    CYAN_TXT("Please use") BOLD_TXT(YELLOW_TXT(" \"--help\" "))
                    CYAN_TXT("option for program usage and for troubleshooting.\n"));
    exit(1);
}

static inline void term_if_zero(size_t val);

int main(int argc, char **argv) {
    atexit(clean); // register clean func. with atexit() - ensures pointers are freed in case of premature termination
    signal(SIGABRT, handler);
    time_t beg_time = time(NULL);
    bool delete = false; // whether to delete .bmp images as they are appended to the video
    bool timed = false; // whether to display the time taken for the video generation
    bool give_size = false; // whether to display the total file size of the video generated
    bool prog = false; // whether to show the progress of the video generation
    char *vpath_given = NULL; // path to the .y4m as given by the user - remains NULL if none given
    const char *folder_path = NULL; // path to directory containing .bmp files - if none given, cwd is used
    long long rate_num = 30; // default frame rate numerator
    long long rate_denom = 1; // default frame rate denominator
    const char *cmpr = "420"; // default colour sub-sampling
    process_argv(argc, argv, &delete, &timed, &give_size, &prog, &vpath_given, &folder_path, &rate_num,
                 &rate_denom, &cmpr);
#ifdef _WIN32
    DWORD fileAttr = GetFileAttributesA(folder_path);
    if (fileAttr == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "Invalid directory provided.\n");
        abort();
    }
    if ((fileAttr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) {
        fprintf(stderr, "Argument provided is not a directory.\n");
        abort();
    }
#else
    struct stat buff = {0};
    if (stat(folder_path, &buff) == -1) {
        fprintf(stderr, "Invalid directory provided.\n");
        abort();
    }
    if (!S_ISDIR(buff.st_mode)) {
        fprintf(stderr, "Argument provided is not a directory.\n");
        abort();
    }
#endif
    bmp_path = malloc(sizeof(char)*(strlen_c(folder_path) + 2));
    strcpy_c(bmp_path, folder_path);
    size_t len = strlen_c(bmp_path);
    if (*(bmp_path + len - 1) != file_sep()) {
        *(bmp_path + len++) = file_sep();
        *(bmp_path + len) = 0;
    }
    size_t size = 0; // have to leave space for NULL at the end of the array
#ifdef _WIN32
    path = malloc(sizeof(char)*(len + 6));
    strcpy_c(path, folder_path);
    strcat_c(path, "*.bmp");
    WIN32_FIND_DATAA find = {0};
    HANDLE first = FindFirstFileA(path, &find);
    free(path);
    path = NULL;
    if (first == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "No .bmp files found in specified directory.\n");
        abort();
    }
#else
    struct dirent *entry;
    DIR *dir = opendir(bmp_path);
    if (dir == NULL) {
        fprintf(stderr, "Could not open directory.\n");
        perror("Error");
        abort();
    }
#endif
    array = malloc(sizeof(char*)*MIN_ARR_SIZE);
    const char **ptr = array;
    char *str;
#ifdef _WIN32
    do {
        str = malloc(sizeof(char)*(MAX_PATH - len)); // MAX_PATH includes null-terminator
        strcpy_c(str, bmp_path);
        strcat_c(str, find.cFileName);
        str = realloc(str, strlen_c(str) + 1);
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
            str = malloc(sizeof(char)*(4096 - len));
            strcpy_c(str, bmp_path);
            // strcat_c(str, "/");
            strcat_c(str, entry->d_name);
            str = realloc(str, strlen_c(str) + 1);
            *ptr++ = str;
            ++size;
            if (size >= MIN_ARR_SIZE && (size & (size - 1)) == 0) {
                array = realloc(array, size*2*sizeof(char*));
                ptr = array + size;
            }
        }
    }
#endif
    // *ptr = NULL;
    if (!size) {
        fprintf(stderr, BOLD_TXT(RED_TXT("Error: ")) CYAN_TXT(UNDERLINED_TXT("No BMP files found in directory:"))
        GREEN_TXT(" \"%s\"\n"), bmp_path);
        abort();
    }
    array = realloc(array, (size + 1)*(sizeof(char*)));
    *(array + size) = NULL;
    alphabetical_sort(array); // in case paths found are not in alphabetical order, this sorts them
    const char **arr = array;
    char clr_space[5];
    clr_space[0] = 'C';
    clr_space[1] = *cmpr++;
    clr_space[2] = *cmpr++;
    clr_space[3] = *cmpr;
    clr_space[4] = 0;
    FILE *bmp = fopen(*array, "rb");
    if (bmp == NULL) {
        fprintf(stderr, "Error trying to open file: %s\n", *array);
        abort();
    }
    bmp_header header;
    bmp_info_header info_header;
    fread(&header, sizeof(char), sizeof(header), bmp);
    if (header.px_arr_offset != PIX_OFFSET) {
        fprintf(stderr, "Invalid BMP format, offset expected = 54 bytes, offset found = %i\n", header.px_arr_offset);
        abort();
    }
    fread(&info_header, sizeof(char), sizeof(info_header), bmp);
    if (info_header.pixel_depth != 24 && info_header.pixel_depth != 32) {
        fprintf(stderr, "Invalid BMP format, bit-depth expected: 24 bpp, depth found = %i bpp\n",
                info_header.pixel_depth);
        abort();
    }
    fclose(bmp);
    unsigned long width = info_header.bmp_width;
    unsigned long height = info_header.bmp_height;
    unsigned char padding = PAD_24BPP(info_header.bmp_width);
    if (strcmp_c(clr_space, "C422") == 0 || strcmp_c(clr_space, "C420") == 0) {
        if (info_header.bmp_width % 2 != 0) {
            --width;
        }
        term_if_zero(width);
    }
    if (strcmp_c(clr_space, "C420") == 0 || strcmp_c(clr_space, "C410")) {
        if (info_header.bmp_height % 2 != 0) {
            --height;
        }
        term_if_zero(height);
    }
    if (strcmp_c(clr_space, "C411") == 0 || strcmp_c(clr_space, "C410")) {
        unsigned char rem = info_header.bmp_width % 4;
        if (rem != 0) {
            if (info_header.bmp_width <= rem) {
                term_if_zero(0);
            }
            width -= rem;
        }
    }
    const char *curr_time = get_und_time();
    char t[sizeof(char)*(UND_TIME_MAX_LEN + 12)];
    strcpy_c(t, "CREATED_ON=");
    strcat_c(t, curr_time);
    yuv_h = yuv_header(width, height, rate_num, rate_denom, 'p', 1, 1, clr_space, t);
    if (yuv_h == NULL) {
        fprintf(stderr, "Invalid parameters for YUV4MPEG2 file.\n");
        abort();
    }
    FILE *vid;
    if (vpath_given) {
        vid_path = vpath_given;
        vid = fopen(vpath_given, "wb");
        if (vid == NULL) {
            fprintf(stderr, "Error opening video output path: \"%s\"\n", vid_path);
            perror("Error type");
            abort();
        }
    }
    else {
        vid_path = malloc(sizeof(char)*(len + UND_TIME_MAX_LEN + 15)); // path for generated .y4m file
        strcpy_c(vid_path, bmp_path);
        free(bmp_path);
        bmp_path = NULL;
#ifndef _WIN32
        chrcat_c(vid_path, '/');
#endif
        strcat_c(vid_path, "Y4M_Video_");
        strcat_c(vid_path, curr_time);
        strcat_c(vid_path, ".y4m");
        vid = fopen(vid_path, "wb");
        if (vid == NULL) {
            fprintf(stderr, "Error opening video output path: \"%s\"\n", vid_path);
            perror("Error type");
            abort();
        }
        free(vid_path);
    }
    vid_path = NULL;
    fwrite(yuv_h, sizeof(char), strlen_c(yuv_h), vid);
    free((char *) yuv_h);
    yuv_h = NULL;
    long start_offset = -((long) (info_header.bmp_width*3 + padding)); // same for all colour sub-sampling cases
    long repeat_offset = -((long) ((width + info_header.bmp_width)*3 + padding));
    colours = malloc(width*height*sizeof(colour)); // I have opted for heap alloc. to avoid repeated calls to
    if (!colours) { // fread(), the tests I have run have shown the comp. time to have been reduced by at least 60%
        fprintf(stderr, "Memory allocation error, likely due to overly large BMP file size.\n");
        abort();
    }
    unsigned char *fclr_ptr;
    colour *clr_ptr;
    unsigned int total_reps = width*height; // guaranteed to never be larger than 4294967295
    unsigned int i; // loop counter
    size_t frame_size; // size of a single frame in the .y4m video
    unsigned int frames = 0;
    if (strcmp_c(clr_space, "C444") == 0) { // uncompressed case - BMP pixel array size = FRAME pixel array size
        frame_size = total_reps*sizeof(colour);
        final_clrs = malloc(frame_size);
        for (;*arr; ++arr) { // lots of repetition below, but better to avoid function calls
            start_frame(vid); // each frame starts with "FRAME\n"
            bmp = fopen(*arr, "rb");
            if (!bmp) {
                fprintf(stderr, "File \"%s\" could not be opened.\n", *arr);
                return 1;
            }
            check_dim(bmp, *arr);
            fseek(bmp, start_offset, SEEK_END); // seek to end row of pixel array in BMP
            clr_ptr = colours;
            for (i = 0; i < height; ++i) { // read in image in inverse row order
                fread(clr_ptr, sizeof(colour), width, bmp);
                fseek(bmp, repeat_offset, SEEK_CUR); // seek to previous row (y4m videos are inverted compared to BMPs)
                clr_ptr += width;
            }
            fclose(bmp);
            if (delete) // not great to re-evaluate this within loop, but leads to cleaner code, and <0.0001% extra time
                if (remove(*arr)) {
                    fprintf(stderr, "Error occurred when trying to delete file \"%s\".\n", *arr);
                    abort();
                }
            clr_ptr = colours;
            fclr_ptr = final_clrs;
            output_444(clr_ptr, fclr_ptr, total_reps);
            fwrite(fclr_ptr, sizeof(unsigned char), frame_size, vid);
            if (prog) {
                printf(GREEN_TXT(UNDERLINED_TXT("Frames completed:")) MAGENTA_TXT(BOLD_TXT(" %i"))
                       YELLOW_TXT(" / ") BLUE_TXT(BOLD_TXT("%zu\r")), ++frames, size);
                fflush(stdout);
            }
        }
    }
    else if (strcmp_c(clr_space, "C422") == 0) { // video frame size = (2/3) * BMP pixel array size
        frame_size = total_reps*2*sizeof(unsigned char);
        final_clrs = malloc(frame_size);
        for (; *arr; ++arr) {
            start_frame(vid);
            bmp = fopen(*arr, "rb");
            if (!bmp) {
                fprintf(stderr, "File \"%s\" could not be opened.\n", *arr);
                abort();
            }
            check_dim(bmp, *arr);
            fseek(bmp, start_offset, SEEK_END);
            clr_ptr = colours;
            for (i = 0; i < height; ++i) {
                fread(clr_ptr, sizeof(colour), width, bmp);
                fseek(bmp, repeat_offset, SEEK_CUR);
                clr_ptr += width;
            }
            fclose(bmp);
            if (delete)
                if (remove(*arr)) {
                    fprintf(stderr, "Error occurred when trying to delete file \"%s\".\n", *arr);
                    return 1;
                }
            clr_ptr = colours;
            fclr_ptr = final_clrs;
            output_422(clr_ptr, fclr_ptr, total_reps);
            fwrite(fclr_ptr, sizeof(unsigned char), frame_size, vid);
            if (prog) {
                printf(GREEN_TXT(UNDERLINED_TXT("Frames completed:")) MAGENTA_TXT(BOLD_TXT(" %i"))
                       YELLOW_TXT(" / ") BLUE_TXT(BOLD_TXT("%zu\r")), ++frames, size);
                fflush(stdout);
            }
        }
    } // 4:2:0 sub-sampling is, in my opinion, the best choice, as quality is decent, and file size is cut in half
    else if (strcmp_c(clr_space, "C420") == 0) { // video frame size = (1/2) * BMP pixel array size
        if (height != info_header.bmp_height) {
            start_offset -= (long) (padding + 3*info_header.bmp_width);
        }
        frame_size = (total_reps*3)/2;
        fclr_ptr = malloc(frame_size);
        for (; *arr; ++arr) {
            start_frame(vid);
            bmp = fopen(*arr, "rb");
            if (!bmp) {
                fprintf(stderr, "File \"%s\" could not be opened.\n", *arr);
                abort();
            }
            check_dim(bmp, *arr);
            fseek(bmp, start_offset, SEEK_END);
            clr_ptr = colours;
            for (i = 0; i < height; ++i) {
                fread(clr_ptr, sizeof(colour), width, bmp);
                fseek(bmp, repeat_offset, SEEK_CUR);
                clr_ptr += width;
            }
            fclose(bmp);
            if (delete)
                if (remove(*arr)) {
                    fprintf(stderr, "Error occurred when trying to delete file \"%s\".\n", *arr);
                    abort();
                }
            clr_ptr = colours;
            output_420(clr_ptr, fclr_ptr, width, height);
            fwrite(fclr_ptr, sizeof(unsigned char), frame_size, vid);
            if (prog) {
                printf(GREEN_TXT(UNDERLINED_TXT("Frames completed:")) MAGENTA_TXT(BOLD_TXT(" %i"))
                       YELLOW_TXT(" / ") BLUE_TXT(BOLD_TXT("%zu\r")), ++frames, size);
                fflush(stdout);
            }
        }
    }
    else if (strcmp_c(clr_space, "C411") == 0) { // C411 - video frame size = (1/2) * BMP pixel array size
        frame_size = (total_reps*3)/2;
        fclr_ptr = malloc(frame_size);
        for (; *arr; ++arr) {
            start_frame(vid);
            bmp = fopen(*arr, "rb");
            if (!bmp) {
                fprintf(stderr, "File \"%s\" could not be opened.\n", *arr);
                abort();
            }
            check_dim(bmp, *arr);
            fseek(bmp, start_offset, SEEK_END);
            clr_ptr = colours;
            for (i = 0; i < height; ++i) {
                fread(clr_ptr, sizeof(colour), width, bmp);
                fseek(bmp, repeat_offset, SEEK_CUR);
                clr_ptr += width;
            }
            fclose(bmp);
            if (delete)
                if (remove(*arr)) {
                    fprintf(stderr, "Error occurred when trying to delete file \"%s\".\n", *arr);
                    abort();
                }
            clr_ptr = colours;
            output_411(clr_ptr, fclr_ptr, total_reps);
            fwrite(fclr_ptr, sizeof(unsigned char), frame_size, vid);
            if (prog) {
                printf(GREEN_TXT(UNDERLINED_TXT("Frames completed:")) MAGENTA_TXT(BOLD_TXT(" %i"))
                       YELLOW_TXT(" / ") BLUE_TXT(BOLD_TXT("%zu\r")), ++frames, size);
                fflush(stdout);
            }
        }
    }
    /* warning: to the best of my knowledge, 4:1:0 subsampling is not supported by any media player, not even VLC, and
     * does not appear to be supported be a supported format by ffmpeg either */
    else { // C410 - video frame size = (5/12) * BMP pixel array size
        printf(MAGENTA_TXT(BOLD_TXT("Warning:"))
               YELLOW_TXT(" 4:1:0 colour subsampling is a mostly unsupported format: consider using 4:2:0 instead.\n"));
        if (height != info_header.bmp_height) {
            start_offset -= (long) (padding + 3*info_header.bmp_width);
        }
        frame_size = (5*total_reps)/4;
        fclr_ptr = malloc(frame_size);
        for (; *arr; ++arr) {
            start_frame(vid);
            bmp = fopen(*arr, "rb");
            if (!bmp) {
                fprintf(stderr, "File \"%s\" could not be opened.\n", *arr);
                abort();
            }
            check_dim(bmp, *arr);
            fseek(bmp, start_offset, SEEK_END);
            clr_ptr = colours;
            for (i = 0; i < height; ++i) {
                fread(clr_ptr, sizeof(colour), width, bmp);
                fseek(bmp, repeat_offset, SEEK_CUR);
                clr_ptr += width;
            }
            fclose(bmp);
            if (delete)
                if (remove(*arr)) {
                    fprintf(stderr, "Error occurred when trying to delete file \"%s\".\n", *arr);
                    return 1;
                }
            clr_ptr = colours;
            output_410(clr_ptr, fclr_ptr, width, height);
            fwrite(fclr_ptr, sizeof(unsigned char), frame_size, vid);
            if (prog) {
                printf(GREEN_TXT(UNDERLINED_TXT("Frames completed:")) MAGENTA_TXT(BOLD_TXT(" %i"))
                       YELLOW_TXT(" / ") BLUE_TXT(BOLD_TXT("%zu\r")), ++frames, size);
                fflush(stdout);
            }
        }
    }
    free(colours);
    colours = NULL;
    free(final_clrs);
    final_clrs = NULL;
    size_t y4m_file_size = ftell(vid); // will be very big!!!
    fclose(vid);
    if (prog)
        putchar('\n');
    free_array(array);
    array = NULL;
    if (give_size) {
        printf("File size: %zu bytes\n", y4m_file_size);
    }
    if (timed) {
        time_t total_time = time(NULL) - beg_time;
        printf(total_time == 1 ? "Elapsed time: %zu second\n" : "Elapsed time: %zu seconds\n", (size_t) total_time);
    }
    return 0;
}

static inline void term_if_zero(size_t val) {
    if (val == 0) {
        fprintf(stderr, "Resultant video would have zero size. Provide larger bitmaps.\n");
        abort();
    }
}
