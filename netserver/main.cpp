#include <iostream>
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
#include <list>
#include <queue>


using namespace std;

pthread_mutex_t msgmutex = PTHREAD_MUTEX_INITIALIZER;

queue<string> msgq;
list<int> clt_list;

static const int BUFFER_SIZE = 256;

void *msgbroad(void *arg)
{
    long qsize;
    string nextmsg;
    char msg[BUFFER_SIZE];

    while(true)
    {
        if( msgq.empty() )
        {
            usleep(10000); // sleep 0.01 sec before trying again
            continue;
        }

        pthread_mutex_lock(&msgmutex);
        qsize=msgq.size();
        if(qsize > 5)
            cout << "Queue size: " << qsize << endl;
        nextmsg=msgq.front(); // get next message in queue
        msgq.pop(); // remove it from the queue
        pthread_mutex_unlock(&msgmutex);
        strcpy(msg, nextmsg.c_str());

        for (list<int>::iterator li = clt_list.begin(); li != clt_list.end(); li++){
            if (send(*li, msg, strlen(msg), 0) < 0){
                cout << strerror(errno) << endl;
                exit(1);
            }
        }
        usleep(1000000); // sleep 1.0 sec
    }
    pthread_exit((void *)0);
} // msgbroad()

void usage(char *pname){
    cout<<"Usage: "<<pname<<" <port> [-eb]"<<endl;
    cout<<"Usage: "<<pname<<" 1234 [-eb]"<<endl;
    exit(0);
}

void *t_exchange(void *clt_sd){
    int sd = *((int *)clt_sd);

    while (true){
        char msg[BUFFER_SIZE];
        ssize_t res = recv(sd, msg, BUFFER_SIZE-1, 0);
        if (res < 0){
            cout << strerror(errno) << endl;
            exit(1);
        }else if (res == 0){
            break;
        }else{
            msg[res] = '\0';
            if (strcmp(msg, "quit") == 0){
                cout << "disconnected" << endl;
                close(sd);
                break;
            }
            cout << "Message: " << msg << endl;

            if (send(sd, msg, strlen(msg), 0) < 0){
                cout << strerror(errno) << endl;
                exit(1);
            }
        }
    }
}

void *t_exchangeBro(void *clt_sd){
    int sd = *((int *)clt_sd);

    char msg[BUFFER_SIZE];

    while (true){
        ssize_t res = recv(sd, msg, BUFFER_SIZE-1, 0);
        if (res < 0){
            cout << strerror(errno) << endl;
            exit(1);
        }else if (res == 0){
            break;
        }else{
            msg[res] = '\0';
            if (strcmp(msg, "quit") == 0){
                cout << "disconnected" << endl;
                pthread_mutex_lock(&msgmutex);
                clt_list.remove(sd);
                pthread_mutex_unlock(&msgmutex);
                close(sd);
                break;
            }
            cout << "Message: " << msg << endl;

            pthread_mutex_lock(&msgmutex);
            msgq.push(msg); // push message to the queue
            pthread_mutex_unlock(&msgmutex);
        }
    }
}

int main(int argc, char *argv[])
{
    char eb[10];
    if (argc == 3){
        strcpy(eb, argv[2]);
        if (strcmp(eb, "-eb"))
            usage(argv[0]);
    }else if (argc == 2);
    else{
        usage(argv[0]);
    }

    int serv_sd, clt_sd;

    if ((serv_sd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        cout << strerror(errno) << endl;
        exit(1);
    }

    sockaddr_in serv_addr, clt_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(atoi(argv[1]));

    bind(serv_sd, (sockaddr*)&serv_addr, sizeof(serv_addr));

    if (listen(serv_sd, 5) < 0){
        cout << strerror(errno) << endl;
        exit(1);
    }

    if ( !strcmp(eb, "-eb") )
    {
        pthread_t thr;
        // Create msgbroad thread
        if( pthread_create(&thr, NULL, msgbroad, NULL) )
        {
            cout << strerror(errno) << endl;
            exit(1);
        }
    }

    pthread_t p_th;
    while (true){
        socklen_t sock_len = sizeof(clt_addr);
        if ((clt_sd = accept(serv_sd, (sockaddr*)&clt_addr, &sock_len)) < 0){
            cout << strerror(errno) << endl;
            exit(1);
        }

        char str[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clt_addr.sin_addr), str, INET_ADDRSTRLEN);
        clt_list.push_back(clt_sd);
        cout << str << "\tconnected" << endl;

        // Create t_exchange or t_exchangeBro Thread
        if ( !strcmp(eb, "-eb") )
        {
            if ( pthread_create(&p_th, NULL, t_exchangeBro, (void *)&clt_sd) )
            {
                cout << strerror(errno) << endl;
                exit(1);
            }
        }else
        {
            if ( pthread_create(&p_th, NULL, t_exchange, (void *)&clt_sd) )
            {
                cout << strerror(errno) << endl;
                exit(1);
            }
        }
    }

    return 0;
}
