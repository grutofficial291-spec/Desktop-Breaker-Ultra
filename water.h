#ifndef WATER_H
#define WATER_H

extern double sqrt(double x);
extern void Sleep(unsigned int ms);
extern void* CreateThread(void *a, unsigned long s, unsigned int (*f)(void*), void *p, unsigned int fl, unsigned int *id);

static const char *refract_fs =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0;\n"
    "uniform sampler2D deskTex;\n"
    "uniform vec2 texel;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    float d = texture(texture0, fragTexCoord).r;\n"
    "    if (d < 0.22) discard;\n"
    "    float dx = texture(texture0, fragTexCoord + vec2(texel.x, 0)).r - texture(texture0, fragTexCoord - vec2(texel.x, 0)).r;\n"
    "    float dy = texture(texture0, fragTexCoord + vec2(0, texel.y)).r - texture(texture0, fragTexCoord - vec2(0, texel.y)).r;\n"
    "    vec3 norm = normalize(vec3(-dx * 4.2, -dy * 4.2, 1.0));\n"
    "    vec2 uv_screen = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);\n"
    "    vec2 refr_uv = clamp(uv_screen + norm.xy * 0.032 * smoothstep(0.22, 0.65, d), 0.001, 0.999);\n"
    "    vec3 bg = texture(deskTex, refr_uv).rgb;\n"
    "    float spec = pow(max(dot(norm, normalize(vec3(-0.4, -0.7, 0.6))), 0.0), 26.0);\n"
    "    float edge = smoothstep(0.22, 0.36, d) * (1.0 - smoothstep(0.38, 0.70, d));\n"
    "    float fresnel = clamp(length(vec2(dx, dy)) * 3.0, 0.0, 1.0);\n"
    "    vec3 water_tint = mix(bg * vec3(0.84, 0.94, 1.05), vec3(0.75, 0.90, 1.0), fresnel * 0.42);\n"
    "    finalColor = vec4(water_tint + vec3(1.0) * (spec * 1.15 + edge * 0.40), 1.0);\n"
    "}\n";

#define FAST_MW 28000
#define F_CELL 8

typedef struct { float x, y, vx, vy; bool on; } FDrop;

static FDrop *f_wt = 0;
static unsigned short *f_dens = 0;
static unsigned char *f_field = 0;
static int fd_w = 0, fd_h = 0, ff_w = 0, ff_h = 0;
static Texture2D f_wtx = {0};
static Shader f_shd = {0};
static volatile int f_jobs = 0;
static int fw_sw = 0, fw_sh = 0, fw_wpn = 1;
static float fw_dt = 0.016f, fw_mx = 0, fw_my = 0, fw_wht = 0, fw_whx = 0, fw_why = 0;
static bool fw_lmb = false;

static unsigned int __stdcall FastWaterThread(void *param) {
    int tid = (int)(size_t)param;
    int ch = FAST_MW / 3 + 1;
    int s = tid * ch, e = s + ch > FAST_MW ? FAST_MW : s + ch;

    while (1) {
        if ((f_jobs & (1 << tid)) == 0) { Sleep(1); continue; }
        float dt = fw_dt;
        int sw = fw_sw, sh = fw_sh;

        for (int i = s; i < e; i++) {
            if (!f_wt[i].on) continue;
            FDrop *d = &f_wt[i];

            if (fw_wpn == 4 && fw_lmb && fw_wht <= 0) {
                float dx = fw_mx - d->x, dy = fw_my - d->y, r2 = dx*dx + dy*dy;
                if (r2 < 900.0f) { d->on = false; continue; }
                if (r2 < 102400.0f) {
                    float inv = 1.0f / ((float)sqrt(r2) + 1.0f);
                    d->vx += (dx*inv*2200.0f - dy*inv*1300.0f) * dt;
                    d->vy += (dy*inv*2200.0f + dx*inv*1300.0f) * dt;
                }
            } else if (fw_wht > 0) {
                float dx = d->x - fw_whx, dy = d->y - fw_why, r2 = dx*dx + dy*dy;
                if (r2 < 144400.0f) {
                    float inv = 1.0f / ((float)sqrt(r2) + 1.0f);
                    d->vx += dx * inv * 2900.0f * dt;
                    d->vy += dy * inv * 2900.0f * dt;
                }
            }

            int gx = (int)(d->x / F_CELL);
            int gy = (int)(d->y / F_CELL);
            if (gx > 1 && gx < fd_w - 2 && gy > 1 && gy < fd_h - 2) {
                int idx = gy * fd_w + gx;
                float px = (float)(f_dens[idx - 1] - f_dens[idx + 1]);
                float py = (float)(f_dens[idx - fd_w] - f_dens[idx + fd_w]);
                d->vx += px * 42.0f * dt;
                d->vy += py * 42.0f * dt;
            }

            d->vy += 1150.0f * dt;
            d->vx *= 0.990f;
            d->vy *= 0.990f;
            d->x += d->vx * dt;
            d->y += d->vy * dt;

            if (d->x < 4.0f) { d->x = 4.0f; d->vx = -d->vx * 0.35f; }
            else if (d->x > sw - 4.0f) { d->x = sw - 4.0f; d->vx = -d->vx * 0.35f; }

            if (d->y < 4.0f) { d->y = 4.0f; d->vy = -d->vy * 0.25f; }
            else if (d->y > sh - 4.0f) { d->y = sh - 4.0f; d->vy = -d->vy * 0.15f; d->vx *= 0.94f; }
        }
        f_jobs &= ~(1 << tid);
    }
    return 0;
}

