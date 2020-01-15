#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <memory>
#include <cstdlib>
#include <iostream>
#include <sqlite3.h>
#include <string>
#include <time.h>

#define PORT 2024

using namespace std;


int callback(void* NotUsed, int argc, char** argv, char** azColName);
int callbackGetUnreadMessages(void *NotUsed, int argc, char **argv, char** azColName);
int callbackGetUserID(void *NotUsed, int argc, char **argv, char** azColName);
int callbackAvailableUsers(void* NotUsed, int argc, char** argv, char** azColName);
int callbackGetUsername(void *NotUsed, int argc, char **argv, char** azColName);    
int callbackGetMaxUserID(void *NotUsed, int argc, char **argv, char** azColName);   

int updateReadStatus(const char *message);

char *getMaxUserID();
int getUSER_ID(const char *username);
int getAvailableUsers();
void getMessage(char message[]);//comunicare cu baza de date

void checkInbox(int newSocket);
void checkHistory(int newSocket);
void sendMessage(int newSocket);

int createDB();
int registerUser(const char* user_id, const char* username);


int checkUsername(const char* username);

int msgCounter = 0;
char **senders = (char **)malloc (100 * sizeof(char*));
char **receivers = (char **)malloc (100 * sizeof(char*));
char **mssg = (char **)malloc (100 * sizeof(char*));
char **date = (char **)malloc (100 * sizeof(char*));

