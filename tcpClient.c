#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sqlite3.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define PORT 2024


char COOKIE[20];
char backupCommand[20];

void exitServer(int clientSocket) {
    close(clientSocket);
    printf("[!]Disconnected from server.\n");
    exit(1);
}

void help() {
    printf ("\t..HELP..\n");
    printf("\t*Do not use capital letters for typing the commands\n");
    printf("\t*Password feature not working yet.Soon on next update.\n");
    printf("\t*You can send messages even to offline users\n");
    printf("\t*If you receive a message while you're offline you can check it once you log in\n");
    printf("\tAll messages/usernames are stored into database*\n");
    printf("\t*That's all:)\n\n\n");
}


int readFirstCommand() {
    char cmd[1024];
    bzero(cmd, 1024);
    
    printf(" -> ");

    scanf("%s", cmd);

    if (strcmp (cmd, "register") == 0) {
        return 0;
    }

    if (strcmp(cmd, "login") == 0) {
        return 1;
    }

    if (strcmp(cmd, "exit") == 0) {
        return 2;
    }    
   
    return 3;
}

void handleCommands_1(int clientSocket) {
    int answear;
    int caz;
    char username[20];

    char cmd[20];
    
    LOOP:
    caz =  readFirstCommand();
    
    switch (caz) {
    case 0:
        printf("CLIENT : REGISTER\n");
        strcpy (cmd, "register");
        if (send(clientSocket, cmd, 20, 0) < 0) {
            printf ("[!] : Eroare trimitere Register<->Username la server.\n");
            exit(1);
        }
        REGISTERLOOP://loop pentru comanda register

        printf(" -> Type your username : ");
        //*********************
        //printf(" -> Type your password : ");
        




        //*********
       
        bzero(username, 20);
        scanf("%s", username);
       
        //TRIMIT useername LA SERVER   
        if (send(clientSocket, username, strlen(username), 0) < 0) {
            printf ("[!] : Eroare trimitere Register<->Username la server.\n");
            exit(1);
        }

        //primesc de la server validarea username-ului
        if (recv (clientSocket, &answear, sizeof(int), 0) < 0) {
            printf ("[!] : Eroare la primiterea rasp ptr validarea register-ului");
            exit (1);
        }


        //daca answear =1 ->utilizatorul deja exista in baza de date
        if (answear) {
            printf("This username is already in use :( Try again.\n");
            goto REGISTERLOOP;
        }
        else {
            printf("Register status : success! :)\n");
            
            strcpy(COOKIE, username);
            
            goto ELOOP;
        }
        goto ELOOP;
    
    case 1:
        strcpy (cmd, "login");
        if (send (clientSocket, cmd, 20, 0) < 0) {
            printf ("[!] : Eroare trimitere Register<->Username la server.\n");
            exit(1);
        }
        
        //loop pentru comanda login
        LOGINLOOP:
        printf(" -> Type your username : ");
        //******
        
        //printf(" -> Type your password : ");
        
        

        //******

        bzero(username, 20);
        scanf("%s", username);
        
        //TRIMIT UTILIZATOR LA SERVER
        if (send (clientSocket, username, strlen(username), 0) < 0) {
            printf ("Client : Eroare trimitere Login<->Username la server.\n");
            exit(1);
        }


        if (recv (clientSocket, &answear, sizeof(int), 0) < 0) {
            printf ("[!] : Client : Eroare la primiterea rasp ptr validarea login-ului");
            exit (1);
        }
        //daca answear = 0 -> username-ul nu exista in baza de date
        if (!answear) {
            printf("This username does not exits :( Try again.\n");
            goto LOGINLOOP;
        }
        else {
            printf("*System : Login status : success! :)\n");
            strcpy(COOKIE, username);
            goto ELOOP;
        }
        goto ELOOP;
    
    case 2:
        strcpy (backupCommand, "EXIT");
            
        if (send (clientSocket, "exit", 4, 0) < 0) {
            printf ("[!] : Error in disconnecting from the server\n");
            exitServer(clientSocket);    
        }
        goto ELOOP;
    
    default:
        printf("Unknown command.Try again.\n");
        goto LOOP;
    }

    ELOOP:
        printf("\n");
        //do nothing
}

