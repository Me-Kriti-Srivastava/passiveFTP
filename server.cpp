#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <pthread.h>
#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <vector>

#define PORT 8010
#define MAX_CLIENTS 5
#define MAX_DATA_CHANNELS_PER_CLIENT 5
#define BUFF_SIZE 256

using namespace std;

struct Client
{
    // pthread_mutex_t mut;
    int latestDataConnection;
    int fd;
    string username;
};

struct FuncParam
{
    int idx;
    string filename;
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

class User
{
private:
    string username;
    string password;

public:
    static map<string, pair<string, bool>> usersMap;
    void setCredentials(string username, string password)
    {
        if (username[username.size() - 1] == '\n')
            username.pop_back();
        if (password[password.size() - 1] == '\n')
            password.pop_back();
        this->username = username;
        this->password = password;
    }
    void signup()
    {
        // TODO
        // append credentials to "users.txt" and create a new directory
        // for new user
        ofstream authFile("users.txt", ios::app);
        string userDirectoryName = username;
        if (mkdir(userDirectoryName.c_str(), 0777) == 0)
        {
            authFile << username << " " << password << "\n";
            usersMap[username] = {password, 1};
        }
        else
        {
            perror("Error in creating user directory!");
            exit(1);
        }
    };
    bool authenticate()
    {
        // TODO
        // if username not found then signup()
        // else verify passsword
        //     if verified, switch to user directory
        if (usersMap.find(username) != usersMap.end())
        {
            if (password == usersMap[username].first)
            {
                return 1;
            }

            else
                return 0;
        }

        else
        {
            signup();
        }

        return 1;
    };
};

map<string, pair<string, bool>> User::usersMap; // username to {password, status}
bool availableSlots[MAX_CLIENTS];
Client clients[MAX_CLIENTS];
vector<vector<int>> clientDataConns(MAX_CLIENTS, vector<int>(MAX_DATA_CHANNELS_PER_CLIENT, -1));
map<string, int> dataConnMap; // ip to index
pthread_t thread_id;

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

// void newDataConnection()
// {
//     int dataSockFd = init(PORT - 1), dataClientFd;
//     struct sockaddr_in client_addr;
//     socklen_t cliSize;

//     dataClientFd = accept(dataSockFd, (struct sockaddr *)&client_addr, &cliSize);
// }

int init(int);
void *recvFile(void *);
void *sendFile(void *);

void commandMapping(char *command, int idx)
{
    // cout << "Command: " << command;
    string res;
    char *op = NULL, *filename = NULL, *flag = NULL;
    char statusBuffer[BUFF_SIZE];
    op = strtok(command, " ");
    if (op != NULL)
        filename = strtok(NULL, " ");
    if (filename != NULL)
        flag = strtok(NULL, " ");
    int fd = clients[idx].fd;
    FuncParam *params = new FuncParam();
    params->idx = idx;
    params->filename = string(filename);

    bzero(statusBuffer, BUFF_SIZE);

    res += "CREATE_DATA_CONN";
    if (strcmp(op, "GET") == 0 && filename != NULL)
        res += " GET ";
    else if (strcmp(op, "PUT") == 0 && filename != NULL)
        res += " PUT ";
    res += charArrToString(filename, getStringSizeC(filename));
    if (flag != NULL && strcmp(flag, "-b") == 0)
        res += " -b";

    bcopy("CREATE_DATA_CONN", statusBuffer, BUFF_SIZE);
    write(fd, statusBuffer, BUFF_SIZE);
    write(fd, res.c_str(), BUFF_SIZE);
    bzero(statusBuffer, BUFF_SIZE);
    int x = read(fd, statusBuffer, BUFF_SIZE);
    cout << "Status: " << statusBuffer << endl;

    if (strcmp(statusBuffer, "DATA_CONN_TRUE") == 0)
    {
        if (strcmp(op, "GET") == 0 && op != NULL)
            pthread_create(&thread_id, NULL, sendFile, (void *)params);
        else if (strcmp(op, "PUT") == 0 && op != NULL)
            pthread_create(&thread_id, NULL, recvFile, (void *)params);
    }
}

int getFileSize(ifstream *myFile)
{
    (*myFile).seekg(0, ios::end);
    return (*myFile).tellg();
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
    FuncParam *param = (FuncParam *)args;
    int idx = param->idx, fileSize, numBytes = 0, totalBytesRecieved = 0, cnt = 0;
    int clientFd = clients[idx].latestDataConnection;
    bool isBinary = param->isBinary;
    string user_dir = clients[idx].username + "/" + param->filename;
    FILE *fptr;
    // cout << user_dir << endl;
    if (isBinary)
        fptr = fopen(user_dir.c_str(), "wb");
    else
        fptr = fopen(user_dir.c_str(), "w");
    // ifstream myFile("test.txt");
    // int fileSize = getFileSize(&myFile);
    char fileBuffer[BUFF_SIZE];
    bzero(fileBuffer, BUFF_SIZE);
    read(clientFd, &fileSize, sizeof(int));
    cout << "FileSize " << fileSize << endl;
    while (totalBytesRecieved < fileSize)
    {
        numBytes = read(clientFd, fileBuffer, BUFF_SIZE);
        if (numBytes == 0) // EOF detected
            break;
        fwrite(fileBuffer, sizeof(char), numBytes, fptr);
        // printf("%s\n", fileBuffer);
        //  min(BUFF_SIZE, fileSize - totalBytesRecieved));
        // myFile.write(buffer, BUFF_SIZE);
        // numBytes = BUFF_SIZE;

        totalBytesRecieved += numBytes;
        cnt++;
    }
    fclose(fptr);
    close(clientFd);
    return nullptr;
}

void *sendFile(void *args)
{
    int idx = ((FuncParam *)args)->idx, numBytes = 0, totalBytesSent = 0, cnt = 0;
    int clientFd = clients[idx].latestDataConnection;
    bool isBinary = ((FuncParam *)args)->isBinary;
    cout << ((FuncParam *)args)->filename << endl;
    FILE *fptr;

    if (isBinary)
        fptr = fopen(("SERVER/" + ((FuncParam *)args)->filename).c_str(), "rb");
    else
        fptr = fopen(("SERVER/" + ((FuncParam *)args)->filename).c_str(), "r");

    int fileSize = getFileSizeC(fptr);

    // ifstream myFile("test.txt");
    // int fileSize = getFileSize(&myFile);
    write(clientFd, &fileSize, sizeof(int));
    char fileBuffer[BUFF_SIZE];
    cout << "FileSize: " << fileSize << endl;
    while (totalBytesSent < fileSize)
    {
        bzero(fileBuffer, BUFF_SIZE);
        numBytes = fread(fileBuffer, sizeof(char), BUFF_SIZE, fptr);
        if (numBytes == 0) // EOF detected
            break;
        // cout << fileBuffer << endl;
        // myFile.read(fileBuffer, BUFF_SIZE);
        // numBytes = myFile.gcount();
        // cout << numBytes << endl;
        write(clientFd, fileBuffer, numBytes);
        totalBytesSent += numBytes;
        cnt++;
    }
    fclose(fptr);
    close(clientFd);
    return nullptr;
}

void *listenDataConn(void *args)
{
    int newSockFd = init(PORT + 1);
    struct sockaddr_in client_addr;
    socklen_t cliSize;

    while (1)
    {
        int clientFd = accept(newSockFd, (struct sockaddr *)&client_addr, &cliSize);
        if (clientFd < 0)
        {
            perror("[ X ] Something went wrong while accepting data conn.\n");
            exit(1);
        }
        string client_uid = to_string(client_addr.sin_addr.s_addr);
        for (int i = 0; i < MAX_DATA_CHANNELS_PER_CLIENT; i++)
        {
            if (clientDataConns[dataConnMap[client_uid]][i] == -1)
            {
                clientDataConns[dataConnMap[client_uid]][i] = clientFd;
                break;
            }
        }

        clients[dataConnMap[client_uid]].latestDataConnection = clientFd;
    }
}

void *connector(void *args)
{
    int slot = *((int *)args), senderFd = clients[slot].fd;
    User user;
    char usernameBuffer[BUFF_SIZE], passwordBuffer[BUFF_SIZE], commandBuffer[BUFF_SIZE], statusBuffer[BUFF_SIZE];
    bzero(usernameBuffer, BUFF_SIZE);
    bzero(passwordBuffer, BUFF_SIZE);
    cout << "[ / ] Client_fd " << clients[slot].fd << " connected!" << endl;

    while (1)
    {
        read(senderFd, usernameBuffer, BUFF_SIZE);
        read(senderFd, passwordBuffer, BUFF_SIZE);
        user.setCredentials(
            charArrToString(usernameBuffer, BUFF_SIZE),
            charArrToString(passwordBuffer, BUFF_SIZE));
        user.authenticate();
        clients[slot].username = string(usernameBuffer);
        // TODO
        // Establish data connection
        read(senderFd, commandBuffer, BUFF_SIZE);
        commandMapping(commandBuffer, slot);
        // char *param1 = strtok(commandBuffer, " "), *param2 = strtok(NULL, " ");

        printf("%s\n", statusBuffer);

        // pthread_create(&thread_id, NULL, sendFile, (void *)&slot);
    }
}

int init(int PORT_NUM)
{
    struct sockaddr_in server_addr;
    int sockfd;
    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd <= 0)
    {
        perror("[ X ] Socket creation failed!\n");
        exit(1);
    }

