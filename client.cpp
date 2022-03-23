#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <string>
#include <iostream>
#include <fstream>
#include <signal.h>
#include <dirent.h>
#include <fcntl.h>

#define PORT 8010
#define BUFF_SIZE 256
#define MAX_CLIENTS 5

using namespace std;

pthread_t sender, reciever, tid;

// void handleSig(int sig_num)
// {
//     signal(SIGINT, handleSig);
//     fflush(stdout);
//     return;
// }

void *dataChannel(void *args)
{
    int sockfd = *((int *)args), fileSize, totalBytesRecieved = 0, numBytes = 0, cnt = 0;
    char buffer[BUFF_SIZE];
    bzero(buffer, BUFF_SIZE);
    FILE *fptr;
    fptr = fopen("new2.txt", "w");
    // ofstream myFile("new.txt", ios::app);
    read(sockfd, &fileSize, sizeof(int));
    while (totalBytesRecieved < fileSize)
    {
        numBytes = read(sockfd, buffer, BUFF_SIZE);
        if (numBytes == 0) // EOF detected
            break;
        fwrite(buffer, sizeof(char), numBytes, fptr);
        printf("%s\n", buffer);
        //  min(BUFF_SIZE, fileSize - totalBytesRecieved));
        // myFile.write(buffer, BUFF_SIZE);
        // numBytes = BUFF_SIZE;

        totalBytesRecieved += numBytes;
        cnt++;
    }

    fclose(fptr);
    cout << cnt << "sdfsdfsdfsfsdvdbnnhgbfvdsdfgfdbbbbbbbbbbbbbbb" << endl;
    // read(sockfd, buffer, BUFF_SIZE);
    // printf("%s\n\nsefafasfasdasfafasf!!!!!!\n", buffer);
}

void *sender_func(void *args)
{

    int sockfd = *((int *)args);
    while (1)
    {
        // signal(SIGINT, handleSig);
        char buffer[BUFF_SIZE];
        bzero(buffer, BUFF_SIZE);
        // printf("Enter message: ");
        // fflush(stdout);
        scanf("%s", buffer);
        write(sockfd, buffer, BUFF_SIZE);
        // if (strncmp(buffer, "close", 5) == 0)
        // {
        //     break;
        // }
    }
}

void *reciever_func(void *args)
{
    int fd = *((int *)args);
    while (1)
    {
        // signal(SIGINT, handleSig);
        char buffer[BUFF_SIZE];
        bzero(buffer, BUFF_SIZE);
        read(fd, buffer, BUFF_SIZE);

        printf("MESSAGE: %s\n", buffer);

        int sockfd;
        struct sockaddr_in serverAdd;
        sockfd = socket(AF_INET, SOCK_STREAM, 0);

        if (sockfd <= 0)
        {
            perror("[ X ] Socket creation failed!\n");
            exit(1);
        }

        printf("[ / ] Socket created successfully!");

        memset(&serverAdd, '\0', sizeof(serverAdd));
        serverAdd.sin_family = AF_INET;
        serverAdd.sin_port = htons(PORT + 1);
        serverAdd.sin_addr.s_addr = INADDR_ANY;

        // signal(SIGINT, handleSig);
        // cout << "DEBUG_63" << endl;

        if (connect(sockfd, (struct sockaddr *)&serverAdd, sizeof(serverAdd)) == 0)
        {

            // cout << "DEBUG_68" << endl;
            // sleep(10);

            pthread_create(&tid, NULL, dataChannel, (void *)&sockfd);
            printf("[ / ] LOLOLOL!");
        }
        else
        {
            perror("[ X ]Connection with the server failed...\n");
            exit(1);
        }
        // fflush(stdout);
    }
}

int main()
{
    int sockfd;
    struct sockaddr_in serverAdd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd <= 0)
    {
        perror("[ X ] Socket creation failed!\n");
        exit(1);
    }

    printf("[ / ] Socket created successfully!\n");

    memset(&serverAdd, '\0', sizeof(serverAdd));
    serverAdd.sin_family = AF_INET;
    serverAdd.sin_port = htons(PORT);
    serverAdd.sin_addr.s_addr = INADDR_ANY;

    // signal(SIGINT, handleSig);
    // cout << "DEBUG_63" << endl;

    if (connect(sockfd, (struct sockaddr *)&serverAdd, sizeof(serverAdd)) == 0)
    {

        // cout << "DEBUG_68" << endl;
        // sleep(10);

        pthread_create(&sender, NULL, sender_func, (void *)&sockfd);
        pthread_create(&reciever, NULL, reciever_func, (void *)&sockfd);

        // pthread_join(reciever, NULL);
        pthread_join(sender, NULL);
    }
    else
    {
        perror("[ X ]Connection with the server failed...\n");
        exit(1);
    }

    close(sockfd);
}
