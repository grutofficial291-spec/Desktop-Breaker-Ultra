#include <stdio.h>
#include <stdlib.h>

extern double sqrt(double x);
extern double sin(double x);
extern double atan2(double y, double x);

#define W 64
#define H 64

int main(void) {
    unsigned int *px = calloc(W * H, 4);

    for (int y = 0; y < H; y++) {
        for (int x = 0; x < W; x++) {
            unsigned int col = 0x00000000;
            if (y >= 54 && y <= 58 && x >= 20 && x <= 44) col = 0xFF3A4252;
            if (y >= 48 && y <= 54 && x >= 28 && x <= 36) col = 0xFF282E3A;
            if (x >= 4 && x <= 59 && y >= 6 && y <= 48) {
                col = 0xFF2B3240;
                if (x >= 8 && x <= 55 && y >= 10 && y <= 43) {
                    col = (x < 32) ? 0xFF1E6BB8 : 0xFF175596;
                    double dx = (double)(x - 34), dy = (double)(y - 26);
                    double dist = sqrt(dx * dx + dy * dy);
                    double crack = sin(atan2(dy, dx) * 5.0 + dist * 0.15);
                    if (crack < 0) crack = -crack;
                    if (dist < 22.0 && crack < 0.08) col = 0xFFE8F4FF;
                    if (dist < 8.5) col = 0xFFFF6A00;
                    if (dist < 6.5) col = 0xFFFFD000;
                    if (dist < 4.8) col = 0xFF101218;
                }
            }
            px[y * W + x] = col;
        }
    }

    FILE *f = fopen("app.ico", "wb");
    if (!f) return 1;

    unsigned short *hdr = calloc(3, 2);
    hdr[0] = 0; hdr[1] = 1; hdr[2] = 1;
    fwrite(hdr, 2, 3, f);

    unsigned char *ent = calloc(16, 1);
    ent[0] = W; ent[1] = H; ent[4] = 1; ent[6] = 32; ent[12] = 22;
    *(unsigned int *)(ent + 8) = 40 + W * H * 4 + (W * H / 8);
    fwrite(ent, 1, 16, f);

    int *bih = calloc(10, 4);
    bih[0] = 40; bih[1] = W; bih[2] = H * 2; bih[3] = 1 | (32 << 16); bih[5] = W * H * 4;
    fwrite(bih, 4, 10, f);

    for (int y = H - 1; y >= 0; y--) fwrite(&px[y * W], 4, W, f);

    unsigned char *mask = calloc(W * H / 8, 1);
    fwrite(mask, 1, W * H / 8, f);

    fclose(f);
    free(px); free(hdr); free(ent); free(bih); free(mask);
    printf("app.ico ready!\n");
    return 0;
}
