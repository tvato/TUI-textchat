#include <stdio.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <ncurses.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

WINDOW *pad;
unsigned int nMessages = 0;
int padPos = 0;
char userList[10][10];

struct data {
    char message[256];
    char username[10];
    time_t time;
};

int fileLog(const char *logMessage){
    FILE *f;
    f = fopen("client.log", "a+");
    if(f == NULL){
        return 1;
    }
    time_t rawTime;
    struct tm *timeinfo;
    time(&rawTime);
    timeinfo = localtime(&rawTime);
    fprintf(f, "[%d:%d:%02d]: %s\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, logMessage);
    fclose(f);
    return 0;
}

int fileLogInt(const int logMessage){
    FILE *f;
    f = fopen("client.log", "a+");
    if(f == NULL){
        return 1;
    }
    time_t rawTime;
    struct tm *timeinfo;
    time(&rawTime);
    timeinfo = localtime(&rawTime);
    fprintf(f, "[%d:%d:%02d]: %d\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, logMessage);
    fclose(f);
    return 0;
}

void createTUI(char *address){
    initscr();
    keypad(stdscr, TRUE);

    pad = newpad(1000, COLS);

    mvhline(1, 0, '-', COLS-1);

    move(0,0);
    printw("Use '!commands' to show all the available commands.  |  Connected to %s", address);

    mvhline(LINES-2, 0, '-', COLS-1);

    move(LINES-1, 0);
    printw("> ");
    refresh();
}

struct tm* getTime(time_t rawTime){
    time(&rawTime);
    return localtime(&rawTime);
}

void *receiveData(void *arg){
    struct data *recvData = (struct data*)malloc(sizeof(struct data));
    time_t rawTime;
    struct tm *timeinfo;
    int sockfd = (int)arg;

    while(strcmp(recvData->message, "!exit") != 0){
        int bytes_read = read(sockfd, recvData, sizeof(struct data));
        strcpy(userList[1], recvData->username);
        timeinfo = getTime(recvData->time);
        if(bytes_read > 0){
            wprintw(pad, "[%d:%d:%02d] %s: %s\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, recvData->username, recvData->message);

            if(nMessages < LINES-4){
                nMessages++;
            }else{
                nMessages++;
                padPos++;
            }

            prefresh(pad, padPos, 0, 2, 0, LINES-3, COLS);
            move(LINES-1, 0);
            clrtoeol();
            printw("> ");
            refresh();
        }
    }

    free(recvData);
}

int inputLoop(char *username, int sockfd){
    time_t rawTime;
    struct tm *timeinfo;
    struct data *sendData = (struct data*)malloc(sizeof(struct data));

    pthread_t dataReceiver;
    pthread_create(&dataReceiver, NULL, receiveData, (void*)sockfd);

    strcpy(sendData->username, username);
    strcpy(userList[0], username);
    while(strcmp(sendData->message, "!exit") != 0){
        getstr(sendData->message);
        //fileLog(sendData->message);
        wmove(pad, nMessages, 0);
        if(sendData->message[0] == '!'){
            if(strcmp(sendData->message, "!commands") == 0){
                wprintw(pad, "Commands:\n\t'!exit' disconnects from the server\n\t'!users' list users in the chat\n");
                nMessages = nMessages + 2;
            }else if(strcmp(sendData->message, "!users") == 0){
                wprintw(pad, "\tConnected users:\n");
                for(int i=0; i < sizeof(userList)/sizeof(userList[0]); i++){
                    if(userList[i][0] != NULL){
                        wprintw(pad, "\t- %s\n", userList[i]);
                    }
                }
                nMessages++;
            }
        }else{
            timeinfo = getTime(rawTime);
            wprintw(pad, "[%d:%d:%02d] %s: %s\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, sendData->username, sendData->message);
            sendData->time = rawTime;
            write(sockfd, sendData, sizeof(struct data));
        }

        if(nMessages < LINES-4){
            nMessages++;
        }else{
            nMessages++;
            padPos++;
        }

        /*  prefresh() params:
            1. WINDOW *pad      - pointer to pad to be used
            2. pad_minrow       - row number where pad starts
            3. pad_mincol       - col number where pad starts
            4. screen_minrow    - row number where to place pad on screen
            5. screen_mincol    - col number where to place pad on screen
            6. screen_maxrow    - row number where pad ends on screen
            7. screen_maxcol    - col number where pad ends on screen
        */
        prefresh(pad, padPos, 0, 2, 0, LINES-3, COLS);

        move(LINES-1, 0);
        clrtoeol();
        printw("> ");
        refresh();
    }

    free(sendData);
    return 0;
}

int createClient(char *ip_address, char *username){
    struct sockaddr_in dest_addr;
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){ fileLog("Sockfd failed...\n"); return 1; }

    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(6666);
    dest_addr.sin_addr.s_addr = inet_addr(ip_address);

    int connectReturn = -1;
    while(connectReturn < 0){
        connectReturn = connect(sockfd, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    }

    //if(connect(sockfd, (struct sockaddr*)&dest_addr, sizeof(dest_addr)) < 0){
    //    printf("Connection failed....\n");
    //    return 1;
    //}

    createTUI(ip_address);
    inputLoop(username, sockfd);

    close(sockfd);

    return 0;
}

void *listenConnections(void *arg){
    int sockfd = (int)arg;
    struct sockaddr_in con_addr;
    int clientfd[10] = {0};
    int clientIndex = 0;

    while(1){
        socklen_t addrlen = sizeof(con_addr);
        clientfd[clientIndex] = accept(sockfd, (struct sockaddr*)&con_addr, &addrlen);
        if(clientfd < 0){
            fileLog("Connection accept failed...\n");
            return 1;
        }
        clientIndex++;
        if(clientIndex > 10){
            break;
        }
    }
}

int createServer(char *address, char *username){
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){ fileLog("Server sockfd failed...\n"); return 1; }
    int clientfd = 0;
    int clientIndex = 0;

    struct sockaddr_in server_addr;
    struct sockaddr_in con_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(6666);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr)) < 0){
        fileLog("server bind failed...\n");
        return 1;
    }

    if(listen(sockfd, 10) < 0){
        fileLog("Server listen failed...\n");
        return 0;
    }
    
    socklen_t addrlen = sizeof(con_addr);
    clientfd = accept(sockfd, (struct sockaddr*)&con_addr, &addrlen);
    if(clientfd < 0){
        fileLog("Connection accept failed...\n");
        return 1;
    }

    //pthread_t listeningThread;
    //pthread_create(&listeningThread, NULL, listenConnections, (void*)sockfd);

    createTUI(address);
    inputLoop(username, clientfd);

    close(sockfd);

    return 0;
}