void checkHistory (int clientSocket) {
    printf("Do you want to list the existing users?Type yes or no..\n");



//bla bla get av users

    char x[4];
    printf("-> ");
    scanf("%s", x);
    getchar();

    send(clientSocket, x, 4 * sizeof(char), 0);

        if (strcmp(x, "yes") == 0) {
            
        int countAvailableUsers = 0;
        char availableUser[100];
        if (recv(clientSocket, &countAvailableUsers, 4, 0) < 0) {
            printf("[!] : Error in receiving countAvailableUsers.\n");
            exit(1);
        }
        
        printf("There are %d available users at the moment.\n", countAvailableUsers);

        for (int i = 0; i < countAvailableUsers; i++) {
            if (recv(clientSocket,availableUser , 100, 0) < 0) {
                printf("[!] : Error in receiving availableUser.\n");
                exit(1);
            }
            printf ("* %s\n", availableUser);
        }
        
        printf("Type one username.\n");
        
        printf("-> ");
        char username[20];
        scanf("%s", username);
        getchar();
        
        send(clientSocket, username, 20, 0);
        
        send(clientSocket, COOKIE, 20, 0);
        
        int msgCounter = 0;
        if (recv(clientSocket, &msgCounter, sizeof(int), 0) < 0) {
            printf("[!] : Error in receiving the msgCounter.\n");
            exit(1);
        }
    
        if (msgCounter == 0) {
            printf("There are now messages between you and user %s\n", username);
        }
        else {
            printf("The conversation between you and user %s contains %d messages.\n", username, msgCounter);
            printf("\t*This is the history*\n");
        }

        char **date = (char **)malloc (100 * sizeof(char*));
        char **senders = (char **)malloc (100 * sizeof(char*));
        char **receivers = (char **)malloc (100 * sizeof(char*));
        char **mssg = (char **)malloc (100 * sizeof(char*));

        for (int i = 0; i < msgCounter; i++) {    
            receivers[i] = (char *)malloc (100 * sizeof (char));
            senders[i] = (char *)malloc (100 * sizeof (char));
            mssg[i] = (char *)malloc (100 * sizeof (char));
            date[i] = (char *)malloc (100 * sizeof (char));

            if (recv (clientSocket, senders[i], 100 * sizeof (char), 0) < 0) {
                printf ("Eroare la primirea  sender de la server.\n");
                exit (1);
            }
            if (recv (clientSocket, receivers[i], 100 * sizeof (char), 0) < 0) {
                printf ("Eroare la primirea  receiver de la server.\n");
                exit (1);
            }
            if (recv (clientSocket, mssg[i], 100 * sizeof (char), 0) < 0) {
                printf ("Eroare la primirea  mesajului de la server.\n");
                exit (1);
            }
            if (recv (clientSocket, date[i], 100 * sizeof(char), 0) < 0) {
                printf ("Eroare la primirea msg date de la server.\n");
                exit(1);
            }

            printf ("\tFROM : %s -> TO : %s : %s *at : %s\n", senders[i], receivers[i], mssg[i], date[i]);
        }    

    }
    else {
        send(clientSocket, x, 4, 0);
    } 
}

void checkInbox (int clientSocket) {
    if (send (clientSocket, COOKIE, 20 * sizeof (char), 0) < 0) {
        printf("[!] : client -> server cookie error.\n");
        exit(1);
    }

    int msgCounter = 0;
    if (recv(clientSocket, &msgCounter, 4, 0) < 0) {
        printf("[!] : Error in receiving the msgCounter.\n");
        exit(1);
    }
    
    if (msgCounter == 0) {
        printf("You've got no new messages.\n");
    }
    else {
        printf("You've got %d new messages.\n", msgCounter);
    }

    char **senders = (char **)malloc (100 * sizeof(char*));
    char **mssg = (char **)malloc (100 * sizeof(char*));
    char **date = (char **)malloc (100 * sizeof(char*));
    
    printf("\n");
    for (int i = 0; i < msgCounter; i++) {    
        senders[i] = (char *)malloc (100 * sizeof (char));
        mssg[i] = (char *)malloc (100 * sizeof (char));
        date[i] = (char *)malloc (100 * sizeof (char));
        
        if (recv (clientSocket, senders[i], 100 * sizeof (char), 0) < 0) {
            printf ("Eroare la primirea  sender[i] de la server.\n");
            exit (1);
        }
        if (recv (clientSocket, mssg[i], 100 * sizeof (char), 0) < 0) {
            printf ("Eroare la primirea  mssg[i] de la server.\n");
            exit (1);
        }
        
        if (recv (clientSocket, date[i], 100 * sizeof (char), 0) < 0) {
            printf ("Eroare la primirea  date[i] de la server.\n");
            exit (1);
        }    

        printf ("\tYou've got a new message from -> %s : %s at : %s \n", senders[i], mssg[i], date[i]);
    }
} 

