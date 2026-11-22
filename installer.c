#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "payload.h"

extern int CreateDirectoryA(const char *p, void *s);
extern unsigned int GetEnvironmentVariableA(const char *n, char *b, unsigned int s);
extern void* ShellExecuteA(void* h, const char *o, const char *f, const char *p, const char *d, int s);
typedef struct { void *hw, *pr; char *dn; const char *t; unsigned int fl; void *fn; long lp; int im; } BI;
extern void* SHBrowseForFolderA(BI *b); extern int SHGetPathFromIDListA(void *p, char *s); extern void CoTaskMemFree(void *p);

Font load_rus_font(int sz) {
    int *cp = malloc(512 * sizeof(int)), c = 0;
    for (int i = 32; i < 127; i++) cp[c++] = i;
    for (int i = 0x0400; i <= 0x04FF; i++) cp[c++] = i;
    Font f = GetFontDefault();
    if (FileExists("C:\\Windows\\Fonts\\segoeui.ttf")) {
        f = LoadFontEx("C:\\Windows\\Fonts\\segoeui.ttf", sz, cp, c);
        SetTextureFilter(f.texture, TEXTURE_FILTER_BILINEAR);
    }
    free(cp);
    return f;
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_UNDECORATED); InitWindow(640, 380, "Setup"); SetTargetFPS(60);
    Font fnt = load_rus_font(22);

    float *sx = malloc(40 * 4), *sy = malloc(40 * 4), *sv = malloc(40 * 4);
    for (int i = 0; i < 40; i++) { sx[i] = (float)(rand()%640); sy[i] = (float)(rand()%380); sv[i] = 15.0f + rand()%25; }

    char *up = calloc(512, 1), *dir = calloc(512, 1), *exe = calloc(512, 1), *dll = calloc(512, 1);
    strcpy(up, "C:");
    GetEnvironmentVariableA("USERPROFILE", up, 260);
    sprintf(dir, "%s\\DesktopBreaker", up);

    int st = 0, step = 0; float prg = 0; bool ed = 0;
    while (!WindowShouldClose()) {
        float dt = GetFrameTime(); Vector2 m = GetMousePosition();
        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON) && m.y < 42 && m.x < 595) { Vector2 d = GetMouseDelta(), wp = GetWindowPosition(); SetWindowPosition((int)(wp.x+d.x), (int)(wp.y+d.y)); }

        if (st == 0 && ed) {
            for (int k = GetCharPressed(); k > 0; k = GetCharPressed()) { int l = (int)strlen(dir); if (k >= 32 && k <= 125 && l < 240) { dir[l] = (char)k; dir[l+1] = 0; } }
            if (IsKeyPressed(KEY_BACKSPACE)) { int l = (int)strlen(dir); if (l > 0) dir[l-1] = 0; }
        }
        if (st == 1) {
            prg += dt * 1.4f;
            if (prg > 0.25f && step == 0) { CreateDirectoryA(dir, 0); sprintf(exe, "%s\\breaker.exe", dir); sprintf(dll, "%s\\raylib.dll", dir); step = 1; }
            if (prg > 0.55f && step == 1) { FILE *f = fopen(exe, "wb"); if (f) { fwrite(exe_data, 1, exe_len, f); fclose(f); } step = 2; }
            if (prg > 0.85f && step == 2) { FILE *f = fopen(dll, "wb"); if (f) { fwrite(dll_data, 1, dll_len, f); fclose(f); } step = 3; }
            if (prg >= 1.0f) { prg = 1.0f; st = 2; }
        }

        BeginDrawing(); ClearBackground((Color){18, 20, 26, 255});
        for (int i = 0; i < 40; i++) { sy[i] -= sv[i]*dt; if (sy[i] < -10) sy[i] = 390; DrawCircle((int)sx[i], (int)sy[i], 2, Fade(SKYBLUE, 0.22f)); }
        DrawRectangleLines(0, 0, 640, 380, (Color){55, 65, 85, 255}); DrawRectangle(0, 0, 640, 42, (Color){26, 30, 38, 255});
        DrawTextEx(fnt, "DESKTOP BREAKER ULTRA  |  УСТАНОВЩИК", (Vector2){18, 10}, 18, 1, WHITE);

        Rectangle cls = {598, 6, 34, 30}; bool h_cls = CheckCollisionPointRec(m, cls);
        DrawRectangleRec(cls, h_cls ? RED : BLANK); DrawTextEx(fnt, "X", (Vector2){610, 10}, 18, 1, WHITE);
        if (h_cls && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) break;

        DrawTextEx(fnt, "Desktop Breaker Ultra v1.0", (Vector2){35, 62}, 30, 1, SKYBLUE);
        DrawTextEx(fnt, "Интерактивный разрушитель рабочего стола", (Vector2){35, 98}, 18, 1, LIGHTGRAY);
        DrawRectangleRounded((Rectangle){35, 140, 570, 130}, 0.1f, 8, (Color){28, 32, 42, 255});

        if (st == 0) {
            DrawTextEx(fnt, "Папка установки:", (Vector2){55, 158}, 18, 1, LIGHTGRAY);
            Rectangle pb = {55, 192, 420, 38}; DrawRectangleRec(pb, (Color){16, 19, 26, 255});
            DrawRectangleLinesEx(pb, 1.5f, ed ? SKYBLUE : (Color){65, 75, 95, 255}); DrawTextEx(fnt, dir, (Vector2){65, 200}, 18, 1, WHITE);
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) ed = CheckCollisionPointRec(m, pb);

            Rectangle bb = {485, 192, 100, 38}; bool hb = CheckCollisionPointRec(m, bb);
            DrawRectangleRounded(bb, 0.2f, 6, hb ? (Color){65, 78, 105, 255} : (Color){48, 56, 76, 255}); DrawTextEx(fnt, "Обзор...", (Vector2){502, 200}, 18, 1, WHITE);
            if (hb && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                char *ch = calloc(512, 1); BI bi = {0}; bi.t = "Выберите папку установки:"; bi.fl = 0x41;
                void *id = SHBrowseForFolderA(&bi); if (id) { if (SHGetPathFromIDListA(id, ch)) sprintf(dir, "%s\\DesktopBreaker", ch); CoTaskMemFree(id); }
                free(ch);
            }

            Rectangle btn = {405, 300, 200, 48}; bool h = CheckCollisionPointRec(m, btn);
            DrawRectangleRounded(btn, 0.25f, 8, h ? (Color){50, 170, 255, 255} : (Color){30, 130, 220, 255}); DrawTextEx(fnt, "УСТАНОВИТЬ", (Vector2){442, 313}, 20, 1, WHITE);
            if (h && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) st = 1;
        } else if (st == 1) {
            DrawTextEx(fnt, step==0?"Создание папки...":(step==1?"Распаковка breaker.exe...":"Распаковка raylib.dll..."), (Vector2){55, 165}, 20, 1, WHITE);
            DrawRectangleRounded((Rectangle){55, 205, 530, 28}, 0.3f, 8, (Color){15, 18, 24, 255});
            DrawRectangleRounded((Rectangle){55, 205, 530*prg, 28}, 0.3f, 8, SKYBLUE);
            DrawTextEx(fnt, TextFormat("%d%%", (int)(prg*100)), (Vector2){302, 209}, 18, 1, WHITE);
        } else {
            DrawTextEx(fnt, "Установка успешно завершена!", (Vector2){55, 165}, 24, 1, LIME);
            DrawTextEx(fnt, "Программа готова к работе:", (Vector2){55, 202}, 18, 1, LIGHTGRAY);
            Rectangle rb = {385, 300, 220, 48}; bool h = CheckCollisionPointRec(m, rb);
            DrawRectangleRounded(rb, 0.25f, 8, h ? (Color){60, 210, 100, 255} : (Color){40, 175, 80, 255}); DrawTextEx(fnt, "ЗАПУСТИТЬ ИГРУ", (Vector2){412, 313}, 20, 1, WHITE);
            if (h && IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) { ShellExecuteA(0, "open", exe, 0, dir, 1); break; }
        }
        EndDrawing();
    }
    UnloadFont(fnt); CloseWindow(); return 0;
}