void *createServer2(char *address, char *username){
    int sockfd = 0;
    int clientfd = 0;
    int bytes_read = 0;
    char buffer[256];
    struct sockaddr_in server_addr;
    struct sockaddr_in conn_addr;

    struct data *recvData = (struct data*)malloc(sizeof(struct data));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){
        printf("Sockfd failed... %d\n", sockfd);
        pthread_exit(NULL);
        return NULL;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(6666);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bindfd = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));
    if(bindfd < 0){
        printf("Bindfd failed... %d\n", bindfd);
        pthread_exit(NULL);
        return NULL;
    }

    int listenfd = listen(sockfd, 3);
    if(listenfd < 0){
        printf("Listenfd failed... %d\n", listenfd);
        pthread_exit(NULL);
        return NULL;
    }

    socklen_t addrlen = sizeof(conn_addr);
    clientfd = accept(sockfd, (struct sockaddr*)&conn_addr, &addrlen);
    if(clientfd < 0){
        printf("Clientfd failed... %d\n", clientfd);
        pthread_exit(NULL);
        return NULL;
    }

    time_t rawTime;
    struct tm *timeinfo;

    while(strcmp(recvData->message, "!exit") != 0){
        bytes_read = read(clientfd, recvData, sizeof(struct data));
        //***********************************************************************
        fileLog("Received data message:");
        fileLog(recvData->message);
        fileLog("Received data username:");
        fileLog(recvData->username);
        //***********************************************************************
        wmove(pad, nMessages, 0);

        time(&rawTime);
        timeinfo = localtime(&rawTime);

        if(strcmp(recvData->message, "!exit") == 0){
            wprintw(pad, "[%d:%d:%02d] [%s disconnected]\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, recvData->username);
        }else{
            wprintw(pad, "[%d:%d:%02d] %s: %s\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, recvData->username, recvData->message);
        }

        if(nMessages < LINES-2){
            nMessages++;
        }else{
            nMessages++;
            padPos++;
        }

        prefresh(pad, padPos, 0,0,0, LINES-3, COLS);

        move(LINES-1, 0);
        clrtoeol();
        printw("> ");
        refresh();
    }

    close(sockfd);
    close(clientfd);
    free(recvData);

    return 0;
}

int main(int argc, char **argv){
    char *username = "";
    char *address = "";
    int opt;
    bool isHosting = false;
    pthread_t server_thread;

    while((opt = getopt(argc, argv, "c:u:h")) != -1){
        switch(opt){
            case 'c':
                address = optarg;
                break;
            case 'u':
                username = optarg;
                break;
            case 'h':
                isHosting = true;
                break;
            case '?':
                fprintf(stderr, "No options given");
                break;
            default:
                fprintf(stderr, "Usage: %s [-c <ADDRESS>] [-u <USERNAME>]", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    //pthread_create(&server_thread, NULL, createServer, NULL);
    //pthread_detach(server_thread);
    
    //fileLog("Thread created");
    //fileLog(address);
    if(isHosting){
        createServer(address, username);
    }else{
        createClient(address, username);
    }

    delwin(pad);
    endwin();

    return 0;
}
