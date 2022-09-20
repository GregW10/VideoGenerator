//
// Created by mario on 18/09/2022.
//

#include "overhead.h"

static inline bool little_endian() {
    unsigned int test = 0x4ed85ba9;
    unsigned char arr[] = {0x4e, 0xd8, 0x5b, 0xa9};
    unsigned int *ptr = (unsigned int *) arr; // interpret char array as integer
    if (*ptr == test) { // if same, system is big-endian
        return false;
    }
    return true;
}

static inline void set_bit(unsigned char *c, const colour *col, const unsigned char *cutoff) {
    unsigned char avg = (col->r + col->g + col->b)/3;
    if (avg >= *cutoff) {
        ++(*c);
    }
    *c <<= 1;
}

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
    unsigned int bright_clr = (255 << 16) + (255 << 8) + 255; // white in RGB
    if (argc >= 4) {
        const char *dark_str = *(argv + 3);
        while (*dark_str) {
            dark_clr *= 10;
            dark_clr += *dark_str++ - 48;
        }
        dark_clr &= 0x00ffffff; // make zero the first byte of the RGB value if not already
        if (!little_endian()) { // if on big-endian system, must switch 0RGB to BGR0 value
            dark_clr = ((dark_clr & 0x000000ff) << 24) + ((dark_clr & 0x0000ff00) << 8) +
                                                         ((dark_clr & 0x00ff0000) >> 8);
        }
        if (argc == 5) {
            const char *bright_str = *(argv + 4);
            bright_clr = 0;
            while (*bright_str) {
                bright_clr *= 10;
                bright_clr += *bright_str++ - 48;
            }
            bright_clr &= 0x00ffffff;
        }
    }
    if (!little_endian()) {
        bright_clr = ((bright_clr & 0x000000ff) << 24) + ((bright_clr & 0x0000ff00) << 8) +
                                                         ((bright_clr & 0x00ff0000) >> 8);
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
    printf("malloc'ed size: %u\n", sizeof(colour)*info_hdr.bmp_width*info_hdr.bmp_height);
    if (!colours) {
        fprintf(stderr, "Memory allocation error - likely due to overly large BMP file size.\n");
        return -1;
    }
    colour *clr_ptr = colours;
    unsigned int i = 0;
    unsigned int j = 0;
    printf("original padding: %u\n", padding);
    for (; i < info_hdr.bmp_height; ++i) {
        fread(clr_ptr, sizeof(colour), info_hdr.bmp_width, fp);
        fseek(fp, padding, SEEK_CUR);
        clr_ptr += info_hdr.bmp_width;
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
    padding = info_hdr.bmp_width % 32 == 0 ? 0 : (32 - (info_hdr.bmp_width % 32)) / 8;
    unsigned int image_size = ((info_hdr.bmp_width % 8 == 0 ? info_hdr.bmp_width/8 : info_hdr.bmp_width/8 + 1) + padding)*info_hdr.bmp_height;
    unsigned int size = PIX_OFFSET + 8 + image_size;
    info_hdr.pixel_depth = 1; // 1 bpp
    info_hdr.image_size = image_size;
    hdr.fileSize = size;
    fwrite(&hdr, sizeof(bmp_header), 1, fp);
    fwrite(&info_hdr, sizeof(bmp_info_header), 1, fp);
    // unsigned char h[] = {0, 0, 0};
    // clr_ptr = colours;
    // for (i = 0; i < info_hdr.bmp_height; ++i) {
    //     fwrite(clr_ptr, sizeof(colour), info_hdr.bmp_width, fp);
    //     fwrite(h, sizeof(unsigned char), padding, fp);
    //     clr_ptr += info_hdr.bmp_width;
    // }

    fwrite(&dark_clr, sizeof(unsigned int), 1, fp); // write colour table to .bmp file
    fwrite(&bright_clr, sizeof(unsigned int), 1, fp);
    unsigned char c;
    clr_ptr = colours;
    unsigned char pad_arr[] = {0, 0, 0};
    unsigned char rem = info_hdr.bmp_width % 8;
    bool has_rem = rem != 0;
    unsigned int w_lim = !has_rem ? info_hdr.bmp_width/8 : info_hdr.bmp_width/8 + 1;
    unsigned char bit_count;
    long double avg;
    unsigned int tot_count = info_hdr.bmp_width*info_hdr.bmp_height;
    printf("End padding: %u\n", padding);
    for (i = 0; i < info_hdr.bmp_height; ++i) {
        for (j = 1; j <= w_lim; ++j) {
            c = 0;
            // if (j == w_lim) {
            //     if (has_rem) {
            //         for (bit_count = 0; bit_count < (8 - rem); ++bit_count) {
            //             c <<= 1;
            //             if (tot_count > 0) {
            //                 avg = (clr_ptr->r + clr_ptr->g + clr_ptr->b)/3.0l;
            //                 if (avg >= cutoff) {
            //                     ++c;
            //                 }
            //                 ++clr_ptr;
            //                 --tot_count;
            //             }
            //         }
            //         c <<= 8 - rem;
            //     }
            //     // else {
            //     //     goto other;
            //     // }
            // }
            // else {
            //     other:
            //     for (bit_count = 0; bit_count < 8; ++bit_count) {
            //         c <<= 1;
            //         if (tot_count > 0) {
            //             avg = (clr_ptr->r + clr_ptr->g + clr_ptr->b)/3.0l;
            //             if (avg >= cutoff) {
            //                 ++c;
            //             }
            //             ++clr_ptr;
            //             --tot_count;
            //         }
            //     }
            // }
            fputc(c, fp);
        }
        fwrite(pad_arr, sizeof(unsigned char), padding, fp);
    }
    fclose(fp);
    free(out_str);
    free(colours);
    return 0;
}