int main() {
    int sockfd, ret;
    struct sockaddr_in serverAddr;

    int newSocket;
    struct sockaddr_in newAddr;

    socklen_t addr_size;

    char buffer[2024];
    pid_t pid;//fork in order to handle multiple clients;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        printf("[*]Error in creating socket.");
        exit(2);
    }

    printf("[+]Server socket is created.\n");

    
    memset(&serverAddr,'\0', sizeof(serverAddr));

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    ret = bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr));

    if (ret < 0) {
        printf("[!]Bind error.\n");
        exit(2);
    }

    printf("[+]Bind to port %d\n", PORT);

    if (listen(sockfd, 100) == 0) {
        //maximum 100 clients
        printf("Waiting for clients to connect..\n");
    }else {
        printf("[!]Listen error.\n");
        exit(2);
    }

    while (1) {
        newSocket = accept(sockfd, (struct sockaddr*)&newAddr, &addr_size);

        if (newSocket < 0) {
            exit(2);
        }
        printf("New connection established from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));

        if ((pid = fork()) == 0) {
            close(sockfd);
            char command[20];
            int answear;
            char username[20];
            
            LOOP:
            
            bzero (command, 20);
            if (recv (newSocket, command, 20 * sizeof (char), 0) < 0) {
                printf ("Error in receving the first commnad from client.\n");
                exit (1);
            }

            printf ("Am primit comanda : %s\n", command);

            if (strcmp (command, "register") == 0) {
                printf("MATCH PE REGISTER\n");
                RLOOP:
                bzero (username, 20);
                if (recv (newSocket, username, 20 * sizeof (char), 0) < 0) {
                    printf ("Error in receving the username for register from client.\n");
                    exit (1);
                }
                answear = 0;
                answear = checkUsername (username);

                printf ("REGISTER ANSWEAR : %d\n", answear);

                if (send (newSocket, &answear, sizeof (int), 0) < 0) {
                    printf ("Error in sending register status to client\n");
                    exit (1);
                }

                if (answear) {
                    printf ("GOTO RLOOP\n");
                    goto RLOOP;
                }
                else {
                    char *newUserID = getMaxUserID();

                    int x = atoi (newUserID);
                    x++;
                    sprintf(newUserID, "%d", x);

                    if (registerUser (newUserID, username) < 0) {
                        printf("Failed to register.\n");
                    }
                    printf ("GOTO EELOOP\n");
                    goto EELOOP;
                }
            }
            
            if (strcmp (command, "login") == 0) {
                printf("MATCH PE LOGIN\n");
                LLOOP:
                bzero (username, 20);
                if (recv (newSocket, username, 20 * sizeof (char), 0) < 0) {
                    printf ("Error in receving the username for login from client.\n");
                    exit (1);
                }
                answear = 0;
                answear = checkUsername (username);

                printf ("LOGIN ANSWEAR : %d\n", answear);

                if (send (newSocket, &answear, sizeof (int), 0) < 0) {
                    printf ("Error in sending login status to client\n");
                    exit (1);
                }

                if (answear == 0) {//username does not exists
                    goto LLOOP;
                }
                else {
                    goto EELOOP; //username exits
                }
            }

            if (strcmp (command, "exit") == 0) {
                printf("MATCH PE EXIT\n");
                printf("Client disconnected from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
                break;
                
            }

            EELOOP:             
            while (1) {
                
                char buffer[2024];
                //CUSTOMIZED RECV***
                //recvl(newSocket, buffer);
                recv(newSocket, buffer, 2024, 0);
                
                printf("WHAT I RECEIVED FROM CLIENT : %s\n", buffer);
                
                int r = strlen(buffer);
                printf(" STRLEN(%s) : %d\n", buffer, r);
                
                if (strcmp(buffer, "exit") == 0) {
                    printf("Client disconnected from %s:%d\n", inet_ntoa(newAddr.sin_addr), ntohs(newAddr.sin_port));
                    break;
                }
                if (strcmp (buffer, "check inbox") == 0){
                    printf("SERVER CHECK INBOX\n");   
                    checkInbox(newSocket);
                    
                    //printf("Client: %s\n", buffer);
                    //break;
                    //send(newSocket, buffer, strlen(buffer), 0);
                    bzero(buffer, sizeof(buffer));
                }
                if (strcmp (buffer, "check history") == 0) {
                    printf("SERVER : CHECKING HISTORY...\n");
                    checkHistory(newSocket);
                }
                if (strcmp (buffer, "send message") == 0) {
                    printf("SERVER : SENDING MESSAGE...\n");
                    sendMessage(newSocket);
                }   
            }  
        }
    }
    close(newSocket);

    return 0;
}


char *user_name = (char *)malloc(100);
int getUSERNAME(const char *user_id) {
    sqlite3* db;
    bzero(user_name, 100);
    int exit = sqlite3_open("CHAT.db", &db);

    string sql = "SELECT USERNAME FROM USERS WHERE USER_ID = '";

    sql = sql + user_id;
    sql = sql + "';";
    
    sqlite3_exec(db, sql.c_str(), callbackGetUsername, NULL, NULL);

    return 0;
}

int callbackGetUsername(void *NotUsed, int argc, char **argv, char** azColName) {    
    strcpy (user_name, argv[0]);
    return 0;
}

char *user_id = (char *)malloc(100);
int getUSER_ID(const char *username) {
    sqlite3* db;
    bzero(user_id, 100);
    int exit = sqlite3_open("CHAT.db", &db);

    string sql = "SELECT USER_ID FROM USERS WHERE USERNAME = '";

    sql = sql + username;
    sql = sql + "';";
    
    sqlite3_exec(db, sql.c_str(), callbackGetUserID, NULL, NULL);

    return 0;
}

int callbackGetUserID(void *NotUsed, int argc, char **argv, char** azColName) {    
    for (int i = 0; i < argc; i++) {
        strcpy (user_id, argv[i]);
        printf("%s\n", argv[i]);
    }
    return 0;
}


int getUNREAD_MESSAGES(const char *userID) {
    sqlite3* db;
    int exit = sqlite3_open("CHAT.db", &db);

    string sql = "SELECT SENDER_ID, TEXT, DATE FROM MESSAGES WHERE RECEIVER_ID = '";
    sql = sql + userID;
    sql = sql + "' AND READ_STATUS ='UR' ORDER BY DATE ASC;";
    
    cout << "SQL COMMAND IS : " << sql << '\n';

    sqlite3_exec(db, sql.c_str(), callbackGetUnreadMessages, NULL, NULL);

    return 0;
}

int callbackGetUnreadMessages(void *NotUsed, int argc, char **argv, char** azColName) {    
    getUSERNAME(argv[0]);
    
    cout << "\t*You've got a new message from -> " << user_name << " : " << argv[1] << endl;
    
    senders[msgCounter] = (char *)malloc (100 * sizeof (char));
    mssg[msgCounter] = (char *)malloc (100 * sizeof (char));
    date[msgCounter] = (char *)malloc (100 * sizeof (char));
    
    getUSERNAME(argv[0]);
    strcpy(senders[msgCounter], user_name);
    
    strcpy(mssg[msgCounter], argv[1]);
    strcpy(date[msgCounter], argv[2]);
    
    msgCounter++;

    return 0;
} 

int updateReadStatus(const char *receiver) {
    sqlite3* DB;
    char *messageError;

    int exit = sqlite3_open("CHAT.db", &DB);
    
    string sql("UPDATE MESSAGES SET READ_STATUS = 'R' WHERE RECEIVER_ID='");
    sql = sql + receiver;
    sql = sql + "';";
    
    printf ("#updateReadStatu : command is : ");
    cout << sql << '\n';

    exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &messageError);
   
    if (exit != SQLITE_OK) {
        cerr << "EROARE LA INSERAREA MESAJULUI IN BAZA DE DATE.\n";
        sqlite3_free(messageError);
    }
    else {
        cout << "SUCCESS.\n";
    }
    return 0;
}
/*
int callback1(void *NotUsed, int argc, char **argv, char** azColName);
int selectData(char* id) {
    sqlite3* db;
    int exit = sqlite3_open("CHAT.db", &db);

    string sql = "SELECT TEXT, READ_STATUS FROM MESSAGES WHERE RECEIVER_ID='";
    sql = sql + id;
    sql = sql + "';";

    cout << "SELECTDATA : " << sql << '\n';
    
    sqlite3_exec(db, sql.c_str(), callback1, NULL, NULL);

    return 0;
}

int callback1(void *NotUsed, int argc, char **argv, char** azColName) { 
    for (int i = 0; i < argc; i++) {
        //column name and value1`
        cout << azColName[i] << ": " << argv[i] << endl;
    }
    cout << endl;

    return 0;
} 
*/
//(****************)

