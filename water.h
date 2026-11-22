#ifndef WATER_H
#define WATER_H

extern double sqrt(double x);
extern void Sleep(unsigned int ms);
extern void* CreateThread(void *a, unsigned long s, unsigned int (*f)(void*), void *p, unsigned int fl, unsigned int *id);

static void FastWaterEndShader(Texture2D *tex, int sw, int sh, int *wpn, float *wht, float *whx, float *why, float *sfl);

#define KEY_NINE -1
#define EndShaderMode() FastWaterEndShader(tex, sw, sh, wpn, wht, whx, why, sfl)
#include "addons.h"
#undef EndShaderMode
#undef KEY_NINE

static const char *refract_fs =
    "#version 330\n"
    "in vec2 fragTexCoord;\n"
    "uniform sampler2D texture0;\n"
    "uniform sampler2D deskTex;\n"
    "uniform vec2 texel;\n"
    "out vec4 finalColor;\n"
    "void main() {\n"
    "    float d = texture(texture0, fragTexCoord).r;\n"
    "    if (d < 0.25) discard;\n"
    "    float dx = texture(texture0, fragTexCoord + vec2(texel.x, 0)).r - texture(texture0, fragTexCoord - vec2(texel.x, 0)).r;\n"
    "    float dy = texture(texture0, fragTexCoord + vec2(0, texel.y)).r - texture(texture0, fragTexCoord - vec2(0, texel.y)).r;\n"
    "    vec3 norm = normalize(vec3(-dx * 4.8, -dy * 4.8, 1.0));\n"
    "    vec2 uv_screen = vec2(fragTexCoord.x, 1.0 - fragTexCoord.y);\n"
    "    vec2 refr_uv = clamp(uv_screen + norm.xy * 0.038 * smoothstep(0.25, 0.70, d), 0.001, 0.999);\n"
    "    vec3 bg = texture(deskTex, refr_uv).rgb;\n"
    "    float spec = pow(max(dot(norm, normalize(vec3(-0.4, -0.7, 0.6))), 0.0), 28.0);\n"
    "    float edge = smoothstep(0.25, 0.36, d) * (1.0 - smoothstep(0.38, 0.72, d));\n"
    "    float fresnel = clamp(length(vec2(dx, dy)) * 3.2, 0.0, 1.0);\n"
    "    vec3 water_tint = mix(bg * vec3(0.85, 0.95, 1.06), vec3(0.78, 0.92, 1.0), fresnel * 0.42);\n"
    "    finalColor = vec4(water_tint + vec3(1.0) * (spec * 1.25 + edge * 0.45), 1.0);\n"
    "}\n";

#define F_CELL 28
#define F_SCALE 6
#define BASE_RAD 6.5f   // Аккуратные мелкие капли на старте (~13 px)
#define MAX_MASS 12.0f  // Максимальный размер после слияния (~44 px в диаметре)

typedef struct { float x, y, vx, vy, mass, rad; bool on; int next; } FDrop;

