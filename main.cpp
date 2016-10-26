#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <defines.h>

using namespace std;

#ifdef CLIENT

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <unordered_map>

#include <curl/curl.h>

#include <vector>
#include <sstream>
#include <iomanip>
#include <string>

double scale(double v, double vl, double vh, double nl, double nh)
{
    return nl + (nh - nl) * (v - vl) / (vh - vl);
}

SDL_Texture *RenderFromText(SDL_Renderer *r, SDL_Rect *rect, string t, TTF_Font *f, SDL_Color c, int wrap)
{

    SDL_Surface *s = TTF_RenderText_Blended_Wrapped(f, t.c_str(), c, wrap);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(r, s);
    if (tex == NULL) return NULL;
    //rect->x = 0; rect->y = 0;
    rect->w = s->w;
    rect->h = s->h;
    //SDL_QueryTexture(tex, NULL, NULL, &rect->w, &rect->h);
    SDL_FreeSurface(s);
    return tex;
}

#elif defined(MASTER)

#include <algorithm>

#endif

int SendText(int s, char* msg)
{
    Message reply;
    reply.type = Text;
    reply.len = (int)strlen(msg);
    //printf("Sending header: %d\n", (int)sizeof(reply));
    //printf("Sending msg: %d\n", reply.len);
    int s1 = send(s, &reply, sizeof(reply), 0);
    if (s1 != sizeof(reply)) return -1;
    int s2 = send(s, msg, reply.len, 0);
    if (s2 != reply.len) return -2;
    return s1 + s2;
}

