#include "raylib.h"
#include <stdlib.h>
#include "water.h"

extern double sin(double x); extern double cos(double x); extern double sqrt(double x);
extern void* GetDC(void* h); extern int ReleaseDC(void* h, void* dc);
extern void* CreateCompatibleDC(void* dc); extern void* CreateCompatibleBitmap(void* dc, int w, int h);
extern void* SelectObject(void* dc, void* h); extern int DeleteObject(void* h); extern int DeleteDC(void* dc);
extern int BitBlt(void* d, int x, int y, int w, int h, void* s, int x1, int y1, unsigned int r);
extern int GetDIBits(void* d, void* b, unsigned int st, unsigned int c, void* p, void* i, unsigned int u);

#define TS 42
#define MP 1200
#define MG 12

typedef struct { Rectangle src; float x, y, vx, vy, ang, vang, burn; bool brk, eaten; } Tile;
typedef struct { float x, y, vx, vy, life, max, sz; Color col; int kind; } Part;
typedef struct { float x, y, vx, vy, t, ty; bool on, nuke; } Gren;

Tile *tl; Part *pt; Gren *gr;
int cols, rows, tc, grab = -1, bh_mass = 0;
float wh_timer = 0, wh_x = 0, wh_y = 0;
float s_sh=0, s_bm=0, s_nk=0, s_wh=0, s_ls=0, s_fl=0, s_hl=0, ph_l=0, ph_h=0, ph_w=1200, lp=0;

void AudioCb(void *buf, unsigned int frames) {
    short *d = (short *)buf;
    for (unsigned int i = 0; i < frames; i++) {
        float w = ((float)(rand()%2000)/1000.0f) - 1.0f, out = 0; lp += (w - lp)*0.12f;
        if (s_sh > 0.001f) { out += (w*0.5f + lp*1.4f)*s_sh; s_sh *= 0.9993f; }
        if (s_bm > 0.001f) { ph_h += 55.0f/44100; out += (lp*1.8f + (float)sin(ph_h*6.283f)*0.9f)*s_bm; s_bm *= 0.9997f; }
        if (s_nk > 0.001f) { ph_h += 32.0f/44100; out += (lp*2.2f + (float)sin(ph_h*6.283f)*1.2f)*s_nk; s_nk *= 0.99991f; }
        if (s_wh > 0.01f)  { ph_w = ph_w > 180 ? ph_w - 450.0f/44100 : 180; ph_l += ph_w/44100; out += (float)sin(ph_l*6.283f)*0.35f*s_wh; s_wh *= 0.9995f; }
        if (s_fl > 0.01f)  { out += lp*0.65f*s_fl; s_fl *= 0.999f; }
        if (s_ls > 0.01f)  { ph_l += 520.0f/44100; out += (((ph_l-(int)ph_l)*2-1)*0.35f + w*0.15f)*s_ls; s_ls *= 0.999f; }
        if (s_hl > 0.01f)  { ph_h += (48.0f + bh_mass*3.0f)/44100; out += (float)sin(ph_h*6.283f)*0.5f*s_hl; s_hl *= 0.999f; }
        d[i] = (short)((out > 1 ? 1 : (out < -1 ? -1 : out)) * 25000.0f);
    }
}

void sp(float x, float y, float vx, float vy, float l, float sz, Color c, int k) {
    for (int i = 0; i < MP; i++) if (pt[i].life <= 0) { pt[i] = (Part){x, y, vx, vy, l, l, sz, c, k}; return; }
}

Texture2D grab_scr(int w, int h) {
    void *sc = GetDC(0), *mc = CreateCompatibleDC(sc), *bm = CreateCompatibleBitmap(sc, w, h);
    SelectObject(mc, bm); BitBlt(mc, 0, 0, w, h, sc, 0, 0, 0x00CC0020);
    int *b = calloc(16, 4); *b = 40; *(b+1) = w; *(b+2) = -h; *(b+3) = 1 | (32 << 16);
    unsigned char *px = malloc(w*h*4); GetDIBits(mc, bm, 0, h, px, b, 0); free(b);
    for (int i = 0; i < w*h*4; i += 4) { unsigned char t = px[i]; px[i] = px[i+2]; px[i+2] = t; px[i+3] = 255; }
    Texture2D tx = LoadTextureFromImage((Image){px, w, h, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8});
    free(px); DeleteObject(bm); DeleteDC(mc); ReleaseDC(0, sc); return tx;
}

