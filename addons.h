#ifndef ADDONS_H
#define ADDONS_H

extern double sqrt(double x); extern void Sleep(unsigned int ms);
extern void* FindWindowA(const char *c, const char *w); extern int PostMessageA(void* h, unsigned int m, unsigned long w, long l);
extern void* CreateThread(void *a, unsigned long s, unsigned int (*f)(void*), void *p, unsigned int fl, unsigned int *id);
Texture2D grab_scr(int w, int h); void sp(float x, float y, float vx, float vy, float l, float sz, Color c, int k); void boom(float ex, float ey, float rad, float pwr, bool nk);
typedef struct { Rectangle src; float x, y, vx, vy, ang, vang, burn; bool brk, eaten; } ATile;
typedef struct { float x, y, vx, vy; bool on; } Meteor;
typedef struct { float x, y, vx, vy; bool on; int next; } Drop;
#define MW 10000
#define WC 14
#define WS 3
static bool ui_hide = 0, show_fps = 0; static volatile int w_jobs = 0; static volatile bool w_clr = 0, g_lmb = 0;
static float bh_tm = 0, ad_cd = 0, rl_fl = 0, g_dt = 0.016f, g_mx = 0, g_my = 0, g_wht = 0, g_whx = 0, g_why = 0;
static Vector2 rl_a = {0,0}, rl_b = {0,0}; static Meteor *mt = 0; static Drop *wt = 0; static ATile *g_tl = 0;
static int *wg = 0, wgc = 0, wgr = 0, mww = 0, mwh = 0, g_sw = 0, g_sh = 0, g_wpn = 1, g_tc = 0;
static unsigned char *wf = 0; static Texture2D wtx = {0}; static Shader w_shd = {0};
static const char *w_fs = "#version 330\nin vec2 fragTexCoord;\nuniform sampler2D texture0;\nuniform vec2 texel;\nout vec4 finalColor;\nvoid main(){\n float d=texture(texture0,fragTexCoord).r;if(d<0.28)discard;\n float dx=texture(texture0,fragTexCoord+vec2(texel.x,0)).r-texture(texture0,fragTexCoord-vec2(texel.x,0)).r;\n float dy=texture(texture0,fragTexCoord+vec2(0,texel.y)).r-texture(texture0,fragTexCoord-vec2(0,texel.y)).r;\n vec3 n=normalize(vec3(-dx*4.5,-dy*4.5,1.0));float sp=pow(max(dot(n,normalize(vec3(-0.4,-0.7,0.6))),0.0),24.0);\n float ed=smoothstep(0.28,0.42,d)*(1.0-smoothstep(0.45,0.75,d));float fr=clamp(length(vec2(dx,dy))*3.2,0.0,1.0);\n finalColor=vec4(vec3(0.82,0.93,1.0)*(0.25+fr*0.55)+vec3(sp*1.1+ed*0.45),clamp(0.14+fr*0.42+sp*0.75+ed*0.30,0.0,0.88));\n}\n";
static unsigned int __stdcall WaterWorker(void *param) {
    int tid = (int)(size_t)param, ch = MW/3 + 1, s = tid*ch, e = s + ch > MW ? MW : s + ch;
    while (1) {
        if ((w_jobs & (1 << tid)) == 0) { Sleep(1); continue; }
        float dt = g_dt; int sw = g_sw, sh = g_sh, tc = sw/42 + 1, tr = sh/42 + 1;
        for (int i = s; i < e; i++) if (wt[i].on) {
            Drop *d = &wt[i];
            if (g_wpn == 4 && g_lmb && g_wht <= 0) { float dx = g_mx-d->x, dy = g_my-d->y, r2 = dx*dx+dy*dy; if (r2 < 900) { d->on = 0; continue; } if (r2 < 102400) { float iv = 1.0f/((float)sqrt(r2)+1); d->vx += (dx*iv*2200 - dy*iv*1300)*dt; d->vy += (dy*iv*2200 + dx*iv*1300)*dt; } }
            else if (g_wht > 0) { float dx = d->x-g_whx, dy = d->y-g_why, r2 = dx*dx+dy*dy; if (r2 < 144400) { float iv = 1.0f/((float)sqrt(r2)+1); d->vx += dx*iv*2900*dt; d->vy += dy*iv*2900*dt; } }
            int gx = (int)(d->x/WC), gy = (int)(d->y/WC);
            for (int oy = -1; oy <= 1; oy++) if (gy+oy>=0 && gy+oy<wgr) for (int ox = -1; ox <= 1; ox++) if (gx+ox>=0 && gx+ox<wgc)
                for (int j = wg[(gy+oy)*wgc+gx+ox]; j != -1; j = wt[j].next) if (i != j) { float dx = d->x-wt[j].x, dy = d->y-wt[j].y, r2 = dx*dx+dy*dy; if (r2 < 196 && r2 > 0.01f) { float pr = (1 - r2*0.0051f)*85*dt; d->vx += dx*pr+(wt[j].vx-d->vx)*0.06f; d->vy += dy*pr+(wt[j].vy-d->vy)*0.06f; } }
            d->vy += 1080*dt; d->vx *= 0.993f; d->vy *= 0.993f; d->x += d->vx*dt; d->y += d->vy*dt;
            if (d->x < 5) { d->x = 5; d->vx *= -0.35f; } if (d->x > sw-5) { d->x = sw-5; d->vx *= -0.35f; } if (d->y > sh-5) { d->y = sh-5; d->vy *= -0.20f; d->vx *= 0.90f; }
            int tx = (int)(d->x/42), ty = (int)(d->y/42);
            if (tx>=0&&tx<tc&&ty>=0&&ty<tr && ty*tc+tx < g_tc && !g_tl[ty*tc+tx].eaten) { ATile *t = &g_tl[ty*tc+tx]; if (t->burn > 0) t->burn = t->burn > dt*2 ? t->burn-dt*2 : 0; if (t->brk) { t->vx += d->vx*0.06f*dt; t->vy += d->vy*0.06f*dt; } }
        }
        w_jobs &= ~(1 << tid);
    }
    return 0;
}
static void AddonEndDrawing(Texture2D *tex, int sw, int sh, void (*rst)(void), int *grb, int *wpn, void *tlv, int tc, int *bhm, float *wht, float *whx, float *why, float *shk, float *nfls, float *sls, float *sfl) {
    ATile *tl = (ATile *)tlv;
    if (!mt) {
        mt = calloc(16, sizeof(Meteor)); wt = calloc(MW, sizeof(Drop)); wgc = sw/WC+2; wgr = sh/WC+2; wg = malloc(wgc*wgr*4); mww = sw/WS+1; mwh = sh/WS+1; wf = calloc(mww*mwh, 1);
        wtx = LoadTextureFromImage((Image){wf, mww, mwh, 1, PIXELFORMAT_UNCOMPRESSED_GRAYSCALE}); SetTextureFilter(wtx, TEXTURE_FILTER_BILINEAR); w_shd = LoadShaderFromMemory(0, w_fs);
        Vector2 txl = {1.0f/mww, 1.0f/mwh}; SetShaderValue(w_shd, GetShaderLocation(w_shd, "texel"), &txl, SHADER_UNIFORM_VEC2);
        g_sw = sw; g_sh = sh; g_tc = tc; g_tl = tl; for (int t = 0; t < 3; t++) CreateThread(0, 0, WaterWorker, (void*)(size_t)t, 0, 0);
    }
    float dt = GetFrameTime(); if (dt > 0.033f) dt = 0.033f; if (ad_cd > 0) ad_cd -= dt; if (rl_fl > 0) rl_fl -= dt*3.5f;
    if (IsKeyPressed(KEY_F12)) show_fps = !show_fps;
    bool alt = IsKeyDown(KEY_LEFT_ALT) || IsKeyDown(KEY_RIGHT_ALT);
    if (alt && IsKeyPressed(KEY_U)) { ui_hide = !ui_hide; if (ui_hide) ShowCursor(); else HideCursor(); }
    if (alt && IsKeyPressed(KEY_D)) { ClearWindowState(FLAG_WINDOW_TOPMOST); SetWindowState(FLAG_WINDOW_HIDDEN); Sleep(100); void *tr = FindWindowA("Shell_TrayWnd", 0); if (tr) PostMessageA(tr, 0x0111, 419, 0); Sleep(400); UnloadTexture(*tex); *tex = grab_scr(sw, sh); ClearWindowState(FLAG_WINDOW_HIDDEN | FLAG_WINDOW_MINIMIZED); SetWindowState(FLAG_WINDOW_TOPMOST); SetWindowSize(sw, sh); SetWindowPosition(0, 0); SetWindowFocused(); rst(); *grb = -1; bh_tm = 0; w_clr = 1; }
    if (IsKeyPressed(KEY_R)) { bh_tm = 0; rl_fl = 0; for (int i = 0; i < 16; i++) mt[i].on = 0; w_clr = 1; }
    if (*bhm > 0 && *wht <= 0) { if ((bh_tm += dt) >= 20.0f) { Vector2 m = GetMousePosition(); *wht = 1; *whx = m.x; *why = m.y; *nfls = 0.9f; bh_tm = 0; sp(*whx, *why, 0, 0, 0.55f, 480, WHITE, 3); } } else if (*bhm == 0) bh_tm = 0;
    if (IsKeyPressed(KEY_SEVEN)) *wpn = 7; if (IsKeyPressed(KEY_EIGHT)) *wpn = 8; if (IsKeyPressed(KEY_NINE)) *wpn = 9;
    Vector2 m = GetMousePosition();
    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        if (*wpn == 7 && ad_cd <= 0) { ad_cd = 0.45f; *shk = 18; *sls = 1; rl_fl = 1; rl_a = (Vector2){sw*0.5f, (float)sh}; float dx = m.x-rl_a.x, dy = m.y-rl_a.y, ln = (float)sqrt(dx*dx+dy*dy)+1, ux = dx/ln, uy = dy/ln; rl_b = (Vector2){rl_a.x+ux*2500, rl_a.y+uy*2500}; for (int s = 0; s < sw+sh; s += 20) { float px = rl_a.x+ux*s, py = rl_a.y+uy*s; if (px<0||px>sw||py<0||py>sh) break; sp(px, py, (float)(rand()%100-50), (float)(rand()%100-50), 0.4f, 8, SKYBLUE, 0); for (int i = 0; i < tc; i++) if (!tl[i].eaten && (tl[i].x-px)*(tl[i].x-px)+(tl[i].y-py)*(tl[i].y-py) < 2200) { tl[i].brk = 1; tl[i].burn += 0.4f; tl[i].vx += ux*400+rand()%300-150; tl[i].vy += uy*400-200; tl[i].vang += rand()%600-300; } } }
        else if (*wpn == 8 && ad_cd <= 0) { ad_cd = 0.12f; for (int i = 0; i < 16; i++) if (!mt[i].on) { mt[i] = (Meteor){m.x+(rand()%500-250)-300, -50, 380, 850+(float)(rand()%300), 1}; break; } }
        else if (*wpn == 9 && w_jobs == 0) { *sfl = 0.45f; for (int i = 0, spw = 0; i < MW && spw < 45; i++) if (!wt[i].on) { wt[i] = (Drop){m.x+rand()%50-25, m.y+rand()%50-25, (float)(rand()%240-120), 100+(float)(rand()%120), 1, -1}; spw++; } }
    }
    int act_w = 0;
    if (w_jobs == 0) {
        if (w_clr) { for (int i = 0; i < MW; i++) wt[i].on = 0; w_clr = 0; }
        for (int i = 0; i < mww*mwh; i++) wf[i] = 0; for (int i = 0; i < wgc*wgr; i++) wg[i] = -1;
        for (int i = 0; i < MW; i++) if (wt[i].on) { act_w++; int gx = (int)(wt[i].x/WC), gy = (int)(wt[i].y/WC); if (gx>=0&&gx<wgc&&gy>=0&&gy<wgr) { int c = gy*wgc+gx; wt[i].next = wg[c]; wg[c] = i; } int cx = (int)(wt[i].x/WS), cy = (int)(wt[i].y/WS); for (int dy = -2; dy <= 2; dy++) if (cy+dy>=0 && cy+dy<mwh) for (int dx = -2; dx <= 2; dx++) if (cx+dx>=0 && cx+dx<mww) { int d2 = dx*dx+dy*dy; if (d2 <= 5) { int idx = (cy+dy)*mww+cx+dx, v = wf[idx]+(6-d2)*16; wf[idx] = v>255?255:(unsigned char)v; } } }
        UpdateTexture(wtx, wf); g_dt = dt; g_wpn = *wpn; g_lmb = IsMouseButtonDown(MOUSE_LEFT_BUTTON); g_mx = m.x; g_my = m.y; g_wht = *wht; g_whx = *whx; g_why = *why; w_jobs = 7;
    }
    BeginShaderMode(w_shd); DrawTextureEx(wtx, (Vector2){0, 0}, 0, (float)WS, WHITE); EndShaderMode();
    if (rl_fl > 0) { DrawLineEx(rl_a, rl_b, 24*rl_fl, Fade(SKYBLUE, rl_fl*0.6f)); DrawLineEx(rl_a, rl_b, 8*rl_fl, Fade(WHITE, rl_fl)); }
    for (int i = 0; i < 16; i++) if (mt[i].on) { mt[i].x += mt[i].vx*dt; mt[i].y += mt[i].vy*dt; sp(mt[i].x, mt[i].y, -mt[i].vx*0.2f+rand()%60-30, -mt[i].vy*0.2f, 0.35f, 14, ORANGE, 1); DrawCircle((int)mt[i].x, (int)mt[i].y, 16, (Color){90,45,30,255}); DrawCircleLines((int)mt[i].x, (int)mt[i].y, 17, YELLOW); if (mt[i].y >= m.y || mt[i].y >= sh-25) { mt[i].on = false; *shk = 16; boom(mt[i].x, mt[i].y, 160, 750, 0); } }
    if (show_fps) { DrawRectangle(12, 12, 165, 28, Fade(BLACK, 0.8f)); DrawText(TextFormat("FPS: %d | W: %d", GetFPS(), act_w), 20, 18, 16, LIME); }
    if (*bhm > 0 && *wht <= 0 && !ui_hide) DrawText(TextFormat("OVERLOAD: %.1fs", 20-bh_tm>0?20-bh_tm:0), (int)m.x-45, (int)m.y-55, 16, bh_tm>15?RED:VIOLET);
    if (ui_hide) DrawTextureRec(*tex, (Rectangle){(sw-720)/2.f, 10, 720, 32}, (Vector2){(sw-720)/2.f, 10}, WHITE);
    else { DrawRectangle((sw-880)/2, 10, 880, 32, Fade(BLACK, 0.88f)); DrawText("1:Gun 2:Flame 3:Laser 4:Hole(20s) 5:Gren 6:Nuke 7:Rail 8:Meteor 9:Water | Alt+U/D F12:FPS R:Fix", (sw-880)/2+12, 18, 14, WHITE); }
    (EndDrawing)();
}
#define EndDrawing() AddonEndDrawing(&tex, sw, sh, reset_tl, &grab, &wpn, tl, tc, &bh_mass, &wh_timer, &wh_x, &wh_y, &shk, &nfls, &s_ls, &s_fl)
#endif
