/*************************************************************************
	> File Name: common.c
	> Author: 
	> Mail: 
	> Created Time: Thu 14 Jul 2022 01:47:59 PM CST
 ************************************************************************/
#include "./head.h"
int socket_create(int port){
    int listenfd;
    if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        perror("sockfd");
        exit(1);
    }
    struct sockaddr_in server;
    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    int reuse = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (char *)&reuse, sizeof(int));//设置地址可重用
    if(bind(listenfd, (struct sockaddr*)&server, sizeof(server)) < 0){
        perror("bind");
        exit(1);
    }
    if(listen(listenfd, 1000) < 0){
        perror("listen");
        exit(1);
    }
    return listenfd;
}
int socket_connect(const char* ip, int port){
    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
        return -1;
    }
    struct sockaddr_in serveraddr;
    bzero(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &serveraddr.sin_addr.s_addr); 
    serveraddr.sin_port = htons(port);
    if(connect(sockfd, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) < 0){
        return -1;
    }
    return sockfd;
}
void make_nonblock(int fd){
    int flags;
    if((flags = fcntl(fd, F_GETFL)) < 0){
        perror("fcntl get flags");
        exit(1);
    } 
    flags |= O_NONBLOCK;
    if((flags = fcntl(fd, F_SETFL, flags)) < 0){
        perror("fcntl set flags");
        exit(1);
    } 
    return;
}
void make_block(int fd){
    int flags;
    if((flags = fcntl(fd, F_GETFL)) < 0){
        perror("fcntl get flags");
        exit(1);
    } 
    flags &= ~O_NONBLOCK;//进行还原
    if((flags = fcntl(fd, F_SETFL, flags)) < 0){
        perror("fcntl set flags");
        exit(1);
    } 
    return;
}
const char* get_conf_value(const char* file, const char *key){
    FILE* fp;
    if((fp = fopen(file, "r")) == NULL){
        perror("fopen confg_file");
        exit(1);
    }
    char *line = NULL, *sub = NULL;
    ssize_t len = 0, nread = 0;
    bzero(port, sizeof(port));
    while((nread = getline(&line, &len, fp)) != -1){
        if((sub = strstr(line, key)) == NULL) continue;
        if(sub == line && line[strlen(key) == '=']){
            strcpy(port, line + strlen(key) + 1);
            if(port[strlen(port) - 1] == '\n'){
                port[strlen(port) - 1] = '\0';//字符串后面加上\0是一个好的习惯
            }
        }
    }
    fclose(fp);
    free(line);//释放line
    return port;
}
