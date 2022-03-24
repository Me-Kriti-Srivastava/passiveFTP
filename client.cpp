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

struct FuncParam
{
    int fd;
    string file_addr;
    bool isBinary;
};

int getStringSizeC(char *s)
{
    int size = 0;
    for (int i = 0; s[i] != '\n'; i++)
    {
        size++;
    }

    return size + 1;
}

string charArrToString(char *a, int size)
{
    int i;
    string s = "";
    for (i = 0; i < size && a[i] != '\0'; i++)
    {
        s = s + a[i];
    }
    return s;
}

int getFileSizeC(FILE *fptr)
{
    size_t fileSize;
    fseek(fptr, 0L, SEEK_END);
    fileSize = ftell(fptr);
    rewind(fptr);
    return fileSize;
}

void *recvFile(void *args)
{
    int sockfd = ((FuncParam *)args)->fd, fileSize, totalBytesRecieved = 0, numBytes = 0, cnt = 0;
    char buffer[BUFF_SIZE];
    bool isBinary = ((FuncParam *)args)->isBinary;
    string client_dir = "CLIENT/" + ((FuncParam *)args)->file_addr;
    FILE *fptr;
    if (isBinary)
        fptr = fopen(client_dir.c_str(), "wb");
    else
        fptr = fopen(client_dir.c_str(), "w");
    read(sockfd, &fileSize, sizeof(int));
    cout << "FileSize: " << fileSize << endl;
    while (totalBytesRecieved < fileSize)
    {
        bzero(buffer, BUFF_SIZE);
        numBytes = read(sockfd, buffer, BUFF_SIZE);
        if (numBytes == 0) // EOF detected
            break;
        fwrite(buffer, sizeof(char), numBytes, fptr);
        totalBytesRecieved += numBytes;
        cnt++;
    }

    fclose(fptr);
    return nullptr;
}

void *sendFile(void *args)
{
    int sockfd = ((FuncParam *)args)->fd, fileSize, totalBytesSent = 0, numBytes = 0, cnt = 0;
    char buffer[BUFF_SIZE];
    bool isBinary = ((FuncParam *)args)->isBinary;
    FILE *fptr;
    if (isBinary)
        fptr = fopen(("CLIENT/" + ((FuncParam *)args)->file_addr).c_str(), "rb");
    else
        fptr = fopen(("CLIENT/" + ((FuncParam *)args)->file_addr).c_str(), "r");
    fileSize = getFileSizeC(fptr);

    write(sockfd, &fileSize, sizeof(int));
    while (totalBytesSent < fileSize)
    {
        bzero(buffer, BUFF_SIZE);
        numBytes = fread(buffer, sizeof(char), BUFF_SIZE, fptr);
        if (numBytes == 0) // EOF detected
            break;
        write(sockfd, buffer, numBytes);
        totalBytesSent += numBytes;
        cnt++;
    }

    fclose(fptr);
    return nullptr;
}

void *sender_func(void *args)
{

    int sockfd = *((int *)args);
    while (1)
    {
        // signal(SIGINT, handleSig);
        char buffer[BUFF_SIZE];
        bzero(buffer, BUFF_SIZE);
        fgets(buffer, BUFF_SIZE, stdin);
        int size = 0;
        for (int i = 0; buffer[i] != '\n'; i++)
        {
            size++;
        }
        char ptr[size + 1];
        strncpy(ptr, buffer, size);
        ptr[size] = '\0';
        if (strcmp(ptr, "DATA_CONN_TRUE") == 0)
            continue;
        write(sockfd, ptr, BUFF_SIZE);
    }
}

void *reciever_func(void *args)
{
    int fd = *((int *)args);
    // while (1)
    // {
    // signal(SIGINT, handleSig);
    char buffer[BUFF_SIZE];
    while (1)
    {
        bzero(buffer, BUFF_SIZE);
        read(fd, buffer, BUFF_SIZE);
        printf("MESSAGE: %s\n", buffer);
        if (strcmp(buffer, "CREATE_DATA_CONN") == 0)
        {
            char res[BUFF_SIZE];
            read(fd, res, BUFF_SIZE);
            FuncParam *params = new FuncParam();

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
            serverAdd.sin_port = htons(PORT + 1);
            serverAdd.sin_addr.s_addr = INADDR_ANY;

            pthread_t thread_id;

            if (connect(sockfd, (struct sockaddr *)&serverAdd, sizeof(serverAdd)) == 0)
            {

                write(fd, "DATA_CONN_TRUE", BUFF_SIZE);
                char *status = NULL, *op = NULL, *file_addr = NULL, *flag = NULL;
                status = strtok(res, " ");
                if (status != NULL)
                    op = strtok(NULL, " ");
                if (op != NULL)
                    file_addr = strtok(NULL, " ");
                if (file_addr != NULL)
                    flag = strtok(NULL, " ");
                params->fd = sockfd;
                params->file_addr = string(file_addr);
                if (flag != NULL)
                    params->isBinary = 1;
                if (strcmp(op, "GET") == 0)
                    pthread_create(&thread_id, NULL, recvFile, (void *)params);
                else
                    pthread_create(&thread_id, NULL, sendFile, (void *)params);
            }
            else
            {
                perror("[ X ]Connection with the server failed...\n");
                exit(1);
            }
        }
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
