#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <unordered_map>

#include <Mandelbrot.h>
#include <../constants.h>
#include <vector>

using namespace std;

double scale(double, double, double, double, double);

int main()
{
    SDL_Renderer *renderer;
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *w;
    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN, &w, &renderer);
    SDL_SetWindowTitle(w, "Test");

    TTF_Init();
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12);
    SDL_Color White = { 255, 255, 255};

    if (!font)
    {
        printf("%s", TTF_GetError());
        return 0;
    }

    Mandelbrot m;

    enum Style {
        Banded, Smooth, STYLE_SIZE
    };

    enum Palette {
        Linear, Pre1, Pre2, Pre3, Pre4, Pre5, PALETTE_SIZE
    };

    string StyleNames[] = { "Banded", "Smooth", "NULL" };
    string PaletteNames[] = { "Linear", "Ultra Fractal", "Fire", "Rainbow", "Mint", "Frost", "NULL" };

    const int gradientScale = 256;
    vector<float> stops[] { { 0, 0.16, 0.42, 0.6425, 0.8575, 1 },
                            { 0, .33, .66, 1 },
                            { 0, .2, .4, .6, 1 },
                            { 0, .33, .66, 1 },
                            { 0, .33, .66, 1 } };
    vector<SDL_Color> stopcols[] = { { { 0, 7, 100 }, { 32, 107, 203 }, { 237, 255, 255 }, { 255, 170, 0 }, { 0, 2, 0 } },
                                     { { 255, 0, 0 }, { 255, 255, 0 }, { 128, 0, 0 } },
                                     { { 255, 0, 0 }, { 255, 255, 0 }, { 0, 255, 0 }, { 0, 0, 255 } },
                                     { { 0, 32, 0 }, { 32, 192, 32 }, { 64, 255, 96 } },
                                     { { 0, 64, 128 }, { 64, 192, 255 }, { 255, 255, 255 } } };
    vector<SDL_Color*> gradient;

    SDL_Color linearColor = { 255, 0, 0 };

    for (int p = 0; p < PALETTE_SIZE - 1; p++)
    {
        gradient.push_back(new SDL_Color[gradientScale]);
        for (int i = 0, n = 0; i < gradientScale; i++)
        {
            float f = (float)i / gradientScale;
            if (f > stops[p][n + 1]) n++;
            gradient[p][i] = {
                scale(f, stops[p][n], stops[p][n + 1], stopcols[p][n].r, stopcols[p][(n + 1) % stopcols[p].size()].r),
                scale(f, stops[p][n], stops[p][n + 1], stopcols[p][n].g, stopcols[p][(n + 1) % stopcols[p].size()].g),
                scale(f, stops[p][n], stops[p][n + 1], stopcols[p][n].b, stopcols[p][(n + 1) % stopcols[p].size()].b)
            };
        }
    }

    unordered_map<SDL_Keycode, bool> keys;
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_RenderClear(renderer);
    SDL_RenderPresent(renderer);

    int lx = -1, ly = -1;
    int mx = 0, my = 0;
    int lastiter = m.iter;
    Style style = Banded;
    Palette palette = Linear;
    double *vals = new double[SCREEN_HEIGHT * SCREEN_WIDTH]();

    bool redraw = false;
    bool recalc = true;
    long double loffx = 0, loffy = 0;
    SDL_Event e;
    while (true)
    {
        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT) return 0;
            else if (e.type == SDL_KEYDOWN && !keys[e.key.keysym.sym])
            {
                keys[e.key.keysym.sym] = true;
                switch (e.key.keysym.sym)
                {
                    case SDLK_ESCAPE: return 0;
                    case SDLK_r: recalc = true; break;
                    case SDLK_t: redraw = true; break;
                    case SDLK_s: style = (Style)((style + 1) % STYLE_SIZE);
                        redraw = true;
                        break;
                    case SDLK_a: palette = (Palette)((palette + 1) % PALETTE_SIZE);
                        redraw = true;
                        break;
                }
            }
            else if (e.type == SDL_KEYUP && keys[e.key.keysym.sym]) keys[e.key.keysym.sym] = false;
            else if (e.type == SDL_MOUSEMOTION)
            {
                if (keys[e.button.button])
                {
                    m.offx = loffx + (lx - mx) * m.zoom;
                    m.offy = loffy + (ly - my) * m.zoom;
                    //loffx = m.offx;
                   //loffy = m.offy;
                }
                mx = e.button.x;
                my = e.button.y;
                //redraw = true;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && !keys[e.button.button])
            {
                lx = mx;
                ly = my;
                keys[e.button.button] = true;
            }
            else if (e.type == SDL_MOUSEBUTTONUP)
            {
                loffx = m.offx;
                loffy = m.offy;
                keys[e.button.button] = false;
            }
            else if (e.type == SDL_MOUSEWHEEL)
            {
                if (e.wheel.y > 0)
                {
                    loffx = m.offx = loffx + (mx * m.zoom) - ((SCREEN_WIDTH / 2.0) * m.zoom);
                    loffy = m.offy = loffy + (my * m.zoom) - ((SCREEN_HEIGHT / 2.0) * m.zoom);
                    m.zoom /= 1.1 * e.wheel.y;
                    SDL_WarpMouseInWindow(w, SCREEN_WIDTH / 2.0, SCREEN_HEIGHT / 2.0);
                    recalc = true;
                }
                else if (e.wheel.y < 0)
                {
                    m.zoom *= -1.1 * e.wheel.y;
                    //loffx = m.offx = loffx + (lx - mx) * m.zoom;
                    //loffy = m.offy = loffy + (ly - my) * m.zoom;
                    recalc = true;
                }
            }

            if (keys[SDLK_KP_PLUS] && e.key.keysym.sym == SDLK_KP_PLUS)
            {
                if (keys[SDLK_i]) m.iter++;
                if (keys[SDLK_KP_1]) linearColor.r = min(255, linearColor.r + 1);
                if (keys[SDLK_KP_2]) linearColor.g = min(255, linearColor.g + 1);
                if (keys[SDLK_KP_3]) linearColor.b = min(255, linearColor.b + 1);

                if (m.iter > MAX_ITER) m.iter = MAX_ITER;
            }

            if (keys[SDLK_KP_MINUS] && e.key.keysym.sym == SDLK_KP_MINUS)
            {
                if (keys[SDLK_i]) m.iter--;
                if (keys[SDLK_KP_1]) linearColor.r = max(0, linearColor.r - 1);
                if (keys[SDLK_KP_2]) linearColor.g = max(0, linearColor.g - 1);
                if (keys[SDLK_KP_3]) linearColor.b = max(0, linearColor.b - 1);

                if (m.iter < 2) m.iter = 2;
            }

            if (m.iter != lastiter)
            {
                recalc = true;
                lastiter = m.iter;
            }
        }

        if (recalc)
        {
            m.Update(vals);
            redraw = true;
        }

        if (redraw)
        {
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            for (int y = 0; y < SCREEN_HEIGHT; y++)
            {
                for (int x = 0; x < SCREEN_WIDTH; x++)
                {
                    double n = vals[y * SCREEN_WIDTH + x];
                    if (n < 0) continue;
                    int k = (int)n;
                    if (style == Banded)
                    {
                        switch (palette)
                        {
                            case Linear:
                                SDL_SetRenderDrawColor(renderer,
                                    scale(k, 0, m.iter, 0, linearColor.r),
                                    scale(k, 0, m.iter, 0, linearColor.g),
                                    scale(k, 0, m.iter, 0, linearColor.b), 255);
                                break;
                            default:
                                SDL_SetRenderDrawColor(renderer,
                                    gradient[palette - 1][k % gradientScale].r,
                                    gradient[palette - 1][k % gradientScale].g,
                                    gradient[palette - 1][k % gradientScale].b, 255);
                                break;
                        }
                    }
                    else if (style == Smooth)
                    {
                        switch (palette)
                        {
                            case Linear:
                            {
                                SDL_Color a = { scale(k, 0, m.iter, 0, linearColor.r),
                                    scale(k, 0, m.iter, 0, linearColor.g),
                                    scale(k, 0, m.iter, 0, linearColor.b) };
                                SDL_Color b = { scale(k + 1, 0, m.iter, 0, linearColor.r),
                                    scale(k + 1, 0, m.iter, 0, linearColor.g),
                                    scale(k + 1, 0, m.iter, 0, linearColor.b) };
                                SDL_SetRenderDrawColor(renderer,
                                    scale(n - (int)n, 0, 1, a.r, b.r),
                                    scale(n - (int)n, 0, 1, a.g, b.g),
                                    scale(n - (int)n, 0, 1, a.b, b.b) , 255);
                                break;
                            }
                            default:
                                SDL_SetRenderDrawColor(renderer,
                                    scale(n - (int)n, 0, 1, gradient[palette - 1][k % gradientScale].r, gradient[palette - 1][(k + 1) % gradientScale].r),
                                    scale(n - (int)n, 0, 1, gradient[palette - 1][k % gradientScale].g, gradient[palette - 1][(k + 1) % gradientScale].g),
                                    scale(n - (int)n, 0, 1, gradient[palette - 1][k % gradientScale].b, gradient[palette - 1][(k + 1) % gradientScale].b), 255);
                                break;
                        }
                    }
                    SDL_RenderDrawPoint(renderer, x, y);
                }
            }

            string s = "Palette: " + PaletteNames[palette] + "\n" +
                        "Style: " + StyleNames[style] + "\n" +
                        "Iters: " + to_string(m.iter);

            SDL_Surface* txt = TTF_RenderText_Blended_Wrapped(font, s.c_str(), White, 500);
            SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, txt);
            SDL_Rect r; r.x = 10; r.y = 5;
            SDL_QueryTexture(tex, NULL, NULL, &r.w, &r.h);
            SDL_RenderCopy(renderer, tex, NULL, &r);
            SDL_FreeSurface(txt);
            if (palette == Linear)
            {
                s = "{ " + to_string(linearColor.r) + ", " + to_string(linearColor.g) + ", " + to_string(linearColor.b) + " }";
                txt = TTF_RenderText_Blended_Wrapped(font, s.c_str(), linearColor, 500);
                tex = SDL_CreateTextureFromSurface(renderer, txt);
                SDL_Rect r; r.x = 120; r.y = 5;
                SDL_QueryTexture(tex, NULL, NULL, &r.w, &r.h);
                SDL_RenderCopy(renderer, tex, NULL, &r);
                SDL_FreeSurface(txt);
            }
            else
            {
                for (int i = 0; i < gradientScale; i++)
                {
                    SDL_SetRenderDrawColor(renderer, gradient[palette - 1][i].r, gradient[palette - 1][i].g, gradient[palette - 1][i].b, 255);
                    SDL_RenderDrawLine(renderer, 10, r.h + i + 5, 20, r.h + i + 5);
                }
            }

            SDL_RenderPresent(renderer);
            redraw = false;
        }
    }
}

double scale(double v, double vl, double vh, double nl, double nh)
{
    return nl + (nh - nl) * (v - vl) / (vh - vl);
}
