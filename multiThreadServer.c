/*
 * multiThreadServer.c -- a multithreaded server
 */

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <cstring>
#include <cstdlib>
#include <set>

using namespace std;

#define PORT 2477  // port we're listening on
#define MAX_LINE 256
#define MAX_CLIENTS 100

fd_set master;   // master file descriptor list
int listener;    // listening socket descriptor
int fdmax;

//define structure for users
struct Users {
  char username[20];
  char password[20];
};
struct Users users[4];

//define structure for clients
struct ClientInfo {
  int fd;
  string username;
  char ip_address[16];
};

//create an array to store ClientInfo data type
struct ClientInfo clientList[MAX_CLIENTS];
int numClients = 0; //keep track of connect clients

//create a set to keep track of active file descriptors
set<int> activeFD;

// *********************** Initialization Steps ******************************** //
  char status200[] = "200 OK\n";
  char status210[] = "210 the server is about to shutdown .....\n";
  char status300[] = "300 message format error\n";
  char status401[] = "401 You are not currently logged in, login first.\n";
  char status402[] = "402 User not allowed to execute this command.\n";
  char status410[] = "410 Wrong UserID or Password\n";
  char status411[] = "411 MSGSTORE Full.\n";
  char* msgs[20];
  char newMessageBuffer[MAX_LINE]; // Buffer used to store char input for MSGSTORE
  int messageLen; // Store number of bytes from recv
  int msgGetCount = 0; 
  int nextAvailableSlot = 5;
  
// *************************************************************************** //