void reset_tl(void) {
    for (int r = 0; r < rows; r++) for (int c = 0; c < cols; c++) {
        int i = r*cols + c; float x = (float)(c*TS), y = (float)(r*TS);
        tl[i] = (Tile){{x, y, TS, TS}, x + TS*0.5f, y + TS*0.5f, 0, 0, 0, 0, 0, false, false};
    }
    for (int i = 0; i < MG; i++) gr[i].on = false;
    bh_mass = 0; wh_timer = 0;
}

void boom(float ex, float ey, float rad, float pwr, bool nk) {
    if (nk) { s_nk = 1; s_wh = 0; sp(ex, ey, 0, 0, 0.7f, rad*1.6f, WHITE, 3); sp(ex, ey, 0, 0, 0.5f, rad*1.1f, SKYBLUE, 3); }
    else    { s_bm = 1; sp(ex, ey, 0, 0, 0.35f, rad*1.4f, WHITE, 3); }
    for (int i = 0; i < tc; i++) if (!tl[i].eaten) {
        float dx = tl[i].x - ex, dy = tl[i].y - ey, d = (float)sqrt(dx*dx + dy*dy);
        if (d < rad) { tl[i].brk = true; tl[i].burn += (1-d/rad)*(nk?1.3f:0.75f); float f = (1-d/rad)*pwr; tl[i].vx += dx/(d+1)*f+rand()%200-100; tl[i].vy += dy/(d+1)*f-(nk?550:320); tl[i].vang += rand()%700-350; }
    }
    for (int k = 0; k < (nk ? 120 : 35); k++) { float a = (rand()%628)/100.f, s = (nk?200:120)+rand()%(nk?700:400); sp(ex, ey, (float)cos(a)*s, (float)sin(a)*s-(nk?150:0), nk?1.2f:0.5f, (nk?18:8)+rand()%12, nk?(k%2?WHITE:YELLOW):ORANGE, 1); }
    if (nk) for (int k = 0; k < 35; k++) { float a = (rand()%628)/100.f; sp(ex+rand()%40-20, ey-k*6.0f, rand()%40-20, -240, 2.0f, 30, k%2?ORANGE:DARKGRAY, 2); sp(ex+(float)cos(a)*85, ey-160+(float)sin(a)*30, (float)cos(a)*130, -130, 2.2f, 40, k%2?RED:GRAY, 2); }
}

