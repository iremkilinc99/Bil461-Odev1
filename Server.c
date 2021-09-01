#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <netinet/in.h>

struct ClientInfo {
    pthread_t thread_id;
    pthread_t serverThreadId;
    long msg_type;
    int fileName;
    int shmid;
}clientInfo;

int readFile1(char *, char *);
void respond();
void setSemaphore();
void readData();
void unlockSemaphore();
void *listener();

int msgid;
key_t key1;
char *shm;
sem_t sem_var;
int newSocket;
int buffer;
int socket_desc;

int main(){
    struct sockaddr_in server, client;
    pthread_t pthread;

    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1) printf("Could not create socket");
    puts("Socket created");

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons( 8888 );

    if(bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0) perror("bind failed. Error");

    puts("bind done");
    listen(socket_desc, 15);
    int c = sizeof(struct sockaddr_in);

    while(1){
        newSocket = accept(socket_desc, (struct sockaddr *) &client, (socklen_t *)&c);
        printf("accepted");
        recv(newSocket, &buffer, sizeof(buffer), 0);

        key1 = ftok("Server.c", buffer);
        if ( 0 > key1 ) perror("ftok");
        msgid = msgget(key1, IPC_CREAT | 0666);

        if(pthread_create(&pthread, NULL, listener, &newSocket) != 0 )
            printf("Failed to create thread\n");

        printf("going back to listening.\n");
        printf("\n");
    }

    close(socket_desc);
    exit(0);
}

void *listener() {
    while (1) {
        printf("Waiting... \n");
        setSemaphore();
        readData();

        if(clientInfo.msg_type == -1)
            break;

        unlockSemaphore();
        respond();
    }

    unlockSemaphore();
    printf("%ld destroyed. \n", clientInfo.thread_id);
    shmdt((void *) shm);
    printf("Server has detached its shared memory...\n");
    shmctl(clientInfo.shmid, IPC_RMID, NULL);
    printf("Server has removed its shared memory...\n");
    pthread_cancel(clientInfo.serverThreadId);
    printf("\n");
}

void readData(){
    printf("Ready to receive... \n");
    if(msgrcv(msgid, &clientInfo, sizeof(clientInfo), 0, 0) <0)   perror("Error receiving a message in READUEUE.\n");
    clientInfo.serverThreadId = pthread_self();

    printf("Data Received is : file: %i, msg type %li, shmid: %i, and client id: %ld, Server thread %ld \n", clientInfo.fileName, clientInfo.msg_type, clientInfo.shmid, clientInfo.thread_id, clientInfo.serverThreadId);

    if ((shm = shmat(clientInfo.shmid, NULL, 0)) == (char *) -1) {
        perror("shmat");
        exit(1);
    }

    if(clientInfo.fileName == 1) {
        readFile1(shm, "d1");
    }else if(clientInfo.fileName == 2) {
        readFile1(shm, "d2");
    }else if(clientInfo.fileName == 3) {
        readFile1(shm, "d3");
    }else if(clientInfo.fileName == 4) {
        readFile1(shm, "d4");
    }else
        readFile1(shm, "d5");

}

void setSemaphore(){
//(lock)
    sem_init(&sem_var, 1, 1);
    sem_wait(&sem_var);
}

void unlockSemaphore(){
// unlock
    sem_post(&sem_var);
}

void respond(){
    printf("Replying...\n");
    int rpid = msgget(clientInfo.thread_id, IPC_CREAT | 0600);
    clientInfo.msg_type = 0;

    if(msgsnd(rpid, &clientInfo, sizeof(clientInfo),1) < 0) perror("Msg send error");

    msgctl(rpid,IPC_RMID,NULL);
    printf("\n");
}

int readFile1(char *shm, char *fileName) {
    FILE *fptr;
    char buffer[250];
    char *s = shm;

    fptr = fopen(fileName,"r");

    fgets(buffer,250,fptr);
    for (int i =0; i < sizeof buffer; i++)
        *s++ = buffer[i];

    *s = NULL;
    fclose(fptr);
    return 1;
}