void checkInbox(int newSocket) {
    printf("SEVER INSIDE checkInbox\n");
    
    msgCounter = 0;
    
    char username[20];
    if (recv (newSocket, username, 20 * sizeof (char), 0) < 0) {
        printf ("server <- client cookie error.\n");
        //exit (1);
    }

    printf("******THE MSG COUNTER IS : %d\n\n", msgCounter);

    getUSER_ID (username);
    char userID[20];
    strcpy (userID, user_id);// GOT USER_ID FOR GIVEN USERNAME

    getUNREAD_MESSAGES (userID);

    updateReadStatus(userID);

    printf("SERVER : TRIMIT MSGCOUNTER = %d\n", msgCounter);

    if (send(newSocket, &msgCounter, sizeof(msgCounter), 0) < 0) {
        printf("[*]Error in sending the msgCounter.\n");
    }

    for (int i = 0; i < msgCounter; i++) {
        printf ("Trimit mesajul : %s *de la user-ul : %s\n", mssg[i], senders[i]);
        if (send (newSocket, senders[i], 100 * sizeof (char), 0) < 0) {
            printf ("eroare la cominucarea autorului mesajului catre client.\n");
            exit(1);
        }
        free(senders[i]);
        if (send (newSocket, mssg[i], 100 * sizeof (char), 0) < 0) {
            printf ("eroare la cominucarea mesajului catre client.\n");
            exit(1);
        }
        free(mssg[i]);

        if (send (newSocket, date[i], 100 * sizeof (char), 0) < 0) {
            printf ("eroare la cominucarea message date catre client.\n");
            exit(1);
        }
        free(date[i]);
    }
}