int main(void) {
    pt = calloc(MP, sizeof(Part)); gr = calloc(MG, sizeof(Gren));
    SetConfigFlags(FLAG_WINDOW_UNDECORATED | FLAG_WINDOW_TOPMOST); InitWindow(1, 1, "Breaker");
    int sw = GetMonitorWidth(0), sh = GetMonitorHeight(0); Texture2D tex = grab_scr(sw, sh);
    SetWindowSize(sw, sh); SetWindowPosition(0, 0); SetTargetFPS(60); HideCursor();
    InitAudioDevice(); SetAudioStreamBufferSizeDefault(2048);
    AudioStream st = LoadAudioStream(44100, 16, 1); SetAudioStreamCallback(st, AudioCb); PlayAudioStream(st);

    cols = sw/TS + 1; rows = sh/TS + 1; tc = cols*rows; tl = calloc(tc, sizeof(Tile)); reset_tl();
    int wpn = 1; float shk = 0, cd = 0, fls = 0, nfls = 0, h = TS*0.5f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime(); if (dt > 0.033f) dt = 0.033f;
        if (cd > 0) cd -= dt; if (shk > 0) shk -= dt*35; if (fls > 0) fls -= dt*4.5f; if (nfls > 0) nfls -= dt*1.4f;
        for (int k = 1; k <= 6; k++) if (IsKeyPressed(KEY_ZERO + k)) wpn = k;
        if (IsKeyPressed(KEY_R)) { reset_tl(); grab = -1; }

        Vector2 m = GetMousePosition();
        if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) for (int i = 0; i < tc; i++) if (!tl[i].eaten && (tl[i].x-m.x)*(tl[i].x-m.x)+(tl[i].y-m.y)*(tl[i].y-m.y) < TS*TS) { tl[i].brk = true; grab = i; break; }
        if (IsMouseButtonReleased(MOUSE_RIGHT_BUTTON)) grab = -1;
        if (grab != -1) { tl[grab].vx = (m.x-tl[grab].x)*18; tl[grab].vy = (m.y-tl[grab].y)*18; tl[grab].x = m.x; tl[grab].y = m.y; }

        // Извержение Белой дыры при перегрузке черной
        if (wh_timer > 0) {
            wh_timer -= dt; shk = 14; s_bm = 0.7f;
            for (int i = 0; i < tc; i++) if (tl[i].eaten && rand()%4 == 0) {
                tl[i].eaten = false; tl[i].brk = true; tl[i].x = wh_x; tl[i].y = wh_y;
                float a = (rand()%628)/100.f, spd = 700 + rand()%900;
                tl[i].vx = (float)cos(a)*spd; tl[i].vy = (float)sin(a)*spd - 300; tl[i].vang = rand()%800 - 400;
                sp(wh_x, wh_y, tl[i].vx*0.5f, tl[i].vy*0.5f, 0.4f, 6, SKYBLUE, 0);
            }
            if (wh_timer <= 0) { for (int i = 0; i < tc; i++) if (tl[i].eaten) { tl[i].eaten = false; tl[i].brk = true; tl[i].x = wh_x; tl[i].y = wh_y; tl[i].vy = -600; } bh_mass = 0; }
        }

        if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            if (wpn == 1 && cd <= 0) { cd = 0.14f; shk = 10; fls = 0.35f; s_sh = 1; for (int i = 0; i < tc; i++) if (!tl[i].eaten) { float dx = tl[i].x-m.x, dy = tl[i].y-m.y, d = (float)sqrt(dx*dx+dy*dy); if (d < 90) { tl[i].brk = true; tl[i].vx += dx/(d+1)*(90-d)*9+rand()%140-70; tl[i].vy += dy/(d+1)*(90-d)*9-260; tl[i].vang += rand()%420-210; } } for (int k = 0; k < 18; k++) { float a = (rand()%628)/100.f, s = 180+rand()%340; sp(m.x, m.y, (float)cos(a)*s, (float)sin(a)*s, 0.35f, 3, YELLOW, 0); } }
            else if (wpn == 2) { s_fl = 0.85f; for (int i = 0; i < tc; i++) if (!tl[i].eaten && (tl[i].x-m.x)*(tl[i].x-m.x)+(tl[i].y-m.y)*(tl[i].y-m.y) < 4600) { tl[i].burn += dt*2; if (tl[i].burn > 0.5f && !tl[i].brk) { tl[i].brk = true; tl[i].vang = (float)(rand()%140-70); } } for (int k = 0; k < 5; k++) sp(m.x+rand()%22-11, m.y+rand()%22-11, rand()%140-70, -150-rand()%160, 0.5f, 14, rand()%2?ORANGE:YELLOW, 1); }
            else if (wpn == 3) { s_ls = 0.7f; shk = 2.5f; for (int i = 0; i < tc; i++) if (!tl[i].eaten) { float dx = tl[i].x-m.x, dy = tl[i].y-m.y; if (dx*dx+dy*dy < 1400) { tl[i].brk = true; tl[i].burn = 0.65f; tl[i].vx += dx>0?150:-150; tl[i].vy -= 100; tl[i].vang = dx>0?200:-200; } } for (int k = 0; k < 4; k++) { float a = (rand()%628)/100.f; sp(m.x, m.y, (float)cos(a)*220, (float)sin(a)*220, 0.25f, 2.5f, SKYBLUE, 0); } }
            else if (wpn == 4 && wh_timer <= 0) {
                s_hl = 0.85f; shk = 3 + bh_mass*0.3f;
                for (int i = 0; i < tc; i++) if (!tl[i].eaten) {
                    float dx = m.x-tl[i].x, dy = m.y-tl[i].y, d = (float)sqrt(dx*dx+dy*dy);
                    if (d < 28.0f + bh_mass*0.6f) { tl[i].eaten = true; bh_mass++; sp(m.x, m.y, 0, 0, 0.2f, 25, PURPLE, 3); if (bh_mass >= 25) { wh_timer = 0.8f; wh_x = m.x; wh_y = m.y; nfls = 0.8f; sp(wh_x, wh_y, 0, 0, 0.5f, 450, WHITE, 3); break; } }
                    else if (d < 320) { tl[i].brk = true; float nx = dx/(d+1), ny = dy/(d+1); tl[i].vx = (tl[i].vx+(nx*2200-ny*1350)*dt)*0.95f; tl[i].vy = (tl[i].vy+(ny*2200+nx*1350)*dt)*0.95f; tl[i].vang += 380*dt; }
                }
            }
            else if (wpn == 5 && cd <= 0) { cd = 0.35f; for (int i = 0; i < MG; i++) if (!gr[i].on) { gr[i] = (Gren){m.x, m.y, (float)(rand()%240-120), -250, 1.8f, 0, true, false}; break; } }
            else if (wpn == 6 && cd <= 0) { cd = 1.0f; s_wh = 0.9f; ph_w = 1300; for (int i = 0; i < MG; i++) if (!gr[i].on) { gr[i] = (Gren){m.x, -40, 0, 520, 5, m.y, true, true}; break; } }
        }

        for (int i = 0; i < MG; i++) if (gr[i].on) {
            gr[i].vy += (gr[i].nuke ? 600 : 1000)*dt; gr[i].x += gr[i].vx*dt; gr[i].y += gr[i].vy*dt; gr[i].t -= dt;
            if (gr[i].nuke) { sp(gr[i].x+rand()%10-5, gr[i].y-25, rand()%30-15, -80, 0.3f, 5, LIME, 0); if (gr[i].y >= gr[i].ty || gr[i].y >= sh-20) { gr[i].on = false; shk = 45; nfls = 1; boom(gr[i].x, gr[i].y, sw*0.95f, 1850, true); } }
            else {
                if (gr[i].y > sh - 12) { gr[i].y = sh - 12; gr[i].vy *= -0.55f; gr[i].vx *= 0.8f; }
                if (gr[i].t <= 0) { gr[i].on = false; shk = 22; fls = 0.75f; boom(gr[i].x, gr[i].y, 240, 950, false); }
            }
        }

        for (int i = 0; i < tc; i++) if (!tl[i].eaten) {
            if (tl[i].burn > 0.4f && rand()%45 == 0) sp(tl[i].x, tl[i].y, rand()%30 - 15, -55, 1.1f, 16, Fade(GRAY, 0.35f), 2);
            if (!tl[i].brk || i == grab) continue;
            if (!(wpn == 4 && wh_timer <= 0 && IsMouseButtonDown(MOUSE_LEFT_BUTTON))) tl[i].vy += 1150.0f * dt;
            tl[i].x += tl[i].vx * dt;
            tl[i].y += tl[i].vy * dt;
            tl[i].ang += tl[i].vang * dt;
            if (tl[i].y > sh - h) { tl[i].y = sh - h; tl[i].vy *= -0.35f; tl[i].vx *= 0.8f; tl[i].vang *= 0.7f; }
            if (tl[i].x < h) { tl[i].x = h; tl[i].vx *= -0.5f; }
            if (tl[i].x > sw - h) { tl[i].x = sw - h; tl[i].vx *= -0.5f; }
        }

        float md = TS * 0.88f;
        for (int it = 0; it < 3; it++) for (int i = 0; i < tc; i++) if (tl[i].brk && !tl[i].eaten) for (int j = i + 1; j < tc; j++) if (tl[j].brk && !tl[j].eaten) {
            float dx = tl[j].x - tl[i].x, dy = tl[j].y - tl[i].y, d2 = dx * dx + dy * dy;
            if (d2 < md * md && d2 > 0.001f) {
                float d = (float)sqrt(d2), nx = dx / d, ny = dy / d, ov = (md - d) * 0.5f;
                if (i != grab) { tl[i].x -= nx * ov; tl[i].y -= ny * ov; }
                if (j != grab) { tl[j].x += nx * ov; tl[j].y += ny * ov; }
                float vn = (tl[j].vx - tl[i].vx) * nx + (tl[j].vy - tl[i].vy) * ny;
                if (vn < 0) {
                    float imp = -vn * 0.65f;
                    if (i != grab) { tl[i].vx -= nx * imp; tl[i].vy -= ny * imp; }
                    if (j != grab) { tl[j].vx += nx * imp; tl[j].vy += ny * imp; }
                }
            }
        }

        for (int i = 0; i < MP; i++) if (pt[i].life > 0) {
            if (pt[i].kind == 0) pt[i].vy += 650.0f * dt;
            if (pt[i].kind == 3) pt[i].sz += 1100.0f * dt;
            pt[i].x += pt[i].vx * dt;
            pt[i].y += pt[i].vy * dt;
            pt[i].life -= dt;
        }

        BeginDrawing();
        ClearBackground((Color){10, 10, 14, 255});
        int sx = shk > 0 ? rand() % (int)(shk + 1) - (int)shk / 2 : 0;
        int sy = shk > 0 ? rand() % (int)(shk + 1) - (int)shk / 2 : 0;

        for (int r = 0; r < rows; r++) for (int c = 0; c < cols; c++) {
            int i = r * cols + c;
            if (tl[i].brk || tl[i].eaten) continue;
            unsigned char b = tl[i].burn > 1.0f ? 40 : (unsigned char)(255 - tl[i].burn * 210);
            float px = tl[i].x + sx, py = tl[i].y + sy;
            DrawTexturePro(tex, tl[i].src, (Rectangle){px, py, TS, TS}, (Vector2){h, h}, 0, (Color){b, b, b, 255});
            if ((c > 0 && (tl[i - 1].brk || tl[i - 1].eaten)) || (c < cols - 1 && (tl[i + 1].brk || tl[i + 1].eaten)) ||
                (r > 0 && (tl[i - cols].brk || tl[i - cols].eaten)) || (r < rows - 1 && (tl[i + cols].brk || tl[i + cols].eaten))) {
                DrawRectangleLines((int)(px - h), (int)(py - h), TS, TS, Fade(BLACK, 0.7f));
                DrawLine((int)(px - h), (int)(py - 6), (int)(px + 4), (int)(py + 5), Fade(WHITE, 0.5f));
            }
        }

        for (int i = 0; i < tc; i++) if (tl[i].brk && !tl[i].eaten) {
            unsigned char b = tl[i].burn > 1.0f ? 45 : (unsigned char)(255 - tl[i].burn * 200);
            DrawTexturePro(tex, tl[i].src, (Rectangle){tl[i].x + sx, tl[i].y + sy, TS, TS}, (Vector2){h, h}, tl[i].ang, (Color){b, b, b, 255});
        }

        for (int i = 0; i < MG; i++) if (gr[i].on) {
            if (gr[i].nuke) {
                DrawRectangle((int)gr[i].x - 10, (int)gr[i].y - 25, 20, 42, (Color){65, 80, 60, 255});
                DrawCircle((int)gr[i].x, (int)gr[i].y + 17, 10, YELLOW);
            } else {
                DrawCircle((int)gr[i].x, (int)gr[i].y, 10, ((int)(gr[i].t * 12) % 2) ? RED : DARKGRAY);
            }
        }

        if (wh_timer > 0) {
            DrawCircle((int)wh_x, (int)wh_y, 70, Fade(SKYBLUE, 0.5f));
            DrawCircle((int)wh_x, (int)wh_y, 42, WHITE);
        } else if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
            if (wpn == 3) {
                DrawLineEx((Vector2){sw * 0.5f, (float)sh}, m, 16, Fade(BLUE, 0.45f));
                DrawLineEx((Vector2){sw * 0.5f, (float)sh}, m, 5, WHITE);
            } else if (wpn == 4) {
                float br = 22.0f + bh_mass * 1.4f;
                DrawCircle((int)m.x, (int)m.y, br + 16.0f, Fade(bh_mass > 18 ? RED : PURPLE, 0.45f));
                DrawCircle((int)m.x, (int)m.y, br, BLACK);
            }
        }

        for (int i = 0; i < MP; i++) if (pt[i].life > 0) {
            float a = pt[i].life / pt[i].max;
            if (pt[i].kind == 3) DrawCircleLines((int)pt[i].x, (int)pt[i].y, pt[i].sz, Fade(pt[i].col, a));
            else DrawCircle((int)pt[i].x, (int)pt[i].y, pt[i].sz * (pt[i].kind == 0 ? 1.0f : (0.5f + a * 0.7f)), Fade(pt[i].col, a));
        }

        if (fls > 0) DrawRectangle(0, 0, sw, sh, Fade(ORANGE, fls * 0.25f));
        if (nfls > 0) DrawRectangle(0, 0, sw, sh, Fade(WHITE, nfls * 0.85f));

        Color cc = wpn == 1 ? YELLOW : (wpn == 2 ? ORANGE : (wpn == 3 ? SKYBLUE : (wpn == 4 ? VIOLET : (wpn == 5 ? RED : LIME))));
        DrawCircleLines((int)m.x, (int)m.y, 14.0f + shk * 1.5f, cc);
        DrawRectangle((sw - 720) / 2, 10, 720, 32, Fade(BLACK, 0.85f));
        DrawText("1:Gun 2:Flame 3:Laser 4:Black/WhiteHole 5:Grenade 6:TSAR NUKE | LMB:Fire RMB:Throw R:Fix", (sw - 720) / 2 + 15, 18, 14, WHITE);
        EndDrawing();
    }

    UnloadAudioStream(st);
    CloseAudioDevice();
    UnloadTexture(tex);
    free(tl);
    free(pt);
    free(gr);
    CloseWindow();
    return 0;
}
