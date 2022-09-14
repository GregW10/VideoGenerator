//
// created by Gregor Hartl Watters on 13/09/2022
//

#include <stdio.h>
#include <stdlib.h>
#include "overhead.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

int main(int argc, char **argv) {

    const char *test = "HELLOaz.";
    const char *shit = "hElloAZ.";

    printf("comp: %d\n", alfcmp_c(test, shit));

    const char *array[] = {"bMp003", "002", "Zack", "Bro", "bmP000", "Anna", "Harry", "John", "Alphie", "BMP002", "bmp001", NULL};
    alphabetical_sort(array);
    printf("Printing: \n");
    print_array(array);

    if (argc != 3) {
        fprintf(stderr, "Invalid number of command-line arguments provided.\n");
        return -1;
    }

    if (strcmp_c(*(argv + 2), "4:4:4") != 0 && strcmp_c(*(argv + 2), "4:2:2") != 0 && strcmp_c(*(argv + 2), "4:2:0") != 0) {
        fprintf(stderr, "Invalid YUV format (only 4:4:4, 4:2:2 and 4:2:0 are possible).\n");
        return -1;
    }

    char *path;
    char *bmp_path;
#ifdef _WIN32
    path = malloc(sizeof(char)*(strlen_c(*(argv + 1)) + 7));
    strcpy_c(path, *(argv + 1));
    strcat_c(path, "\\*.bmp");
    bmp_path = malloc(sizeof(char)*(strlen_c(path) + MAX_PATH)); // MAX_PATH includes null-terminator
    strcpy_c(bmp_path, path);
    DWORD fileAttr = GetFileAttributesA(*(argv + 1));
    if (fileAttr == INVALID_FILE_ATTRIBUTES) {
        fprintf(stderr, "Invalid directory provided.\n");
        return -1;
    }
    if ((fileAttr & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY) {
        fprintf(stderr, "Second argument provided is not a directory.\n");
        return -1;
    }
    WIN32_FIND_DATAA find = {};
    HANDLE first = FindFirstFileA(path, &find);
    if (first == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "No .bmp files found in specified directory.\n");
        return -1;
    }
    do {
        printf("Filename: %s\n", find.cFileName);
    } while(FindNextFileA(first, &find) != 0);
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

    return 0;
}