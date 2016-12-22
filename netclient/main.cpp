#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <pthread.h>

using namespace std;

static const int BUFFER_SIZE = 256;

void *t_read(void *serv_sd){
    int sd = *((int *)serv_sd);
    char msg[BUFFER_SIZE];
    ssize_t res;

    while ((res = recv(sd, msg, BUFFER_SIZE-1, 0)) > 0){
        if (res <= 0){
            if (res == 0)
                break;
            cout << strerror(errno) << endl;
            exit(1);
        }
        msg[res] = '\0';
        cout << "Echo Message: " << msg << endl;
    }
}

void usage(char *pname){
    cout<<"Usage: "<<pname<<" <ip-address> <port>"<<endl;
    cout<<"Usage: "<<pname<<" 127.0.0.1 1234"<<endl;
    exit(0);
}

int main(int argc, char *argv[])
{
    if (argc != 3){
        usage(argv[0]);
    }

    int sd = socket(AF_INET, SOCK_STREAM, 0);

    if (sd < 0){
        cout << strerror(errno) << endl;
        exit(1);
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;                  // Use Version INET 4
    addr.sin_addr.s_addr = inet_addr(argv[1]);  // IP Address (127.0.0.1)
    addr.sin_port = htons(atoi(argv[2]));       // PORT

    if (connect(sd, (sockaddr*)&addr, sizeof(addr)) < 0){
        cout << strerror(errno) << endl;
        exit(1);
    }

    pthread_t thr;
    // Create t_read Thread
    if ( pthread_create(&thr, NULL, t_read, (void *)&sd) )
    {
        cout << strerror(errno) << endl;
        exit(1);
    }

    cout << "<Input MSG (or quit) >" << endl;
    while (true){
        char input[BUFFER_SIZE];
        fgets(input, BUFFER_SIZE-1, stdin);

        input[strlen(input)-1] = '\0';

        if (send(sd, input, strlen(input), 0) < 0){
            cout << strerror(errno) << endl;
            exit(1);
        }

        if (strcmp(input, "quit") == 0){
            cout << "okay, bye ~ !" << endl;
            exit(0);
        }
    }

    close(sd);

    return 0;
}
