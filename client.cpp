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
#include <sys/stat.h>

// #define PORT 8010
#define BUFF_SIZE 2048
#define MAX_CLIENTS 5

using namespace std;

pthread_t sender, reciever, tid;
int PORT;

void handleSig(int sig_num)
{
    signal(SIGINT, handleSig);
    fflush(stdout);
    return;
}

struct FuncParam
{
    int fd;
    string file_addr;
    bool isBinary;
};

bool fileExists(char *filename)
{
    DIR *mydir;
    bool exists = false;
    struct dirent *mydirent;
    mydir = opendir("./CLIENT");
    while ((mydirent = readdir(mydir)) != NULL)
        if (strcmp(mydirent->d_name, filename) == 0)
        {
            exists = true;
            break;
        }
    closedir(mydir);
    return exists;
}

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

    cout << "ProcessComplete!\n"
         << "Filename: " << ((FuncParam *)args)->file_addr << "\n"
         << "FileSize: " << fileSize << " bytes\n"
         << "Request: GET\n"
         << "Mode: " << (isBinary ? "Binary\n" : "ASCII\n")
         << "Number of chunks: " << cnt << "\n"
         << "Size per chunk: " << BUFF_SIZE << " bytes\n";

    fclose(fptr);
    close(sockfd);
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

    cout << "ProcessComplete!\n"
         << "Filename: " << ((FuncParam *)args)->file_addr << "\n"
         << "FileSize: " << fileSize << " bytes\n"
         << "Request: PUT\n"
         << "Mode: " << (isBinary ? "Binary\n" : "ASCII\n")
         << "Number of chunks: " << cnt << "\n"
         << "Size per chunk: " << BUFF_SIZE << " bytes\n";
    fclose(fptr);
    close(sockfd);
    return nullptr;
}

void *sender_func(void *args)
{

    int sockfd = *((int *)args);
    char buffer[BUFF_SIZE];

    // Enter username
    bzero(buffer, BUFF_SIZE);
    cout << "Enter your username:\n";
    fgets(buffer, BUFF_SIZE, stdin);

    // TODO: Put this in a function
    int size = 0;
    for (int i = 0; buffer[i] != '\n'; i++)
    {
        size++;
    }
    char ptr[size + 1];
    strncpy(ptr, buffer, size);
    ptr[size] = '\0';
    write(sockfd, ptr, BUFF_SIZE);

    // Enter password
    bzero(buffer, BUFF_SIZE);
    cout << "Enter your password:\n";
    fgets(buffer, BUFF_SIZE, stdin);

    // TODO: Put this in a function
    size = 0;
    for (int i = 0; buffer[i] != '\n'; i++)
    {
        size++;
    }
    char ptr2[size + 1];
    strncpy(ptr2, buffer, size);
    ptr2[size] = '\0';
    write(sockfd, ptr2, BUFF_SIZE);

    while (1)
    {
        signal(SIGINT, handleSig);
        bzero(buffer, BUFF_SIZE);
        cout << "Enter Command (Terminate command with $):\n";
        fgets(buffer, BUFF_SIZE, stdin);
        int size = 0;
        for (int i = 0; buffer[i] != '\n'; i++)
        {
            size++;
        }
        char ptrx[size + 1];
        strncpy(ptrx, buffer, size);
        ptrx[size] = '\0';
        if (strcmp(ptrx, "DATA_CONN_TRUE") == 0)
            continue;
        write(sockfd, ptrx, BUFF_SIZE);
        if (strcmp(ptrx, "close $") == 0)
            break;
    }
    pthread_exit(NULL);
}

void *reciever_func(void *args)
{
    int fd = *((int *)args);
    char buffer[BUFF_SIZE];
    while (1)
    {
        signal(SIGINT, handleSig);
        bzero(buffer, BUFF_SIZE);
        read(fd, buffer, BUFF_SIZE);
        // printf("MESSAGE: %s\n", buffer);
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
                else
                    params->isBinary = 0;
                if (strcmp(status, "FILE_NOT_FOUND") == 0 || (strcmp(op, "PUT") == 0 && fileExists(file_addr) == false))
                {
                    write(fd, "ERR", BUFF_SIZE);
                    close(sockfd);
                    cout << "[ / ] Data channel closed!" << endl;
                    continue;
                }
                else
                    write(fd, "DATA_CONN_TRUE", BUFF_SIZE);
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

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        cout << "Usage: ./client <ip> <port>" << endl;
        exit(1);
    }
    PORT = atoi(argv[2]);
    int sockfd;
    struct sockaddr_in serverAdd;
    signal(SIGINT, handleSig);
    mkdir("CLIENT", 0777);

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
    serverAdd.sin_addr.s_addr = inet_addr(argv[1]);

    if (connect(sockfd, (struct sockaddr *)&serverAdd, sizeof(serverAdd)) == 0)
    {
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