int main()
{
    Message m;
    memset(&m, 0, sizeof(m));
    int sock;
    int numclients = 0;
    bool run = true;

    #ifdef MASTER

    struct sockaddr_in addr;
    int clients[MAXCLIENTS];
    char cltypes[MAXCLIENTS];
    fd_set fds;

    bool recalc = false;
    bool queuerecalc = false;
    vector<int> needrecalc;

    for (int i = 0; i < MAXCLIENTS; i++) clients[i] = 0;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        printf("Could not create socket: %d\n", errno);
        return -1;
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int)) < 0)
    {
        printf("Could not set socket options: %d\n", errno);
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(HOSTPORT);

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        printf("Could not bind port: %d\n", errno);
        return -1;
    }

    printf("Listening on port %d\n", HOSTPORT);

    if (listen(sock, 5) < 0)
    {
        printf("Could not listen: %d\n", errno);
        return -1;
    }

    int addrlen = sizeof(addr);
    printf("Waiting for connections\n");

    int valsreceived = 0;

    #else

    struct addrinfo hints, *serv;
    memset(&hints, 0, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    int ret = getaddrinfo(HOSTADDR, HOSTPORT, &hints, &serv);
    if (ret != 0)
    {
        printf("Could not get address info: %s\n", gai_strerror(ret));
        return -1;
    }

    struct addrinfo *i;
    for (int r = 0; r <= RETRYATTEMPTS; r++)
    {
        for (i = serv; i != NULL; i = i->ai_next)
        {
            if ((sock = socket(i->ai_family, i->ai_socktype, i->ai_protocol)) == -1)
            {
                printf("Could not create socket: %d\n", errno);
                continue;
            }

            if (connect(sock, i->ai_addr, i->ai_addrlen) < 0)
            {
                close(sock);
                printf("Could not connect to host: %d\n", errno);
                continue;
            }

            break;  //successful connection, don't try to connect to more
        }

        if (i == NULL && r < RETRYATTEMPTS)
        {
            sleep(5);
            printf("Retrying, attempt %d of %d\n", r + 1, RETRYATTEMPTS);
        }
        else if (i == NULL && r == RETRYATTEMPTS)
        {
            printf("Could not connect.\n");
            return -1;
        }
        else break; //successful connection
    }

    char saddr[INET_ADDRSTRLEN];
    inet_ntop(i->ai_family, &((struct sockaddr_in*)((struct sockaddr*)i->ai_addr))->sin_addr, saddr, sizeof(saddr));
    printf("Connecting to %s\n", saddr);

    #ifdef CLIENT
    char cltype = 2;
    #else
    char cltype = 1;
    #endif
    send(sock, &cltype, sizeof(cltype), 0);

    freeaddrinfo(serv);

    #endif

    #ifdef CLIENT

    SDL_Renderer *renderer;
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *w;
    SDL_CreateWindowAndRenderer(INIT_SCREEN_WIDTH, INIT_SCREEN_HEIGHT, SDL_WINDOW_SHOWN, &w, &renderer);
    SDL_SetWindowTitle(w, "Test");

    TTF_Init();
    TTF_Font* font = TTF_OpenFont("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf", 12);
    SDL_Color White = { 255, 255, 255};

    if (!font)
    {
        printf("%s", TTF_GetError());
        return 0;
    }

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
    SDL_Color backColor = { 0, 0, 0 };

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
    int lastiter = INIT_ITER;
    int curiter = lastiter;
    Style style = Banded;
    Palette palette = Linear;
    short *vals = new short[INIT_SCREEN_HEIGHT * INIT_SCREEN_WIDTH]();

    bool redraw = false;
    bool recalc = true;
    bool changeBack = false;
    bool hideVals = false;
    bool help = false;
    long double loffx = 0L, loffy = 0L;
    long double curzoom = INIT_ZOOM;
    long double curoffx = 0L, curoffy = 0L;
    int resx = INIT_SCREEN_WIDTH, resy = INIT_SCREEN_HEIGHT;
    int valsreceived = 0;
    SDL_Event e;

    string savename = "";
    bool getsavename = false;

    #elif !defined(MASTER)

    Mandelbrot mandl;

    #endif

    while (run)
    {
        #ifdef MASTER

        FD_ZERO(&fds);

        FD_SET(sock, &fds);
        int ms = sock;
        for (int i = 0; i < MAXCLIENTS; i++)
        {
            int s = clients[i];
            if (s > 0) FD_SET(s, &fds);
            if (s > ms) ms = s;
        }

        int activity = select(ms + 1, &fds, NULL, NULL, NULL);

        if (activity < 0 && errno != EINTR) printf("'select' error\n");

        if (FD_ISSET(sock, &fds))
        {
            int newsock = accept(sock, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
            if (newsock < 0)
            {
                printf("Could not accept client\n");
                return -1;
            }

            printf("New connection from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

            char cltype;
            read(newsock, &cltype, sizeof(cltype));
            printf("Client is type: %d\n", cltype);

            if (SendText(newsock, "You have connected to the Master Pi!") < 0)
                printf("Could not greet %s\n", inet_ntoa(addr.sin_addr));
            else printf("Greeted %s\n", inet_ntoa(addr.sin_addr));

            for (int i = 0; i < MAXCLIENTS; i++)
            {
                if (clients[i] == 0)
                {
                    clients[i] = newsock;
                    printf("Client %d added to list\n", i);
                    if (cltype == 1)
                    {
                        numclients++;
                        for (int j = 0; j < MAXCLIENTS; j++)
                        {
                            if (clients[j] == 0) continue;
                            m.type = Connections;
                            m.len = numclients;
                            send(clients[j], &m, sizeof(m), 0);
                        }
                    }
                    else
                    {
                        m.type = Connections;
                        m.len = numclients;
                        send(newsock, &m, sizeof(m), 0);
                    }
                    cltypes[i] = cltype;
                    break;
                }
            }
        }

        for (int i = 0; i < MAXCLIENTS; i++)
        {
            int s = clients[i];
            if (FD_ISSET(s, &fds))
            {
                int val = read(s, &m, sizeof(m));
                getpeername(s, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
                if (val == 0)
                {
                    printf("Client %d (%s) disconnected\n", i, inet_ntoa(addr.sin_addr));
                    close(s);
                    clients[i] = 0;

                    vector<int>::iterator x;
                    if ((x = find(needrecalc.begin(), needrecalc.end(), i)) != needrecalc.end())
                        needrecalc.erase(x);
                    if (needrecalc.size() == 0) queuerecalc = false;

                    if (cltypes[i] == 1)
                    {
                        numclients--;
                        for (int j = 0; j < MAXCLIENTS; j++)
                        {
                            if (clients[j] == 0) continue;
                            m.type = Connections;
                            m.len = numclients;
                            send(clients[j], &m, sizeof(m), 0);
                        }
                    }
                    queuerecalc = false;
                    recalc = false;
                    cltypes[i] = 0;
                }
                else
                {
                    switch (m.type)
                    {
                        case MessageType::Recalc:
                        {
                            if (recalc && queuerecalc)
                            {
                                printf("Recalc already queued\n");
                                break;
                            }
                            else if (recalc && valsreceived < numclients)
                            {
                                printf("Queuing recalc\n");
                                queuerecalc = true;
                                for (int j = 0; j < MAXCLIENTS; j++)
                                    if (j == i || clients[j] == 0 || cltypes[j] != 1) continue;
                                    else needrecalc.push_back(j);
                                break;
                            }

                            if (numclients == 0)
                            {
                                printf("No suitable clients connected to calculate!\n");
                                break;
                            }
                            else printf("Begin rendering...\n");

                            int cl = 0;
                            valsreceived = 0;
                            for (int j = 0; j < MAXCLIENTS; j++)
                            {
                                if (j == i || clients[j] == 0 || cltypes[j] != 1) continue;
                                m.len = cl;
                                send(clients[j], &m, sizeof(m), 0);
                                printf("Client %d is rendering %d\n", j, cl);
                                cl++;
                            }
                            recalc = true;
                            break;
                        }
                        case MessageType::Connections:

                            break;
                        //just transfer these packets
                        case MessageType::OffX:
                        case MessageType::OffY:
                        case MessageType::Iter:
                        case MessageType::Zoom:
                        case MessageType::ResX:
                        case MessageType::ResY:
                        {
                            char* buf = new char[m.len];
                            read(s, buf, m.len);
                            for (int j = 0; j < MAXCLIENTS; j++)
                            {
                                if (j == i || clients[j] == 0 || cltypes[j] != 1) continue;
                                send(clients[j], &m, sizeof(m), 0);
                                send(clients[j], buf, m.len, 0);
                                printf("Relay message %d to client %d\n", m.type, j);
                            }
                            break;
                        }
                        case MessageType::Vals:
                        {
                            int cur;
                            read(s, &cur, sizeof(cur));
                            printf("Relaying vals from %d...\n", cur);
                            char *buf = new char[m.len]();
                            int ret = read(s, buf, m.len);
                            while (ret < m.len)
                            {
                                int tmp = read(s, &buf[ret], m.len - ret);
                                if (tmp < 0)
                                {
                                    printf("Err: %d\n", errno);
                                    break;
                                }
                                //printf("Read another %d bytes (%d / %d)\n", tmp, tmp + ret, m.len);
                                ret += tmp;
                            }
                            printf("Read %d / %d bytes\n", ret, m.len);
                            bool found = false;
                            for (int j = 0; j < MAXCLIENTS; j++)
                            {
                                if (j == i || clients[j] == 0 || cltypes[j] != 2) continue;
                                printf("Sending vals to render...\nHeader...");
                                send(clients[j], &m, sizeof(m), MSG_MORE);
                                printf("Done.\nPos...");
                                send(clients[j], &cur, sizeof(cur), MSG_MORE);
                                printf("Done.\nVals...\n");
                                int sent = 0;
                                while(sent < m.len)
                                {
                                    int n = send(clients[j], buf, m.len - sent, 0);
                                    if (n < 0)
                                    {
                                        printf("Send - Err: %d\n", errno);
                                        break;
                                    }
                                    printf("Sent another %d bytes...\n", n);
                                    sent += n;
                                }
                                printf("Done.\n");
                                found = true;
                                valsreceived++;
                                if (valsreceived == numclients) printf("Received all values\n");

                                if (queuerecalc)
                                {
                                    vector<int>::iterator x;
                                    if ((x = find(needrecalc.begin(), needrecalc.end(), i)) != needrecalc.end())
                                    {
                                        needrecalc.erase(x);
                                        m.type = Recalc;
                                        m.len = cur;
                                        send(cur, &m, sizeof(m), 0);
                                        printf("Client %d is re-rendering %d\n", i, cur);
                                    }
                                    if (needrecalc.size() == 0) queuerecalc = false;
                                }
                                break;
                            }
                            if (!found) printf("Could not find suitable render client.\n");
                            delete[] buf;
                            break;
                        }
                        case MessageType::Text:
                            //printf("Expecting string of length: %d\n", m.len);
                            char* buf = new char[m.len + 1]();
                            read(s, buf, m.len);
                            buf[m.len] = 0;
                            printf("%s: \"%s\"\n", inet_ntoa(addr.sin_addr), buf);
                            delete[] buf;
                            break;
                    }
                }
            }
        }

        #else

        ret = recv(sock, &m, sizeof(m), MSG_DONTWAIT);

        if (ret == 0)
        {
            printf("Server closed connection\n");
            break;
        }
        else if (ret > 0)
        {
            //printf("Received header type %d size %d\n", m.type, ret);
            switch (m.type)
            {
                case MessageType::Recalc:
                {
                    #ifdef CLIENT

                    #else
                    mandl.parallel_pos = m.len;
                    printf("Rendering with pos %d...", mandl.parallel_pos);
                    short *vals = new short[mandl.width * mandl.parallel_height];
                    mandl.Update(vals);
                    printf("Done\n");
                    Message smsg;
                    smsg.type = Vals;
                    smsg.len = (mandl.width * mandl.parallel_height) * sizeof(short);
                    printf("Sending vals to server (%d values, %d bytes)...\n", mandl.width * mandl.parallel_height, smsg.len);
                    printf("Header...");
                    send(sock, &smsg, sizeof(smsg), MSG_MORE);
                    printf("Done.\nPos...");
                    send(sock, &m.len, sizeof(m.len), MSG_MORE);
                    printf("Done.\nVals...\n");
                    int sent = 0;
                    while(sent < smsg.len)
                    {
                        int n = send(sock, vals, smsg.len - sent, 0);
                        if (n < 0)
                        {
                            printf("Send - Err: %d\n", errno);
                            break;
                        }
                        printf("Sent another %d bytes...\n", n);
                        sent += n;
                    }
                    printf("Done.\n");
                    delete[] vals;
                    #endif
                    break;
                }
                case MessageType::Connections:
                    numclients = m.len;
                    printf("Number of clients is now: %d\n", numclients);
                    #ifndef CLIENT
                    mandl.parallel_height = mandl.height / numclients;
                    #endif
                    break;
                case MessageType::OffX:
                {
                    #ifdef CLIENT

                    #else
                    printf("receive off x\n");
                    char* buf = new char[m.len];
                    read(sock, buf, m.len);
                    memcpy(&mandl.offx, buf, m.len);
                    printf("\tgot: %Lf\n", mandl.offx);
                    #endif
                    break;
                }
                case MessageType::OffY:
                {
                    #ifdef CLIENT

                    #else
                    printf("receive off y\n");
                    char* buf = new char[m.len];
                    read(sock, buf, m.len);
                    memcpy(&mandl.offy, buf, m.len);
                    printf("\tgot: %Lf\n", mandl.offy);
                    #endif
                    break;
                }
                case MessageType::Iter:
                {
                    #ifdef CLIENT

                    #else
                    printf("receive iter\n");
                    char* buf = new char[m.len];
                    read(sock, buf, m.len);
                    memcpy(&mandl.iter, buf, m.len);
                    printf("\tgot: %d\n", mandl.iter);
                    #endif
                    break;
                }
                case MessageType::Zoom:
                {
                    #ifdef CLIENT

                    #else
                    printf("receive zoom\n");
                    char* buf = new char[m.len];
                    read(sock, buf, m.len);
                    memcpy(&mandl.zoom, buf, m.len);
                    printf("\tgot: %Lf\n", mandl.zoom);
                    #endif
                    break;
                }
                case MessageType::ResX:
                {
                    #ifdef CLIENT

                    #else
                    printf("receive res x\n");
                    char* buf = new char[m.len];
                    read(sock, buf, m.len);
                    memcpy(&mandl.width, buf, m.len);
                    printf("\tgot: %d\n", mandl.width);
                    #endif
                    break;
                }
                case MessageType::ResY:
                {
                    #ifdef CLIENT

                    #else
                    printf("receive res y\n");
                    char* buf = new char[m.len];
                    read(sock, buf, m.len);
                    memcpy(&mandl.height, buf, m.len);
                    printf("\tgot: %d\n", mandl.height);
                    #endif
                    break;
                }
                case MessageType::Vals:
                {
                    #ifdef CLIENT
                    int pos;
                    read(sock, &pos, sizeof(pos));
                    printf("Receiving %d values (%d bytes) from pos %d...\n", m.len / sizeof(short), m.len, pos);
                    char* buf = new char[m.len]();
                    printf("Reading...\n");
                    ret = read(sock, buf, m.len);
                    while (ret < m.len)
                    {
                        int tmp = read(sock, &buf[ret], m.len - ret);
                        if (tmp < 0)
                        {
                            printf("Err: %d\n", errno);
                            break;
                        }
                        //printf("Read another %d bytes (%d / %d)\n", tmp, tmp + ret, m.len);
                        ret += tmp;
                    }
                    printf("Read %d / %d bytes\n", ret, m.len);
                    printf("Done.\nCopying values into full array... moving to %d...", pos * m.len);
                    memcpy(&vals[(pos * m.len) / sizeof(short)], buf, m.len);
                    printf("Done.\n");
                    valsreceived++;
                    if (valsreceived >= numclients)
                    {
                        printf("All values received\n");
                        redraw = true;
                    }
                    #else
                    printf("Pi received values... Pi should only send values!\n");
                    #endif
                    break;
                }
                case MessageType::Text:
                    //printf("Expecting string of length: %d\n", m.len);
                    char* buf = new char[m.len + 1]();
                    read(sock, buf, m.len);
                    buf[m.len] = 0;
                    //printf("Received %d\n", ret);
                    printf("Server: \"%s\"\n", buf);
                    delete[] buf;
                    //SendText(sock, "Received message from server");
                    break;
            }
        }

        #endif

        #ifdef CLIENT

        bool updatetext = false;

        while (SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT) return 0;
            else if (e.type == SDL_TEXTINPUT)
            {
                savename += e.text.text;
                updatetext = true;
            }
            else if (e.type == SDL_KEYDOWN && !keys[e.key.keysym.sym])
            {
                keys[e.key.keysym.sym] = true;
                if (!getsavename)
                {
                    switch (e.key.keysym.sym)
                    {
                        case SDLK_ESCAPE:
                        case SDLK_q: run = false; break;
                        case SDLK_r: recalc = true; break;
                        case SDLK_t: redraw = true; break;
                        case SDLK_b: changeBack = !changeBack; redraw = true; break;
                        case SDLK_s: style = (Style)((style + 1) % STYLE_SIZE);
                            redraw = true;
                            break;
                        case SDLK_h: help = !help; redraw = true; break;
                        case SDLK_F12: SDL_StartTextInput(); savename = ""; getsavename = true; break;
                        case SDLK_a: palette = (Palette)((palette + 1) % PALETTE_SIZE);
                            redraw = true;
                            break;
                        case SDLK_z: hideVals = !hideVals; redraw = true; break;
                    }
                }
                else
                {
                    if (e.key.keysym.sym == SDLK_ESCAPE)
                    {
                        SDL_StopTextInput();
                        getsavename = false;
                    }
                    else if (e.key.keysym.sym == SDLK_BACKSPACE)
                    {
                        if (savename.length() > 0)
                        {
                            savename.pop_back();
                            updatetext = true;
                        }
                    }
                    else if (e.key.keysym.sym == SDLK_RETURN || e.key.keysym.sym == SDLK_RETURN2)
                    {
                        //save the picture, upload the picture
                        SDL_StopTextInput();
                        getsavename = false;
                    }
                }

                if (!run) break;
            }
            else if (e.type == SDL_KEYUP && keys[e.key.keysym.sym]) keys[e.key.keysym.sym] = false;
            else if (e.type == SDL_MOUSEMOTION)
            {
                if (keys[e.button.button])
                {
                    //SEND THESE COORDINATES
                    Message sm;
                    sm.type = OffX;
                    sm.len = sizeof(long double);
                    curoffx = loffx + (lx - mx) * curzoom;
                    curoffy = loffy + (ly - my) * curzoom;

                    send(sock, &sm, sizeof(sm), 0);
                    send(sock, &curoffx, sm.len, 0);
                    printf("Sent off x %Lf\n", curoffx);
                    sm.type = OffY;
                    send(sock, &sm, sizeof(sm), 0);
                    send(sock, &curoffy, sm.len, 0);
                    printf("Sent off y %Lf\n", curoffy);
                    //m.offx = loffx + (lx - mx) * m.zoom;
                    //m.offy = loffy + (ly - my) * m.zoom;
                    recalc = true;
                }
                mx = e.button.x;
                my = e.button.y;
            }
            else if (e.type == SDL_MOUSEBUTTONDOWN && !keys[e.button.button])
            {
                lx = mx;
                ly = my;
                keys[e.button.button] = true;
            }
            else if (e.type == SDL_MOUSEBUTTONUP)
            {
                //RECEIVE THESE COORDINATES
                loffx = curoffx;
                loffy = curoffy;
                //loffx = m.offx;
                //loffy = m.offy;
                keys[e.button.button] = false;
            }
            else if (e.type == SDL_MOUSEWHEEL)
            {
                if (e.wheel.y > 0)
                {

                    //SEND THESE VALUES
                    loffx = curoffx = loffx + (mx * curzoom) - ((resx / 2.0L) * curzoom);
                    loffy = curoffy = loffy + (my * curzoom) - ((resy / 2.0L) * curzoom);
                    curzoom /= 1.1L * e.wheel.y;
                    //loffx = m.offx = loffx + (mx * m.zoom) - ((SCREEN_WIDTH / 2.0) * m.zoom);
                    //loffy = m.offy = loffy + (my * m.zoom) - ((SCREEN_HEIGHT / 2.0) * m.zoom);
                    //m.zoom /= 1.1 * e.wheel.y;
                    Message sm;
                    sm.type = OffX;
                    sm.len = sizeof(long double);
                    send(sock, &sm, sizeof(sm), 0);
                    send(sock, &curoffx, sm.len, 0);
                    printf("Sent off x %Lf\n", curoffx);
                    sm.type = OffY;
                    send(sock, &sm, sizeof(sm), 0);
                    send(sock, &curoffy, sm.len, 0);
                    printf("Sent off y %Lf\n", curoffy);
                    sm.type = Zoom;
                    send(sock, &sm, sizeof(sm), 0);
                    send(sock, &curzoom, sm.len, 0);
                    printf("Sent zoom %Lf\n", curzoom);
                    SDL_WarpMouseInWindow(w, resx / 2.0, resy / 2.0);
                    recalc = true;
                }
                else if (e.wheel.y < 0)
                {
                    //SEND THIS VALUE
                    curzoom *= -1.1L * e.wheel.y;
                    Message sm;
                    sm.type = Zoom;
                    sm.len = sizeof(long double);
                    send(sock, &sm, sizeof(sm), 0);
                    send(sock, &curzoom, m.len, 0);
                    printf("Sent zoom %Lf\n", curzoom);
                    //m.zoom *= -1.1 * e.wheel.y;
                    recalc = true;
                }
            }

            SDL_Color tmp = (changeBack ? backColor : linearColor);
            if (keys[SDLK_KP_PLUS] && e.key.keysym.sym == SDLK_KP_PLUS)
            {
                if (keys[SDLK_i]) curiter++;
                if (keys[SDLK_KP_1]) (changeBack ? backColor.r : linearColor.r) = min(255, (changeBack ? backColor.r : linearColor.r) + 1);
                if (keys[SDLK_KP_2]) (changeBack ? backColor.g : linearColor.g) = min(255, (changeBack ? backColor.g : linearColor.g) + 1);
                if (keys[SDLK_KP_3]) (changeBack ? backColor.b : linearColor.b) = min(255, (changeBack ? backColor.b : linearColor.b) + 1);

                if (curiter > MAX_ITER) curiter = MAX_ITER;
            }

            if (keys[SDLK_KP_MINUS] && e.key.keysym.sym == SDLK_KP_MINUS)
            {
                if (keys[SDLK_i]) curiter--;
                if (keys[SDLK_KP_1]) (changeBack ? backColor.r : linearColor.r) = max(0, (changeBack ? backColor.r : linearColor.r) - 1);
                if (keys[SDLK_KP_2]) (changeBack ? backColor.g : linearColor.g) = max(0, (changeBack ? backColor.g : linearColor.g) - 1);
                if (keys[SDLK_KP_3]) (changeBack ? backColor.b : linearColor.b) = max(0, (changeBack ? backColor.b : linearColor.b) - 1);

                if (curiter < 2) curiter = 2;
            }

            //SEND THIS INFORMATION
            if (curiter != lastiter)
            {
                recalc = true;
                lastiter = curiter;
                Message sm;
                sm.type = Iter;
                sm.len = sizeof(int);
                send(sock, &sm, sizeof(sm), 0);
                send(sock, &curiter, sm.len, 0);
                printf("Sent iter %d\n", curiter);
            }


            if (tmp.r != (changeBack ? backColor.r : linearColor.r) || tmp.g != (changeBack ? backColor.g : linearColor.g) || tmp.b != (changeBack ? backColor.b : linearColor.b)) redraw = true;
        }

        if (!run) break;

        SDL_Delay(1);

        if (recalc)
        {
            //m.Update(vals);

            m.type = Recalc;
            m.len = 0;
            send(sock, &m, sizeof(m), 0);
            printf("Recalc request sent\n");
            valsreceived = 0;

            recalc = false;
        }

        if (redraw)
        {
            printf("Rendering...");
            SDL_SetRenderDrawColor(renderer, backColor.r, backColor.g, backColor.b, 255);
            SDL_RenderClear(renderer);
            for (int y = 0; y < INIT_SCREEN_HEIGHT; y++)
            {
                for (int x = 0; x < INIT_SCREEN_WIDTH; x++)
                {
                    double n = vals[y * INIT_SCREEN_WIDTH + x] / 100.0f;
                    if (n < 0) continue;
                    int k = (int)n;
                    if (style == Banded)
                    {
                        switch (palette)
                        {
                            case Linear:
                                SDL_SetRenderDrawColor(renderer,
                                    scale(k, 0, lastiter, 0, linearColor.r),
                                    scale(k, 0, lastiter, 0, linearColor.g),
                                    scale(k, 0, lastiter, 0, linearColor.b), 255);
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
                                SDL_Color a = { scale(k, 0, lastiter, 0, linearColor.r),
                                    scale(k, 0, lastiter, 0, linearColor.g),
                                    scale(k, 0, lastiter, 0, linearColor.b) };
                                SDL_Color b = { scale(k + 1, 0, lastiter, 0, linearColor.r),
                                    scale(k + 1, 0, lastiter, 0, linearColor.g),
                                    scale(k + 1, 0, lastiter, 0, linearColor.b) };
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

            if (help)
            {

            }

            if (updatetext && getsavename)
            {

            }

            string strvals = "";
            if (!hideVals)
            {
                ostringstream zoom;
                ostringstream bounds;
                zoom << setprecision(24) << fixed << curzoom;
                bounds << setprecision(24) << fixed << "    " << curoffx << ",\n    " << curoffy;
                strvals = "Zoom: " + zoom.str() + "\nCenter: {\n" + bounds.str() + "\n}";
            }

            string s = "Palette: " + string(!changeBack ? "* " : "") + PaletteNames[palette] + "\n" +
                        "Inside: " + (changeBack ? "* " : "") + "{ " + to_string(backColor.r) + ", " + to_string(backColor.g) + ", " + to_string(backColor.b) + " }\n" +
                        "Style: " + StyleNames[style] + "\n";
                        "Iters: " + to_string(curiter) + "\n" + (hideVals ? "" : strvals);

            SDL_Rect r;
            SDL_Texture *tex = RenderFromText(renderer, &r, s, font, White, 500);
            r.x = 10; r.y = 5;
            //SDL_Surface* txt = TTF_RenderText_Blended_Wrapped(font, s.c_str(), White, 500);
            //SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, txt);
            //SDL_Rect r; r.x = 10; r.y = 5;
            //SDL_QueryTexture(tex, NULL, NULL, &r.w, &r.h);
            printf("%d, %d, %d, %d\n", r.x, r.y, r.w, r.h);
            SDL_RenderCopy(renderer, tex, NULL, &r);
            //SDL_FreeSurface(txt);
            if (palette == Linear)
            {
                s = "{ " + to_string(linearColor.r) + ", " + to_string(linearColor.g) + ", " + to_string(linearColor.b) + " }";
                //txt = TTF_RenderText_Blended_Wrapped(font, s.c_str(), linearColor, 500);
                //tex = SDL_CreateTextureFromSurface(renderer, txt);
                //SDL_DestroyTexture(tex);
                tex = RenderFromText(renderer, &r, s, font, linearColor, 500);
                //SDL_Rect r; r.x = 120; r.y = 5;
                r.x = 120;
                //SDL_QueryTexture(tex, NULL, NULL, &r.w, &r.h);
                SDL_RenderCopy(renderer, tex, NULL, &r);
                //SDL_FreeSurface(txt);
            }
            else
            {
                for (int i = 0; i < gradientScale; i++)
                {
                    SDL_SetRenderDrawColor(renderer, gradient[palette - 1][i].r, gradient[palette - 1][i].g, gradient[palette - 1][i].b, 255);
                    SDL_RenderDrawLine(renderer, 10, r.h + i + 5, 20, r.h + i + 5);
                }
            }

            //SDL_DestroyTexture(tex);
            SDL_RenderPresent(renderer);
            printf("Done.\n");
            redraw = false;
        }

        #endif
    }

    close(sock);
}