static FDrop *f_wt = 0;
static volatile int f_cnt = 0;
static int f_cap = 0;
static int *f_grid = 0;
static int *f_dens = 0;
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

    while (1) {
        if ((f_jobs & (1 << tid)) == 0) { Sleep(1); continue; }
        int total = f_cnt;
        int ch = total / 3 + 1;
        int s = tid * ch, e = s + ch > total ? total : s + ch;
        float dt = fw_dt;
        int sw = fw_sw, sh = fw_sh;

        for (int i = s; i < e; i++) {
            if (!f_wt[i].on) continue;
            FDrop *d = &f_wt[i];

            if (fw_wpn == 4 && fw_lmb && fw_wht <= 0) {
                float dx = fw_mx - d->x, dy = fw_my - d->y, r2 = dx*dx + dy*dy;
                if (r2 < 1200.0f) { d->on = false; continue; }
                if (r2 < 140000.0f) {
                    float inv = 1.0f / ((float)sqrt(r2) + 1.0f);
                    d->vx += (dx*inv*2400.0f - dy*inv*1400.0f) * dt;
                    d->vy += (dy*inv*2400.0f + dx*inv*1400.0f) * dt;
                }
            } else if (fw_wht > 0) {
                float dx = d->x - fw_whx, dy = d->y - fw_why, r2 = dx*dx + dy*dy;
                if (r2 < 180000.0f) {
                    float inv = 1.0f / ((float)sqrt(r2) + 1.0f);
                    d->vx += dx * inv * 3200.0f * dt;
                    d->vy += dy * inv * 3200.0f * dt;
                }
            }

            // Мягкое выравнивание давления без выброса вверх
            int gx = (int)(d->x / F_CELL);
            int gy = (int)(d->y / F_CELL);
            if (gx > 0 && gx < fd_w - 1 && gy > 0 && gy < fd_h - 1) {
                int idx = gy * fd_w + gx;
                float px = (float)(f_dens[idx - 1] - f_dens[idx + 1]);
                float py = (float)(f_dens[idx - fd_w] - f_dens[idx + fd_w]);
                if (py < 0.0f) py *= 0.35f; // Ограничиваем силу вверх, чтобы капли не подлетали
                d->vx += px * 45.0f * dt;
                d->vy += py * 45.0f * dt;
            }

            // Гравитация всегда тянет капли вниз
            d->vy += 1150.0f * dt;
            d->vx *= 0.985f;
            d->vy *= 0.985f;
            d->x += d->vx * dt;
            d->y += d->vy * dt;

            float r = d->rad;
            if (d->x < r) { d->x = r; d->vx = -d->vx * 0.35f; }
            else if (d->x > sw - r) { d->x = sw - r; d->vx = -d->vx * 0.35f; }

            if (d->y < r) { d->y = r; if (d->vy < 0.0f) d->vy = 0.0f; }
            else if (d->y > sh - r) { d->y = sh - r; d->vy = -d->vy * 0.15f; d->vx *= 0.92f; }
        }
        f_jobs &= ~(1 << tid);
    }
    return 0;
}

