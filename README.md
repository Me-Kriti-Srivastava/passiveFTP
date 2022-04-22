# Passive File Transfer Protocol

## Description

**Passive FTP** is a variant of the FTP, a network protocol for transmitting files between computers over TCP/IP connections.

Two types of channels have been implemented,

1. **Control/Command channel** : Used to exchange commands and replies to commands between client and server
2. **Data channel** : Used to send (recieve) a file to (from) a host

A naive authentication mechanism has also been implemented to make the clients perform R/W operations in a dedicated directory. The program prompts the user to enter username and password, if the entered username does not exist in `users.txt` file, it is considered a _register_ attempt. The user is registered and a new directory is created for that user. If the username matches an entry in `users.txt`, the program checks the corresponding password and the attempt is considered _login_.

The server listens for control channel connection requests on `PORT` and for data channel connection requests on `PORT+1`. A control channel is established when a client connects to the server. Data channels are created on request (GET/PUT). _This mechanism enables multiple clients to perform multiple GET/PUT operations simultaneously_.

Only one control channel exists between a client and the server but multiple data channels may exist. Data channels are closed automatically when the designated file transfer is completed.

## Commands Supported

1. **GET** : Downloads a file from the logged-in user's directory from the server and saves it in the `CLIENT` directory on the client.

   ```bash
   GET <filename> [-b (for binary mode)] $
   ```

2. **LIST** : Description: List all files present in logged in clients's directory on the server.

   ```bash
   LIST $
   ```

3. **PUT** : Uploads a file from the `CLIENT` directory from client to the logged-in user's directory on the server.

   ```bash
   PUT <filename> [-b (for binary mode)] $
   ```

4. **close** : Disconnect the client from server.

   ```bash
   close $
   ```

## Setup and Run

1. Clone this repo

   ```bash
   git clone https://github.com/rahulpathak-github/passiveFTP
   ```

2. Run the following command to build and move into the `build` directory

   ```bash
   cd passiveFTP && sh build.sh && cd build
   ```

3. Run the server

   ```bash
   ./server <PORT>
   ```

4. Run the client

   ```bash
   ./client <IP> <PORT>
   ```

Note: Current implemetation allows only one connection from one IP address.

## Author

- [Rahul Pathak](https://github.com/rahulpathak-github)