int createDB() {
    sqlite3* DB;
    int rc = 0;

    rc = sqlite3_open("CHAT.db", &DB);

    if (rc != SQLITE_OK) {
        cerr << "CREATE ERROR.\n";

        sqlite3_close(DB);

        exit(EXIT_FAILURE);
    }
    
    sqlite3_close(DB);

    printf("CREATED/OPENED SUCCESSFULLY");

    return 0;
}


int registerUser(const char* user_id, const char* username) {
    sqlite3* DB;
    char *messageError;

    int exit = sqlite3_open("CHAT.db", &DB);
    
    string sql("INSERT INTO USERS (USER_ID, USERNAME) VALUES ('");
    sql = sql + user_id;
    sql = sql + "', '";
    sql = sql + username;
    sql = sql + "');";


    exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &messageError);
    if (exit != SQLITE_OK) {
        cerr << "REGISTER_STATUS : FAILURE!\n";
        sqlite3_free(messageError);

        return -1;
    }
    else {
        printf ("REGISTER_STATUS : SUCCESS!\n");
        return 1;
    }

    return 0;
}


int insertMessage(const char* sender, const char* receiver, const char *message) {
    sqlite3* DB;
    char *messageError;

    int exit = sqlite3_open("CHAT.db", &DB);
    

    string msg_id;
    
    char *SENDER_ID = (char *) malloc (4);
    char *RECEIVER_ID = (char *) malloc (4);
    
    getUSER_ID(sender);
    strcpy (SENDER_ID, user_id);
    
    getUSER_ID(receiver);
    strcpy (RECEIVER_ID, user_id);
    
    char *ch = (char *) malloc (1);
    strcpy (ch, "_");
     

    if (strcmp (SENDER_ID, RECEIVER_ID) < 0) {
        msg_id = msg_id + SENDER_ID;
        msg_id = msg_id + ch;
        msg_id = msg_id + RECEIVER_ID;
    }
    else {
        msg_id = msg_id + RECEIVER_ID;
        msg_id = msg_id + ch;
        msg_id = msg_id + SENDER_ID;
    }    

    time_t now = time(0);
    string dt = ctime(&now);//current time

    dt.pop_back();

    string sql("INSERT INTO MESSAGES (MESSAGE_ID, SENDER_ID, RECEIVER_ID, TEXT, READ_STATUS, DATE) VALUES ('");
    
    sql = sql + msg_id;
    sql = sql + "', '";
    sql = sql + SENDER_ID;
    sql = sql + "', '";
    sql = sql + RECEIVER_ID;
    sql = sql + "', '";
    sql = sql + message;
    sql = sql + "', 'UR', '";
    sql = sql + dt;
    sql = sql + "');";
    
    cout << "COMANDA ESTE : " << sql.c_str() << '\n';

    exit = sqlite3_exec(DB, sql.c_str(), NULL, 0, &messageError);
    if (exit != SQLITE_OK) {
        cerr << "EROARE LA INSERAREA MESAJULUI IN BAZA DE DATE.\n";
        sqlite3_free(messageError);
    }
    else {
        cout << "SUCCESS.\n";
    }

    return 0;
}

int countAvailableUsers = 0;
char **availableUsers = (char **)malloc (100 * sizeof(char*));
int getAvailableUsers() {
    sqlite3* db;
    countAvailableUsers = 0;
    int exit = sqlite3_open("CHAT.db", &db);

    string sql = "SELECT USERNAME FROM USERS;";
    
    //printf("\n Available users are\n");

    sqlite3_exec(db, sql.c_str(), callbackAvailableUsers, NULL, NULL);

    return 0;
}

int callbackAvailableUsers(void *NotUsed, int argc, char **argv, char** azColName) {    
    //cout << "\t* " << argv[1] << endl;
        
    availableUsers[countAvailableUsers] = (char *)malloc (20 * sizeof (char));
    strcpy(availableUsers[countAvailableUsers], argv[0]);
    countAvailableUsers++;

    return 0;
} 