void sendMessage(int clientSocket) {
    int countAvailableUsers;

    if (recv (clientSocket, &countAvailableUsers, sizeof(int), 0) < 0) {
        printf ("[!] : Error in receiving countAvailableUsers from server!\n");
        exit (1);
    }

    char availableUser[100];

    printf("\tThere are %d available users(online/offline).\n", countAvailableUsers);

    for (int i = 0; i < countAvailableUsers; i++) {
        bzero (availableUser, 100);

        if (recv (clientSocket, availableUser, 100, 0) < 0) {
            printf ("[!] : Error in receiving availableUser from server!\n");
            exit (1);
        }

        printf("\t* %s\n", availableUser);
    }


    printf("\n*Choose the one you want to send a message to\n");
    printf(" -> ");


    char username[20];

    LOOP:
    scanf("%s", username);
    getchar();
    printf("\n");

    if (strcmp (COOKIE, username) == 0) {
        printf("You cannot send a message to yourself!!Choose another username.\n");
        printf(" -> ");
        goto LOOP;
    }
    else
    {
        goto ELOOP;
    }
    
    ELOOP:
    printf ("Type the message below\n");
    printf(" -> ");
    
    char message[2024];
    fgets(message, 2024, stdin);
    message[strlen(message) - 1] = NULL;
    
    if (send (clientSocket, COOKIE, 20, 0) < 0) {
        printf ("[!] : Error in sending the COOKIE to the server!\n");
        exit (1);
    }
    
    if (send (clientSocket, username, 20, 0) < 0) {
        printf ("[!] : Error in sending the username to the server!\n");
        exit (1);
    }
    
    if (send (clientSocket, message, 2024, 0) < 0) {
        printf ("[!] : Error in sending the message to the server!\n");
        exit (1);
    }
}



void handleCommands_2(int clientSocket) {
    if (strcmp(backupCommand, "EXIT") == 0) {
        printf("[!]Disconnected from server.\n");
        exit(1);
    }


    char buffer[2024];
    getchar();
    while (1) {  
        printf("\tType bellow one of the displayed commands:)\n");
        printf(" -> check inbox\n");
        printf(" -> check history\n");
        printf(" -> send message\n");
        printf(" -> help\n");
        printf(" -> exit\n");
        printf("\n");
        printf(" -> ");

        fgets(buffer, 2024, stdin);
        printf("\n");
        buffer[strlen(buffer) - 1] = NULL;
        
        send(clientSocket, buffer, 2024, 0);
        
        
        if (strcmp(buffer, "check inbox") == 0) {
            printf(" ...Checking inbox...\n");
            checkInbox(clientSocket);
        }
        else {
            if (strcmp(buffer, "check history") == 0) {
                checkHistory(clientSocket);
            }
            else {
                if (strcmp(buffer, "send message") == 0) {
                    printf(" ...Sending message...\n");
                    sendMessage(clientSocket);
                }
                else {
                    if (strcmp(buffer, "help") == 0) {
                        help();
                    } 
                    else {
                        if (strcmp(buffer, "exit") == 0) {
                        exitServer(clientSocket);
                        }
                        else {
                            printf("Unknown command.Try again!\n");
                        }
                    }
                }
            }
        }
    } 
}


int main() {
    int clientSocket, connectionResult;
    struct sockaddr_in serverAddress;
    
    strcpy (backupCommand, "DEFAULT");

    clientSocket = socket(AF_INET, SOCK_STREAM, 0);

    if (clientSocket < 0) {
        printf("[!] : Error in establishing the socket :(.\n");
        exit(1);
    }

    memset(&serverAddress,'\0', sizeof(serverAddress));

    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = inet_addr("127.0.0.1");

    connectionResult = connect(clientSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    if (connectionResult < 0) {
        printf("[!] : Error in connecting to the server :(\n");
        exit(1);
    }

    printf("[+]Connected to Server.\n");

    printf(" What do you want to do?\n");
    printf(" -> register\n");
    printf(" -> login\n");
    printf(" -> exit\n");
    printf("\tType bellow one of the displayed commands\n");

    
    handleCommands_1(clientSocket);
    handleCommands_2(clientSocket); 

    return 0;    
}