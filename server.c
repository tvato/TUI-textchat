#include <stdlib.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

struct data {
    char message[256];
    char username[10];
};

int createServer(){
    int sockfd = 0;
    int clientfd = 0;
    int bytes_read = 0;
    char buffer[256];
    unsigned int struct_size = 0;
    struct sockaddr_in server_addr;
    struct sockaddr_in con_addr;

    struct data *recvData = (struct data*)malloc(sizeof(struct data));

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0){ printf("Sockfd failed... %d\n", sockfd); return 1; }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(6666);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int bindfd = bind(sockfd, (struct sockaddr*)&server_addr, sizeof(struct sockaddr));
    if(bindfd < 0){ printf("Bindfd failed... %d\n", bindfd); return 1; }

    int listenfd = listen(sockfd, 10);
    if(listenfd < 0){ printf("Listenfd failed... %d\n", listenfd); return 1; }

    socklen_t addrlen = sizeof(con_addr);
    clientfd = accept(sockfd, (struct sockaddr*)&server_addr, &addrlen);
    if(clientfd < 0){ printf("Clientfd failed... %d\n", clientfd); return 1; }

    while(strcmp(recvData->message, "!exit") != 0){
        bytes_read = read(clientfd, recvData, sizeof(struct data));
        printf("%d - %s: %s\n", sockfd, recvData->username, recvData->message);
    }

    close(sockfd);
    close(clientfd);
    free(recvData);

    return 0;
}

int main(){
    printf("Starting server...\n");
    createServer();
    printf("Server closed....\n");

    return 0;
}