static void FastWaterEndShader(Texture2D *tex, int sw, int sh, int *wpn, float *wht, float *whx, float *why, float *sfl) {
    (EndShaderMode)();

    if (!f_wt) {
        f_wt = calloc(FAST_MW, sizeof(FDrop));
        fd_w = sw / F_CELL + 2;
        fd_h = sh / F_CELL + 2;
        f_dens = calloc(fd_w * fd_h, sizeof(unsigned short));

        ff_w = sw / 4 + 1;
        ff_h = sh / 4 + 1;
        f_field = calloc(ff_w * ff_h, 1);

        f_wtx = LoadTextureFromImage((Image){ f_field, ff_w, ff_h, 1, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE });
        SetTextureFilter(f_wtx, TEXTURE_FILTER_BILINEAR);

        f_shd = LoadShaderFromMemory(0, refract_fs);
        Vector2 txl = { 1.0f / ff_w, 1.0f / ff_h };
        SetShaderValue(f_shd, GetShaderLocation(f_shd, "texel"), &txl, SHADER_UNIFORM_VEC2);

        fw_sw = sw; fw_sh = sh;
        for (int t = 0; t < 3; t++) CreateThread(0, 0, FastWaterThread, (void*)(size_t)t, 0, 0);
    }

    bool alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
    if (IsKeyPressed(KEY_R) || (alt && IsKeyPressed(KEY_D))) {
        while (f_jobs != 0) Sleep(1);
        for (int i = 0; i < FAST_MW; i++) f_wt[i].on = false;
    }

    Vector2 m = GetMousePosition();
    if (*wpn == 9 && IsMouseButtonDown(MOUSE_LEFT_BUTTON) && f_jobs == 0) {
        *sfl = 0.45f;
        int spw = 0;
        for (int i = 0; i < FAST_MW && spw < 180; i++) {
            if (!f_wt[i].on) {
                f_wt[i] = (FDrop){
                    m.x + (float)(rand() % 70 - 35),
                    m.y + (float)(rand() % 40 - 20),
                    (float)(rand() % 340 - 170),
                    120.0f + (float)(rand() % 140),
                    true
                };
                spw++;
            }
        }
    }

    if (f_jobs == 0) {
        for (int i = 0; i < fd_w * fd_h; i++) f_dens[i] = 0;
        for (int i = 0; i < ff_w * ff_h; i++) f_field[i] = 0;

        for (int i = 0; i < FAST_MW; i++) {
            if (!f_wt[i].on) continue;
            int gx = (int)(f_wt[i].x / F_CELL);
            int gy = (int)(f_wt[i].y / F_CELL);
            if (gx >= 0 && gx < fd_w && gy >= 0 && gy < fd_h) f_dens[gy * fd_w + gx] += 12;

            int cx = (int)(f_wt[i].x * 0.25f);
            int cy = (int)(f_wt[i].y * 0.25f);
            if (cx > 1 && cx < ff_w - 2 && cy > 1 && cy < ff_h - 2) {
                int idx = cy * ff_w + cx;
                int v0 = f_field[idx] + 75; f_field[idx] = v0 > 255 ? 255 : v0;
                int v1 = f_field[idx - 1] + 40; f_field[idx - 1] = v1 > 255 ? 255 : v1;
                int v2 = f_field[idx + 1] + 40; f_field[idx + 1] = v2 > 255 ? 255 : v2;
                int v3 = f_field[idx - ff_w] + 40; f_field[idx - ff_w] = v3 > 255 ? 255 : v3;
                int v4 = f_field[idx + ff_w] + 40; f_field[idx + ff_w] = v4 > 255 ? 255 : v4;
            }
        }

        UpdateTexture(f_wtx, f_field);

        float dt = GetFrameTime();
        fw_dt = dt > 0.033f ? 0.033f : dt;
        fw_wpn = *wpn; fw_lmb = IsMouseButtonDown(MOUSE_LEFT_BUTTON);
        fw_mx = m.x; fw_my = m.y; fw_wht = *wht; fw_whx = *whx; fw_why = *why;
        f_jobs = 7;
    }

    (BeginShaderMode)(f_shd);
    SetShaderValueTexture(f_shd, GetShaderLocation(f_shd, "deskTex"), *tex);
    DrawTextureEx(f_wtx, (Vector2){0, 0}, 0.0f, 4.0f, WHITE);
    (EndShaderMode)();
}

#include "addons.h"
#undef MW
#define MW 0
#undef EndShaderMode
#define EndShaderMode() FastWaterEndShader(tex, sw, sh, wpn, wht, whx, why, sfl)

#endif