    printf("[ / ] Socket created successfully!\n");

    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT_NUM);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        cout << "error in sockopt\n";
        exit(1);
    }

    if (bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("[ X ] Bind failed!\n");
        exit(1);
    }

    printf("[ / ] Bind successful on port: %d\n", (int)PORT_NUM);

    if (listen(sockfd, MAX_CLIENTS) != 0)
    {
        perror("[ X ] Listen attempt failed!\n");
        exit(1);
    }

    printf("[ / ] Listening on port: %d\nWaiting for incoming connections...\n", (int)PORT_NUM);

    return sockfd;
}

int main()
{

    int sockfd = init(PORT), freeSlot = 0;
    struct sockaddr_in client_addr;
    socklen_t cliSize;
    pthread_t dataConnectionListener;

    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        availableSlots[i] = 1;
        // clients[i].mut = PTHREAD_MUTEX_INITIALIZER;
    }
    // cout << availableSlots[3] << endl;

    pthread_create(&dataConnectionListener, NULL, listenDataConn, NULL);

    while (1)
    {
        // cout << freeSlot << endl;
        clients[freeSlot].fd = accept(sockfd, (struct sockaddr *)&client_addr, &cliSize);
        if (clients[freeSlot].fd < 0)
        {
            perror("[ X ] Something went wrong while accepting\n");
            exit(1);
        }
        pthread_t tid;

        int curr_idx = freeSlot;
        if (pthread_create(&tid, NULL, connector, (void *)&curr_idx) == 0)
        {
            availableSlots[freeSlot] = 0;
            string client_uid = to_string(client_addr.sin_addr.s_addr);
            dataConnMap[client_uid] = freeSlot;

            // CAN BE DONE IN O(1) WITH QUEUE
            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                if (availableSlots[i] == 1)
                {
                    freeSlot = i;
                    break;
                }
            }

            printf("[ / ] Successfully created new client thread!\n");
        }
        else
        {
            perror("[ X ] Error in creating client thread!\n");
            close(clients[freeSlot].fd);
        }
    }
    return 0;
}