void sendMessage(int newSocket) {
    countAvailableUsers = 0;
    getAvailableUsers();
    
    cout << "sendMessage : Trimit " << countAvailableUsers << '\n';
    
    if (send (newSocket, &countAvailableUsers, 4, 0) < 0) { 
        printf("[*]Error in sending the countAvailableUsers.\n");
        exit(1);
    }
    cout << "trimis\n\n"; 
    
    for (int i = 0; i < countAvailableUsers; i++) {
        if (send (newSocket, availableUsers[i], 100, 0) < 0) { 
            printf("[*]Error in sending the AvailableUsers[%d].\n", i);
            exit(1);
        }
        free(availableUsers[i]);
    }
    countAvailableUsers = 0;

    
    //receiving the username and the message for that specific username...
    char sender[20];
    char receiver[20];
    char message[2024];
    
    if (recv (newSocket, sender, 20, 0) < 0) {
        printf("Eroare la primirea COOKIE-ului de la client\n");
        exit(1);
    }
    if (recv (newSocket, receiver, 20, 0) < 0) {
        printf("Eroare la primirea destinatarului mesajului de la client\n");
        exit(1);
    }
    if (recv (newSocket, message, 2024, 0) < 0) {
        printf("Eroare la primirea mesajului de la client\n");
        exit(1);
    }
    
    insertMessage(sender, receiver, message);

}
        
int checkUsername(const char* username) {
    countAvailableUsers = 0;
    getAvailableUsers();
    printf("** CHECKING USERNAME**\n");

    for (int i = 0; i < countAvailableUsers; i++) {
        if (strcmp (availableUsers[i], username) == 0) {
            return 1;
        }
    }

    //countAvailableUsers = 0;

    return 0;
}

char *maxUserID = (char *)malloc(100);
char *getMaxUserID() {
    sqlite3* db;
    bzero(user_id, 100);
    int exit = sqlite3_open("CHAT.db", &db);

    string sql = "SELECT USER_ID FROM USERS;";
    
    sqlite3_exec(db, sql.c_str(), callbackGetMaxUserID, NULL, NULL);

    return maxUserID;

    return 0;
}

int callbackGetMaxUserID(void *NotUsed, int argc, char **argv, char** azColName) {    
    strcpy (maxUserID, argv[0]);
    
    return 0;
}


int callbackGetHistory(void *NotUsed, int argc, char **argv, char** azColName);
int getHistory(const char *msgID) {
    sqlite3* db;
    int exit = sqlite3_open("CHAT.db", &db);

    string sql = "SELECT SENDER_ID, RECEIVER_ID, TEXT, DATE FROM MESSAGES WHERE MESSAGE_ID = '";
    sql = sql + msgID;
    sql = sql + "' ORDER BY DATE ASC;";
    
    cout << "SQL COMMAND IS : " << sql << '\n';

    sqlite3_exec(db, sql.c_str(), callbackGetHistory, NULL, NULL);

    return 0;
}

int callbackGetHistory(void *NotUsed, int argc, char **argv, char** azColName) {    
    cout << "INSIDE CALLBACk\n";
    senders[msgCounter] = (char *)malloc (100 * sizeof (char));
    receivers[msgCounter] = (char *)malloc (100 * sizeof (char));
    mssg[msgCounter] = (char *)malloc (100 * sizeof (char));
    date[msgCounter] = (char *)malloc (100 * sizeof (char));
    

    getUSERNAME(argv[0]);
    strcpy(senders[msgCounter], user_name);
    
    getUSERNAME(argv[1]);
    strcpy(receivers[msgCounter], user_name);
    
    strcpy(mssg[msgCounter], argv[2]);
    strcpy(date[msgCounter], argv[3]);

    cout << "SENDER : " ;
    printf("%s <-> %s\n", argv[0], senders[msgCounter]);
    cout << "RECEIVER : ";
    printf("%s  <-> %s\n", argv[1], receivers[msgCounter]);
    cout << "MSSG : ";
    printf("%s\n", mssg[msgCounter]);
    
    printf("DATE : %s\n", date[msgCounter]);

    msgCounter++;

    return 0;
} 

