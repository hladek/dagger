/* 
 *  A forked server
 *  by Martin Broadhurst (www.martinbroadhurst.com)
 */

#ifndef _TCPFORKING
#define _TCPFORKING

#include <stdio.h>
#include <string.h> /* memset() */
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <netdb.h>

#include <iostream>
#include <cstdlib>
#include <sstream>
#include <vector>

using namespace std;

#define BACKLOG     10  /* Passed to listen() */

class Annotator {
    public:

        virtual void annotate(char* line,size_t linesz,ostream& result){
            abort();
        }
        virtual ~Annotator(){}

};

class TCPFork {
    public:
    /* Signal handler to reap zombie processes */
    static void wait_for_child(int sig)
    {
        while (waitpid(-1, NULL, WNOHANG) > 0);
    }

    static void handle(int newsock,Annotator* annot)
    {
        cout << "Handling" << endl;
        while(1){
            vector<char> buf;
            char c= 0;
            int res = 0;
            while (1){
                res = recv(newsock,&c,1,0);
                if (res < 0){
                    cout << "Read error" << endl;
                    break;
                }
                if ( c == '\n'){
                    cout << "Got line " << buf.size() << endl;
                    break;
                }
                buf.push_back(c);
            }
            stringstream os;
            annot->annotate(buf.data(),buf.size(),os);
            res = send(newsock,os.str().c_str(),os.str().size(),0);
            if (res < 0){
                cout << "Write error" << endl;
                break;
            }
        }
        close(newsock);

        cout << "Close" << endl;
    }

    static int start(Annotator* anot,int port)
    {
        int sock;
        struct sigaction sa;
        struct addrinfo hints, *res;
        int reuseaddr = 1; /* True */

        /* Get the address info */
        memset(&hints, 0, sizeof hints);
        hints.ai_family = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        stringstream ss;
        ss << port;
        if (getaddrinfo(NULL, ss.str().c_str(), &hints, &res) != 0) {
            perror("getaddrinfo");
            return 1;
        }

        /* Create the socket */
        sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (sock == -1) {
            perror("socket");
            return 1;
        }

        /* Enable the socket to reuse the address */
        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuseaddr, sizeof(int)) == -1) {
            perror("setsockopt");
            return 1;
        }

        /* Bind to the address */
        if (bind(sock, res->ai_addr, res->ai_addrlen) == -1) {
            perror("bind");
            return 1;
        }

        /* Listen */
        if (listen(sock, BACKLOG) == -1) {
            perror("listen");
            return 1;
        }

        freeaddrinfo(res);

        /* Set up the signal handler */
        sa.sa_handler = wait_for_child;
        sigemptyset(&sa.sa_mask);
        sa.sa_flags = SA_RESTART;
        if (sigaction(SIGCHLD, &sa, NULL) == -1) {
            perror("sigaction");
            return 1;
        }

        /* Main loop */
        while (1) {
            struct sockaddr_in their_addr;
            size_t size = sizeof(struct sockaddr_in);
            int newsock = accept(sock, (struct sockaddr*)&their_addr, (socklen_t*)&size);
            int pid;

            if (newsock == -1) {
                perror("accept");
                return 0;
            }

            printf("Got a connection from %s on port %d\n", inet_ntoa(their_addr.sin_addr), htons(their_addr.sin_port));

            pid = fork();
            if (pid == 0) {
                /* In child process */
                close(sock);
                handle(newsock,anot);
                return 0;
            }
            else {
                /* Parent process */
                if (pid == -1) {
                    perror("fork");
                    return 1;
                }
                else {
                    close(newsock);
                }
            }
        }

        close(sock);

        return 0;
    }

};

#endif
