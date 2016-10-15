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

#define MAXBUF 1024

#ifdef MASTER

#include <sys/time.h>
#include <vector>
#define HOSTPORT 31415
#define MAXCLIENTS 10

#else

#include <Mandelbrot.h>
#define HOSTPORT "31415"
#define HOSTADDR "10.3.14.5"

#endif

using namespace std;

int main()
{
    struct sockaddr_in addr;
    int sock;
    char buf[MAXBUF + 1];

    #ifdef MASTER

    int clients[MAXCLIENTS];
    fd_set fds;

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

        break;
    }

    if (i == NULL)
    {
        printf("Could not connect\n");
        return -1;
    }

    printf("Connecting to %s:%d\n", inet_ntoa(((struct sockaddr_in*)i)->sin_addr), ntohs(((struct sockaddr_in*)i)->sin_port));

    freeaddrinfo(serv);

    #endif

    while (true)
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

            char* msg = "You have just connected to Master Pi!";
            int msglen = strlen(msg);
            if (send(newsock, msg, msglen, 0) != msglen) printf("Could not send message\n");
            else printf("Greeted %s\n", inet_ntoa(addr.sin_addr));

            for (int i = 0; i < MAXCLIENTS; i++)
            {
                if (clients[i] == 0)
                {
                    clients[i] = newsock;
                    printf("Client %d added to list\n", i);
                    break;
                }
            }
        }

        for (int i = 0; i < MAXCLIENTS; i++)
        {
            int s = clients[i];
            if (FD_ISSET(s, &fds))
            {
                int val = read(s, buf, MAXBUF);
                getpeername(s, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
                if (val == 0)
                {
                    printf("Client %s disconnected\n", inet_ntoa(addr.sin_addr));
                    close(s);
                    clients[i] = 0;
                }
                else
                {
                    buf[val - 2] = 0;
                    printf("-> %s: \"%s\"\n", inet_ntoa(addr.sin_addr), buf);
                    char* msg = "Received message";
                    send(s, msg, strlen(msg), 0);
                }
            }
        }

        #else

        ret = recv(sock, buf, MAXBUF, 0);
        if (ret < 0)
        {
            printf("Error receiving message");
            return -1;
        }

        buf[ret] = 0;
        printf("Server: \"%s\"\n", buf);

        #endif
    }

    close(sock);
}
