#include <stdio.h>
#include <stdlib.h>

void pack_file(FILE *out, const char *filename, const char *var_name) {
    FILE *in = fopen(filename, "rb");
    if (!in) { printf("Ошибка: не найден файл %s!\n", filename); exit(1); }
    fseek(in, 0, SEEK_END);
    long sz = ftell(in);
    fseek(in, 0, SEEK_SET);
    unsigned char *buf = malloc(sz);
    fread(buf, 1, sz, in);
    fclose(in);

    fprintf(out, "const unsigned int %s_len = %ld;\n", var_name, sz);
    fprintf(out, "const unsigned char %s_data[] = {\n", var_name);
    for (long i = 0; i < sz; i++) {
        fprintf(out, "0x%02X,", buf[i]);
        if ((i + 1) % 32 == 0) fprintf(out, "\n");
    }
    fprintf(out, "\n};\n\n");
    free(buf);
    printf("Упакован %s (%ld байт)\n", filename, sz);
}

int main(void) {
    FILE *out = fopen("payload.h", "w");
    fprintf(out, "#ifndef PAYLOAD_H\n#define PAYLOAD_H\n\n");
    pack_file(out, "breaker.exe", "exe");
    pack_file(out, "raylib.dll", "dll");
    fprintf(out, "#endif\n");
    fclose(out);
    printf("Готово! Создан payload.h\n");
    return 0;
}