static void FastWaterEndShader(Texture2D *tex, int sw, int sh, int *wpn, float *wht, float *whx, float *why, float *sfl) {
    (EndShaderMode)();

    if (IsKeyPressed(KEY_NINE)) *wpn = 9;

    if (!f_dens) {
        f_cap = 65536;
        f_wt = malloc(f_cap * sizeof(FDrop));
        fd_w = sw / F_CELL + 2;
        fd_h = sh / F_CELL + 2;
        f_dens = calloc(fd_w * fd_h, sizeof(int));
        f_grid = malloc(fd_w * fd_h * sizeof(int));

        ff_w = sw / F_SCALE + 1;
        ff_h = sh / F_SCALE + 1;
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
        f_cnt = 0;
    }

    Vector2 m = GetMousePosition();
    if (*wpn == 9 && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        *sfl = 0.45f;
        int limit = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) ? 1 : 18;
        for (int spw = 0; spw < limit; spw++) {
            float sx = m.x + (limit == 1 ? 0.0f : (float)(rand() % 60 - 30));
            float sy = m.y + (limit == 1 ? 0.0f : (float)(rand() % 30 - 15));
            float svx = (limit == 1 ? 0.0f : (float)(rand() % 240 - 120));
            float svy = 80.0f + (float)(rand() % 80);

            bool reused = false;
            for (int k = 0; k < f_cnt; k++) {
                if (!f_wt[k].on) {
                    f_wt[k] = (FDrop){ sx, sy, svx, svy, 1.0f, BASE_RAD, true, -1 };
                    reused = true;
                    break;
                }
            }
            if (!reused) {
                if (f_cnt >= f_cap) {
                    while (f_jobs != 0) Sleep(1);
                    f_cap *= 2;
                    f_wt = realloc(f_wt, f_cap * sizeof(FDrop));
                }
                f_wt[f_cnt++] = (FDrop){ sx, sy, svx, svy, 1.0f, BASE_RAD, true, -1 };
            }
        }
    }

    if (f_jobs == 0) {
        for (int i = 0; i < fd_w * fd_h; i++) { f_dens[i] = 0; f_grid[i] = -1; }
        for (int i = 0; i < ff_w * ff_h; i++) f_field[i] = 0;

        int total = f_cnt;
        for (int i = 0; i < total; i++) {
            if (!f_wt[i].on) continue;
            int gx = (int)(f_wt[i].x / F_CELL);
            int gy = (int)(f_wt[i].y / F_CELL);
            if (gx >= 0 && gx < fd_w && gy >= 0 && gy < fd_h) {
                int c = gy * fd_w + gx;
                f_wt[i].next = f_grid[c];
                f_grid[c] = i;
            }
        }

        // Слияние капель при касании (ограничено MAX_MASS = 12, чтобы не становились гигантскими)
        for (int i = 0; i < total; i++) {
            if (!f_wt[i].on || f_wt[i].mass >= MAX_MASS) continue;
            FDrop *a = &f_wt[i];
            int gx = (int)(a->x / F_CELL);
            int gy = (int)(a->y / F_CELL);

            for (int oy = -1; oy <= 1; oy++) {
                int ny = gy + oy;
                if (ny < 0 || ny >= fd_h) continue;
                for (int ox = -1; ox <= 1; ox++) {
                    int nx = gx + ox;
                    if (nx < 0 || nx >= fd_w) continue;
                    for (int j = f_grid[ny * fd_w + nx]; j != -1; j = f_wt[j].next) {
                        if (j <= i || !f_wt[j].on) continue;
                        FDrop *b = &f_wt[j];
                        if (a->mass + b->mass > MAX_MASS) continue;

                        float dx = b->x - a->x;
                        float dy = b->y - a->y;
                        float sum_r = (a->rad + b->rad) * 0.8f;
                        if (dx*dx + dy*dy < sum_r * sum_r) {
                            float new_mass = a->mass + b->mass;
                            a->x = (a->x * a->mass + b->x * b->mass) / new_mass;
                            a->y = (a->y * a->mass + b->y * b->mass) / new_mass;
                            a->vx = (a->vx * a->mass + b->vx * b->mass) / new_mass;
                            a->vy = (a->vy * a->mass + b->vy * b->mass) / new_mass;
                            a->mass = new_mass;
                            a->rad = BASE_RAD * (float)sqrt(new_mass);
                            b->on = false;
                        }
                    }
                }
            }
        }

        for (int i = 0; i < total; i++) {
            if (!f_wt[i].on) continue;
            FDrop *d = &f_wt[i];

int gx = (int)(d->x / F_CELL);
int gy = (int)(d->y / F_CELL);
if (gx >= 0 && gx < fd_w && gy >= 0 && gy < fd_h) {
f_dens[gy * fd_w + gx] += (int)d->mass;
}
int cx = (int)(d->x / F_SCALE);
int cy = (int)(d->y / F_SCALE);
int ir = (int)(d->rad / F_SCALE) + 1;
if (ir > 7) ir = 7;
int ir2 = ir * ir;
for (int dy = -ir; dy <= ir; dy++) {
int py = cy + dy;
if (py < 0 || py >= ff_h) continue;
int row = py * ff_w;
for (int dx = -ir; dx <= ir; dx++) {
int px = cx + dx;
if (px < 0 || px >= ff_w) continue;
int d2 = dx * dx + dy * dy;
if (d2 <= ir2) {
int v = f_field[row + px] + ((ir2 - d2 + 1) * 185) / (ir2 + 1);
f_field[row + px] = v > 255 ? 255 : (unsigned char)v;
}
}
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
DrawTextureEx(f_wtx, (Vector2){0, 0}, 0.0f, (float)F_SCALE, WHITE);
(EndShaderMode)();
}
#endif
