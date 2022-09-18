//
// Created by mario on 18/09/2022.
//

#include "overhead.h"

int main(int argc, char **argv) {
    if (argc < 3 || argc > 5) {
        fprintf(stderr, "Between 3-5 command-line arguments expected:\n"
                        "1: Executable name\n"
                        "2: BMP input file (mandatory)\n"
                        "3: Grayscale colour cutoff point (0-255) (mandatory)\n"
                        "4: Darker colour in RGB format (unsigned integer, 0 - 16777255) (optional).\n"
                        "5: Brighter colour in RGB format (unsigned integer, 0 - 16777255) (optional).\n");
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
    unsigned int dark_clr = 0; // black
    unsigned int bright_clr = (255 << 24) + (255 << 16) + (255 << 8); // white but including the 'reserved' zero field
    if (argc >= 4) {
        const char *dark_str = *(argv + 3);
        while (*dark_str) {
            dark_clr *= 10;
            dark_clr += *dark_str++ - 48;
        }
        dark_clr &= 0x00ffffff; // these two lines are for converting the RGB value into a BGR0 value
        dark_clr = ((dark_clr & 0x000000ff) << 24) + ((dark_clr & 0x0000ff00) << 16) + ((dark_clr & 0x00ff0000) << 8);
        if (argc == 5) {
            const char *bright_str = *(argv + 4);
            bright_clr = 0;
            while (*bright_str) {
                bright_clr *= 10;
                bright_clr += *bright_str++ - 48;
            }
            printf("Bright colour: %u\n", bright_clr);
            bright_clr &= 0x00ffffff;
            bright_clr = ((bright_clr & 0x000000ff) << 24) + ((bright_clr & 0x0000ff00) << 16) + ((bright_clr & 0x00ff0000) << 8);
        }
    }
    printf("DarK: %u, bright: %u\n", dark_clr, bright_clr);
    printf("max RGB: %u\n", 0x00ffffff);
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
    if (!fp) {
        fprintf(stderr, "Error opening output BMP file \"%s\".\n", out_str);
        free_ptrs(2, out_str, colours);
        return -1;
    }
    unsigned int bytes_w = info_hdr.bmp_width % 8 == 0 ? info_hdr.bmp_width/8 : info_hdr.bmp_width/8 + 1;
    info_hdr.pixel_depth = 1; // 1 bpp
    info_hdr.image_size = (bytes_w + (bytes_w % 4 == 0 ? 0 : 4 - (bytes_w % 4)))*info_hdr.bmp_height;
    hdr.fileSize = PIX_OFFSET + info_hdr.image_size;
    fwrite(&hdr, sizeof(bmp_header), 1, fp);
    fwrite(&info_hdr, sizeof(bmp_info_header), 1, fp);
    fclose(fp);
    free(out_str);
    free(colours);
    return 0;
}
