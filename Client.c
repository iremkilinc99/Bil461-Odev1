#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/msg.h>
#include <unistd.h>
#include "string.h"
#include <netinet/in.h>
#include <arpa/inet.h>
#define SHMSZ  2000

struct ClientInfo {
    pthread_t thread_id;
    pthread_t serverThreadId;
    long msg_type;
    int fileName;
    int shmid;
}clientInfo;

int rpid;

int main(int arg, char *argv[]) {
    int shmid;
    key_t key;
    char  *s;
    char *shm;
    struct sockaddr_in server;
    pthread_t thread_id;
    thread_id = getpid();

    int sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1) printf("Could not create socket");

    puts("Socket created");

    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 8888 );

    int fileNumber = argv[1][0] -'0';
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0) perror("connect failed. Error");
    puts("Connected");

    if(write(sock, &fileNumber, sizeof(fileNumber)) < 0) printf("Send failed\n");

    key = ftok("Server.c", fileNumber);
    if ( 0 > key ) perror("ftok");

    if ((shmid = shmget(key, SHMSZ,  IPC_CREAT | 0666)) < 0) {
        perror("shmget");
        exit(1);
    }

    if ((shm = shmat(shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    clientInfo.thread_id = thread_id;
    clientInfo.serverThreadId = 0;
    clientInfo.shmid = shmid;
    clientInfo.fileName = fileNumber;
    clientInfo.msg_type=1;

    int msgid = msgget(key,IPC_CREAT | 0666);

    while(1) {
        if(msgsnd(msgid, &clientInfo, sizeof(clientInfo), 1) < 0)
            perror("Error sending the message.");

        rpid = msgget(clientInfo.thread_id, IPC_CREAT | 0600);

        printf("Waiting for server to reply...\n");

        if (rpid < 0) perror("Error creating the message queue.\n");

        if(msgrcv(rpid, &clientInfo, sizeof(clientInfo), 0, 0) < 0)
            perror("Error receiving a message.\n");

        if(clientInfo.msg_type == 0) {
            for (s = shm; *s != NULL; s++)
                putchar(*s);
        }

        putchar('\n');
        printf("do you want to continue? if Yes enter the file number, if No just enter 0.\n");
        scanf("%i",&clientInfo.fileName);

        if(clientInfo.fileName == 0){
            clientInfo.msg_type = -1 ;
            msgsnd(msgid, &clientInfo, sizeof(clientInfo), 1);
            sleep(1);
            break;
        }
    }

    close(sock);
    msgctl(rpid,IPC_RMID,NULL);
    shmdt(shm);
    printf("Client has detached its shared memory... msgid %i\n", msgid);
    exit(0);
}

