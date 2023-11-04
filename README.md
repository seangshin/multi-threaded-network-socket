# Multi-threaded Network Sockets

## Description 
The motivation of this project is to desgin a multi-threaded socket interface for a client-server application. The client and server processes communicate using TCP sockets. It will implment the yamotd protocol to return and store messages from the client. The server has a linux operating system managed through a command-line interface which will be accessed remotely using SSH (Secure Shell).

It uses the pthread library for creating and managing threads, and it employs the fd_set data type along with related macros like FD_ZERO and FD_SET, which are part of the Berkeley sockets API for handling file descriptors.

## Table of Contents
- [Function](#Function)
- [Tools](#Tools)
- [Instructions](#Instructions)
- [Known bugs](#Known-bugs)
- [Sample outputs](#Sample-outputs)

## Function

server.c
1. Initialization steps
Hardcode users. Define status messages. Define login status and root user flags. Define message buffer to hold message input from client. Hardcode 5 "message of the day" and counter. 
2. MSGGET
When the client sends the "MSGGET" command, the server will return a message stored in the messages array along with a status 200. The function keeps track of the message sent by using a global counter that will increment after each call. Once the last message is reached in the stack, the counter will reset to 0. 
3. MSGSTORE
When the client sends the "MSGSTORE" command, the server will check if the user is logged in and return a status 200 unless the user is not logged in which it will return an error status. It will then call the recv function which is a blocking system call. It will wait for the client to send a message and then store it into the messages array. The message array has a maximum capacity of 20 messages, once exceeded it will return an error. 
4. LOGIN
When the client sends the "LOGIN user pass" command, the server will decompose the user input and store the username and password into a string data type for comparison. It will then loop through the hardcoded users and password to check if the input matches. If the user is matched, then a login status flag is set. If the user logs in with the root user/pass, then a flag will be set that the user is the root user. The server will send a status 200 if successful. Otherwise an error status is returned.
5. LOGOUT
When the client sends the "LOGOUT" command, the server will check if the user is logged in. If so, it will reset all flags and send a status 200. If not, it will send an error status.
6. QUIT
When the client sends the "QUIT" command, the server will reset all flags and send a status 200.
7. SHUTDOWN
When the client sends the "SHUTDOWN" command, the server will check if the root user is logged in. If so, then it will send a status 200 and close the server socket and exit the program. If not the root user, it will send an error status. All flags are reset. In addition, the server will send a status 210 to all other users, notifying that the server is about to shutdown and gracefully terminate all clients.
8. WHO
When the client sends the "WHO" command, the server will send a status 200 along with a list of all active users. This is done by iterating through a set of active users maintained by the server and sending the relevant info to the client.
9. SEND
When the client sends the "SEND user" command, the server will check if the user is active by iterating through the client list. If the user exists the server will send a status 200, otherwise it will send a status 420. If a user is found and status 200 is sent, the server will then call the recv function (blocking system call) and wait for the client to send a message. It will then send a status 200 and the message to the respective user.
10. Structures
The server implements the ClientInfo structure to hold information about each client, such as its file descriptor, ip address, and username. It then stores this into the clientList, which is an array of the ClientInfo data structure.

client.c
1. QUIT
When the client sends the "QUIT" command, the client will wait until the server responds to acknowledge the request. The client will then close the socket by exiting the while loop.
2. SHUTDOWN
When the client sends the "SHUTDOWN" command, the client will wait until the server responds to acknowledge the request. The client will then close the socket by exiting the while loop.
3. Reset buffer
After each loop, the client will reset the buffer and the return buffer to avoid any residual data from interfering with command processing.
4. STATUS 210
When the server is about to shutdown, it will send all clients a status 210 indicating the server is about to shutdown. The client will then terminate.


## Tools
Software tools required
Putty - SSH Client
VS Code - Code editor
WinSCP - File Transfer
Global Protect - VPN

## Instructions
1. Open Putty app (or any other SSH client) and connect to login to the following server: login.umd.umich.edu. 
2. Navigate to the root directory where the source code is located and execute the Makefile using the `make` command 
3. Run the server using the following command: `./multiThreadServer`
4. Open another window of the Putty app and navigate to the root directory, then run the client using the following command (note: the IP address provided is a local host address for testing): `./sclient 127.0.0.1` (Repeat this step for multiple client connections)
5. In the client window, send any of the following commands: MSGGET, MSGSTORE, LOGIN, LOGOUT, QUIT, SHUTDOWN, WHO, SEND.

## Known bugs

## Sample outputs
1. C: MSGGET, S: 200 OK, S: Message 1
2. C: LOGIN john john01, S: 200 OK
3. C: MSGSTORE, S: 200 OK, C: New message, S: 200 OK (while logged in)
4. C: WHO, S: The list of active users: root 127.0.0.1
5. C1: Send john, S: 200 OK, C1: hello john, S: 200 ok you have a new message from mary, S: hello mary