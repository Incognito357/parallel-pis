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

#define HOSTPORT 31415

#ifdef MASTER

#include <sys/time.h>
#include <vector>
#define MAXCLIENTS 10

#else

#include <Mandelbrot.h>
#define HOSTADDR "10.3.14.15"

#endif

using namespace std;

int main()
{
    struct sockaddr_in addr;

    #ifdef MASTER

    int addrlen, master_socket, new_socket, clients[MAXCLIENTS];
    fd_set fds;

    for (int i = 0; i < MAXCLIENTS; i++) clients[i] = 0;
    if ((master_socket = socket(AF_INET, SOCK_STREAM, 0) == 0))
    {
        printf("Could not create socket.");
        return -1;
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(HOSTPORT);

    if (bind(master_socket, (struct sockaddr*)&addr, sizeof(addr)) < 0)
    {
        printf("Could not bind port");
        return -1;
    }

    printf("Listening on port %d\n", HOSTPORT);

    if (listen(master_socket, 5) < 0)
    {
        printf("Could not listen");
        return -1;
    }

    addrlen = sizeof(addr);
    printf("Waiting for connections\n");

    #else



    #endif

    while (true)
    {
        #ifdef MASTER

        FD_ZERO(&fds);

        FD_SET(master_socket, &fds);
        int ms = master_socket;
        for (int i = 0; i < MAXCLIENTS; i++)
        {
            int s = clients[i];
            if (s > 0) FD_SET(s, &fds);
            if (s > ms) ms = s;
        }

        int activity = select(ms + 1, &fds, NULL, NULL, NULL);

        if (activity < 0 && errno != EINTR) printf("'select' error\n");

        if (FD_ISSET(master_socket, &fds))
        {
            int newsock = accept(master_socket, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
            if (newsock < 0)
            {
                printf("Could not accept client");
                return -1;
            }

            printf("New connection from %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));

            char* msg = "You have just connected to Master Pi!\n";
            int msglen = strlen(msg);
            if (send(newsock, msg, msglen, 0) != msglen) printf("Could not send message");
            else printf("Greeted %s", inet_ntoa(addr.sin_addr));

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
                char buf[1025];
                int val = read(s, buf, 1024);
                getpeername(s, (struct sockaddr*)&addr, (socklen_t*)&addrlen);
                if (val == 0)
                {
                    printf("Client %s disconnected\n", inet_ntoa(addr.sin_addr));
                    close(s);
                    clients[i] = 0;
                }
                else
                {
                    buf[val] = 0;
                    printf("-> %s: \"%s\"\n", inet_ntoa(addr.sin_addr), buf);
                    char* msg = "Received message";
                    send(s, msg, strlen(buf), 0);
                }
            }
        }

        #else



        #endif
    }

    #ifdef MASTER



    #endif
}