// the child thread
void *ChildThread(void *newfd) {
  char buf[MAX_LINE];
  int nbytes;
  int i, j;
  int childSocket = (long) newfd;


  bool loginStatus = false;
  bool root = false;

  while(1) {

    // handle data from a client
    if ((nbytes = recv(childSocket, buf, sizeof(buf), 0)) <= 0) {
      // got error or connection closed by client
      if (nbytes == 0) {
        // connection closed
        cout << "multiThreadServer: socket " << childSocket <<" hung up" << endl;

        activeFD.erase(childSocket); //remove file descriptor from active list
        memset(&clientList[childSocket], 0, sizeof(ClientInfo)); //delete client info
        numClients--; // decrement number of clients
      } else {
        perror("recv");
      }
      close(childSocket); // bye!
      FD_CLR(childSocket, &master); // remove from master set
      pthread_exit(0);
    } else {
      
      // we got some data from a client
      cout << buf;

      //******************************   MSGGET *************************************** //
      if (strcmp(buf, "MSGGET\n") == 0) { //MSGGET

        if (msgGetCount >= nextAvailableSlot) {
          msgGetCount = 0; // Reset back to the first message
        }
        send (childSocket, status200, strlen(status200), 0); //send status successful
        send (childSocket, msgs[msgGetCount], strlen(msgs[msgGetCount]) + 1, 0); // send message in queue
        msgGetCount++; // point to next message
      }

      //******************************   MSGSTORE *************************************** //
      else if (strcmp(buf, "MSGSTORE\n") == 0) { 

        //check if logged in, if not send back 401, else ...
        if (!loginStatus) {
          send (childSocket, status401, strlen(status401), 0); // send error status
        } else if (nextAvailableSlot == 20) {
          send (childSocket, status411, strlen(status411), 0); // if MSGSTORE storage is full, send error
        } else {
          send (childSocket, status200, strlen(status200), 0); //send status successful
          
          memset(newMessageBuffer, 0, sizeof(newMessageBuffer)); // reset the array
          
          messageLen = recv(childSocket, newMessageBuffer, sizeof(newMessageBuffer), 0); // receive data from client
          newMessageBuffer[messageLen] = '\0';
          strcpy(msgs[nextAvailableSlot], newMessageBuffer); // store client message into next available slot
          nextAvailableSlot++; // move to next available slot
          send (childSocket, status200, strlen(status200), 0); //send status successful
        }
      }

      //******************************   LOGIN *************************************** //
      else if (strncmp(buf, "LOGIN", 5) == 0) { 

        //string manipulation to extract username and password from user input
        string input(buf);
        string username = input.substr(6, input.find(" ", 6) - 6); // extract and store username
        string password = input.substr(input.find(" ", input.find(" ") + 1) + 1); // extract and store password
        password.erase(password.find_last_not_of("\n") + 1); // remove newline character (\n)

        //create string datatypes and hold char for later calculation
        for (int i = 0; i < 4; i++) {
          string tempUser = users[i].username;
          string tempPassword = users[i].password;
          
          //check if username and password matches idatabase
          if (username == tempUser && password == tempPassword) {
            //check if root user/pass
            if(i == 0) {
              root = true;
            }
            
            // add username to existing client info
            struct ClientInfo *client = &clientList[childSocket];
            client->username = username;
            cout << "my fd: " << client->fd << endl; //debug
            cout << "my IP: " << client->ip_address << endl; //debug
            cout << "my username: " << client->username << endl; //debug

            loginStatus = true; // set login status to true for remaining session
            send (childSocket, status200, strlen(status200), 0); //send status successful   
            break; // Exit the loop since we found a match
          }
        }

        if (!loginStatus) {
          send (childSocket, status410, strlen(status410), 0); //send error
        }   
      }

      //******************************   LOGOUT *************************************** //
      else if (strcmp(buf, "LOGOUT\n") == 0) { 

        //check loginStatus to ensure user is logged in, if true then logout user and if not send error
        if (!loginStatus) {
          send (childSocket, status401, strlen(status401) + 1, 0);
        } else {

          //remove username from existing client info
          struct ClientInfo *client = &clientList[childSocket];
          client->username = "Unknown";
          cout << "my fd: " << client->fd << endl; //debug
          cout << "my IP: " << client->ip_address << endl; //debug
          cout << "my username: " << client->username << endl; //debug

          // reset flags
          loginStatus = false;
          root = false;
          send (childSocket, status200, strlen(status200), 0); //send status successful  
        }
      }

      //******************************   QUIT *************************************** //
      else if (strcmp(buf, "QUIT\n") == 0) {

        activeFD.erase(childSocket); //remove file descriptor from active list
        memset(&clientList[childSocket], 0, sizeof(ClientInfo)); //delete client info
        numClients--; // decrement number of clients

        //reset root and loginStatus to false, then send successful message
        root = false;
        loginStatus = false;
        send (childSocket, status200, strlen(status200), 0); //send status successful

      }

      //******************************   SHUTDOWN *************************************** //
      else if (strcmp(buf, "SHUTDOWN\n") == 0) {
        //check if root user is logged in, if so then send successful status and close/terminate server
        if(!root) {
          send (childSocket, status402, strlen(status402), 0);
        } else if (root){
          send (childSocket, status200, strlen(status200), 0); //send status successful

          //broadcast to all other clients to inform them the server will shutdown
          for(j = 0; j <= fdmax; j++) {
            // send to everyone!
            if (FD_ISSET(j, &master)) {
              // except the listener and ourselves
              if (j != listener && j != childSocket) {
                if (send(j, status210, strlen(status210), 0) == -1) {
                  perror("send");
                } else {
                  close(j); // bye!
                  FD_CLR(j, &master); // remove from master set
                }
              }
            }
          }

          cout << "Shutting down..." << endl;
          close(childSocket);
          FD_CLR(childSocket, &master);
          close(listener);
          exit(1);
          

        } else {
          send (childSocket, status300, strlen(status300), 0); //send status successful
        }
        //reset root and loginStatus to false
        root = false;
        loginStatus = false;
      }

      //******************************   WHO *************************************** //
      else if (strcmp(buf, "WHO\n") == 0) {
        send (childSocket, status200, strlen(status200), 0); //send status successful

        char msg1[30];
        strcpy(msg1, "The list of the active users: \n");
        send (childSocket, msg1, strlen(msg1), 0);

        //loop to iterate through the set of all active file descriptors
        for (set<int>::iterator it = activeFD.begin(); it != activeFD.end(); ++it) {
          int i = *it;
          struct ClientInfo *client = &clientList[i]; //access clientList
          string temp = client->username  + "      " + client->ip_address + "\n"; //concat client info
          const char *msg2 = temp.c_str(); //convert to type char
          send (childSocket, msg2, strlen(msg2), 0); //send to client
        }
      }

//********** Send to everyone *************//
      // for(j = 0; j <= fdmax; j++) {
      //   // send to everyone!
      //   if (FD_ISSET(j, &master)) {
      //     // except the listener and ourselves
      //     if (j != listener && j != childSocket) {
      //       if (send(j, buf, nbytes, 0) == -1) {
      //         perror("send");
      //       }
      //     }
      //   }
      // }

      

    }
  }
}