void checkHistory(int newSocket) {
    printf("SEVER INSIDE checkHistory\n");

    char answear[4];
    recv(newSocket, answear, 4, 0);
    cout << "THE ANSWEAR IS : " << answear << '\n';

    if (strcmp(answear, "yes") == 0) {
                
        countAvailableUsers = 0;
        getAvailableUsers();
        
        cout << "checkHistory : Trimit " << countAvailableUsers << '\n';
        
        if (send (newSocket, &countAvailableUsers, 4, 0) < 0) { 
            printf("[!]Error in sending the countAvailableUsers.\n");
            exit(1);
        }
        cout << "trimis\n\n"; 
        
        for (int i = 0; i < countAvailableUsers; i++) {
            if (send (newSocket, availableUsers[i], 100, 0) < 0) { 
                printf("[!]Error in sending the AvailableUsers[%d].\n", i);
                exit(1);
            }
            free(availableUsers[i]);
        }
        countAvailableUsers = 0;        

        msgCounter = 0;

        char senderUsername[20];
        if (recv (newSocket, senderUsername, 20 * sizeof (char), 0) < 0) {
            printf ("server <- senderUsername  error.\n");
            exit (1);
        }

        char receiverUsername[20];
        if (recv (newSocket, receiverUsername, 20 * sizeof (char), 0) < 0) {
            printf ("server <- client receiverUsername error.\n");
            exit (1);
        }

        printf("The SENDER IS : %s\n", senderUsername);
    

        printf("The RECEIVER IS : %s\n", senderUsername);
        
        getUSER_ID (senderUsername);
        char SENDER_ID[20];
        strcpy (SENDER_ID, user_id);// GOT USER_ID FOR GIVEN USERNAME

        getUSER_ID (receiverUsername);
        char RECEIVER_ID[20];
        strcpy (RECEIVER_ID, user_id);// GOT USER_ID FOR GIVEN USERNAME

        printf("THE SENDER_ID : %s\n", SENDER_ID);
        printf("THE RECEIVER_ID : %s\n", RECEIVER_ID);
        
        char *ch = (char *) malloc (1);
        strcpy (ch, "_");

        string msg_id;

        if (strcmp (SENDER_ID, RECEIVER_ID) < 0) {
            msg_id = msg_id + SENDER_ID;
            msg_id = msg_id + ch;
            msg_id = msg_id + RECEIVER_ID;
        }
        else {
            msg_id = msg_id + RECEIVER_ID;
            msg_id = msg_id + ch;
            msg_id = msg_id + SENDER_ID;
        }

        cout << "MSG_ID: " << msg_id.c_str() << '\n';

        getHistory(msg_id.c_str());

    //set read status 

        if (send(newSocket, &msgCounter, sizeof(int), 0) < 0) {
            printf("[*]Error in sending the msgCounter.\n");
        }

        for (int i = 0; i < msgCounter; i++) {
            printf ("Trimit mesajul : %s *de la user-ul : %s la user-ul : %s\n", mssg[i], senders[i], receivers[i]);
            if (send (newSocket, senders[i], 100 * sizeof (char), 0) < 0) {
                printf ("eroare la cominucarea autorului mesajului catre client.\n");
                exit(1);
            }
            free(senders[i]);
            
            if (send (newSocket, receivers[i], 100 * sizeof (char), 0) < 0) {
                printf ("eroare la cominucarea autorului mesajului catre client.\n");
                exit(1);
            }
            
            free(receivers[i]);
            
            if (send (newSocket, mssg[i], 100 * sizeof (char), 0) < 0) {
                printf ("eroare la cominucarea mesajului catre client.\n");
                exit(1);
            }
            free(mssg[i]);

            if (send (newSocket, date[i], 100 * sizeof (char), 0) < 0) {
                printf ("eroare la comunicarea message date catre client.\n");
                exit(1);
            }
            free(date[i]);

        }
        msgCounter = 0;
        }
        else {
            printf("DO NOTHING!\n");
        }
}