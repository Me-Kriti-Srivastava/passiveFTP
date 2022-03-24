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

struct Response
{
    string status;
    string op;
    string file_addr;
};

struct FuncParam
{
    int fd;
    string file_addr;
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
    cout << "dgfdgvsgsdg" << endl;
    // FuncParam *param = new FuncParam;
    // param->fd = ((FuncParam *)args)->fd;
    // param->file_addr = ((FuncParam *)args)->file_addr;
    char *param1 = strtok((char *)args, " "), *param2 = strtok(NULL, " ");
    // cout << param->fd << endl;
    int sockfd = atoi(param1), fileSize, totalBytesRecieved = 0, numBytes = 0, cnt = 0;
    char buffer[BUFF_SIZE];
    if (param2 == NULL)
        cout << "NULLLLLL" << endl;
    string client_dir = "CLIENT/" + string(param2);
    FILE *fptr;
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
}

void *sendFile(void *args)
{
    FuncParam param = *((FuncParam *)args);
    int sockfd = param.fd, fileSize, totalBytesSent = 0, numBytes = 0, cnt = 0;
    char buffer[BUFF_SIZE];
    FILE *fptr;
    fptr = fopen(param.file_addr.c_str(), "r");
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
        string param;
        // = (FuncParam *)malloc(sizeof(FuncParam));
        printf("MESSAGE: %s\n", buffer);
        if (strcmp(buffer, "CREATE_DATA_CONN") == 0)
        {
            // cout << "debug102\n";
            char res[BUFF_SIZE];
            read(fd, res, BUFF_SIZE);
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

            // signal(SIGINT, handleSig);
            // cout << "DEBUG_63" << endl;
            pthread_t thread_id;

            if (connect(sockfd, (struct sockaddr *)&serverAdd, sizeof(serverAdd)) == 0)
            {

                // sleep(10);
                write(fd, "DATA_CONN_TRUE", BUFF_SIZE);
                // string res->file_addr = "test.txt";
                // cout << res->file_addr << endl;
                char *param1 = strtok(res, " "), *param2 = strtok(NULL, " "), *param3 = strtok(NULL, " ");
                param += to_string(sockfd);
                param += " ";
                param += string(param3);

                // param.fd = sockfd;
                // param.file_addr = res.file_addr;
                // cout << (*param).file_addr << endl;
                char buff[BUFF_SIZE];
                bcopy(param.c_str(), buff, param.size());
                if (strcmp(param2, "GET") == 0)
                    pthread_create(&thread_id, NULL, recvFile, (void *)&buff);
                else
                    pthread_create(&thread_id, NULL, sendFile, (void *)&buff);
                // string status = "DATA_CONN_TRUE";
                // char statusBuffer[BUFF_SIZE];
                // bzero(statusBuffer, BUFF_SIZE);
                // bcopy(status.c_str(), statusBuffer, status.size());

                // printf("[ / ] LOLOLOL!\n");
            }
            else
            {
                perror("[ X ]Connection with the server failed...\n");
                exit(1);
            }
            // fflush(stdout);
            //}
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