int main(void) {
  struct sockaddr_in myaddr;     // server address
  struct sockaddr_in remoteaddr; // client address
  int newfd;        // newly accept()ed socket descriptor
  int yes=1;        // for setsockopt() SO_REUSEADDR, below
  socklen_t addrlen;

  pthread_t cThread;

  FD_ZERO(&master);    // clear the master and temp sets

  // *********************** Initialization Steps ******************************** //
  /* Message of the day - hardcode */
    for(int i = 0; i < 20; i++) {
      msgs[i] = (char *)malloc(50 * sizeof(char)); // Allocate memory for each string
    }
    strcpy(msgs[0], "Message 1\n");
    strcpy(msgs[1], "Message 2\n");
    strcpy(msgs[2], "Message 3\n");
    strcpy(msgs[3], "Message 4\n");
    strcpy(msgs[4], "Message 5\n");

    /* Users - hardcode */
    strcpy(users[0].username, "root");
    strcpy(users[0].password, "root01");
    strcpy(users[1].username, "john");
    strcpy(users[1].password, "john01");
    strcpy(users[2].username, "david");
    strcpy(users[2].password, "david01");
    strcpy(users[3].username, "mary");
    strcpy(users[3].password, "mary01");

    // *************************************************************************** //

  // get the listener
  if ((listener = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("socket");
    exit(1);
  }

  // lose the pesky "address already in use" error message
  if (setsockopt(listener, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
    perror("setsockopt");
    exit(1);
  }

  // bind
  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = INADDR_ANY;
  myaddr.sin_port = htons(PORT);
  memset(&(myaddr.sin_zero), '\0', 8);
  if (bind(listener, (struct sockaddr *)&myaddr, sizeof(myaddr)) == -1) {
    perror("bind");
    exit(1);
  }

  // listen
  if (listen(listener, 10) == -1) {
    perror("listen");
    exit(1);
  }

  // add the listener to the master set
  FD_SET(listener, &master);

  // keep track of the biggest file descriptor
  fdmax = listener; // so far, it's this one

  addrlen = sizeof(remoteaddr);

  // main loop
  for(;;) {
    // handle new connections
    if ((newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen)) == -1) {
      perror("accept");
      exit(1);
    } else {
      FD_SET(newfd, &master); // add to master set
      cout << "multiThreadServer: new connection from " << inet_ntoa(remoteaddr.sin_addr) << " socket " << newfd << endl;

      //********* keep track of client info***************************************
      if (newfd < MAX_CLIENTS) {
        activeFD.insert(newfd); //keep track of file descriptor
        struct ClientInfo newClient; //initialize new ClientInfo object
        newClient.fd = newfd; //assign fd
        newClient.username = "Unknown"; //assign username
        char* ipString = inet_ntoa(remoteaddr.sin_addr); //create copy of client IP address using inet_ntoa
        strcpy(newClient.ip_address, ipString); //assign IP by copying above string
        clientList[newfd] = newClient;
        numClients++;

        cout << "fd: " << newClient.fd << endl; //debug
        cout << "username: " << newClient.username << endl; //debug
        cout << "IP: " << newClient.ip_address << endl; //debug
        cout << "num of clients: " << numClients << endl; //debug
      } else {
        perror("max client exceeded, try again later");
        exit(1);
      }
      
      //**************************************************************************

      if (newfd > fdmax) {    // keep track of the maximum
        fdmax = newfd;
      }

      if (pthread_create(&cThread, NULL, ChildThread, (void *)(intptr_t)newfd) <0) {
        perror("pthread_create");
        exit(1);
      }
    }

  }
  return 0;
}

