//
// Created by mario on 18/09/2022.
//

#include "overhead.h"

int main(int argc, char **argv) {
    if (argc != 3) {
        fprintf(stderr, "3 command-line arguments expected (including executable name).\n");
        return -1;
    }
    const char *cutoff_str = *(argv + 2);
    if (!is_numeric(cutoff_str)) {
        fprintf(stderr, "Error: 3rd argument (grayscale cut-off) is not numeric.\n");
        return -1;
    }
    const char *path = *(argv + 1);
    if (!endswith(path, ".bmp")) {
        fprintf(stderr, "Error: file provided is not a .bmp file.\n");
        return -1;
    }
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        fprintf(stderr, "Error opening BMP file.\n");
        return -1;
    }
    unsigned char cutoff = 0;
    while (*cutoff_str) {
        cutoff *= 10;
        cutoff += *cutoff_str++ - 48;
    }
    bmp_header hdr = {0};
    if (fread(&hdr, sizeof(bmp_header), 1, fp) != 1) {
        fprintf(stderr, "Error reading from .bmp file.\n");
        return -1;
    }
    if (hdr.px_arr_offset != PIX_OFFSET) {
        fprintf(stderr, "Invalid BMP file format. Only BMPs containing the BITMAPINFOHEADER are supported.\n");
        return -1;
    }
    bmp_info_header info_hdr = {0};
    if (fread(&info_hdr, sizeof(bmp_info_header), 1, fp) != 1) {
        fprintf(stderr, "Error reading from .bmp file.\n");
        return -1;
    }
    if (info_hdr.pixel_depth != 24) {
        fprintf(stderr, "Invalid BMP file format. Only 24 bpp BMPs are supported.\n");
        return -1;
    }
    unsigned char padding = PAD_24BPP(info_hdr.bmp_width);
    colour *colours = malloc(sizeof(colour)*info_hdr.bmp_width*info_hdr.bmp_height);
    colour *clr_ptr = colours;
    unsigned int i = 0;
    unsigned int j = 0;
    for (; i < info_hdr.bmp_height; ++i) {
        for (; j < info_hdr.bmp_width; ++j) {
            fread(clr_ptr, sizeof(colour), info_hdr.bmp_width, fp);
        }
        fseek(fp, padding, SEEK_CUR);
    }
    fclose(fp);
    size_t path_len = strlen_c(path);
    char *out_str = malloc(path_len + 7 + UND_TIME_MAX_LEN);
    strcpy_c(out_str, path);
    *(out_str + path_len - 4) = 0;
    strcat_c(out_str, "_MONO_");
    strcat_c(out_str, get_und_time());
    strcat_c(out_str, ".bmp");
    fp = fopen(out_str, "wb");
    fclose(fp);
    free(out_str);
    free(colours);
    return 0;
}
