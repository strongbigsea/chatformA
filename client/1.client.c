/*************************************************************************
	> File Name: 1.client.c
	> Author: 
	> Mail: 
	> Created Time: Fri 15 Jul 2022 08:00:53 PM CST
 ************************************************************************/
#include "../common/head.h"

const char* confg = "./wechat.conf";
char name[50] = { 0 };
int sockfd;
WINDOW *msg_win, *sub_msg_win, *info_win, *sub_info_win, *input_win, *sub_input_win;
void logout(int signum)
{
    struct wechat_msg msg;
    msg.type = WECHAT_FIN;
    strcpy(msg.from, name);
    send(sockfd, &msg, sizeof(msg), 0);
    printf(YELLOW "bye!\n" NONE);
    exit(0);
}
int main(int argc, char** argv)
{
    int op, server_port = 0, sex = -1, mode = 0;
    char server_ip[20] = { 0 };
    while ((op = getopt(argc, argv, "p:h:s:n:m:")) != -1) { //可选参数传参方式
        switch (op) {
        case 'p':
            server_port = atoi(optarg);
            break;
        case 'h':
            strcpy(server_ip, optarg);
            break;
        case 's':
            sex = atoi(optarg);
            break;
        case 'n':
            strcpy(name, optarg);
            break;
        case 'm':
            mode = atoi(optarg);
            break;
        default:
            printf("Usage %s -p port -h host -s sex -n name -m mode\n", argv[0]);
            exit(1);
        }
    }
    if (access(confg, R_OK)) {
        fprintf(stderr, RED "open config file erro\n" NONE);
        exit(1);
    }
#ifdef UI
    setlocale(LC_ALL, "");
    init_ui();
#endif
    if (server_port == 0)
        server_port = atoi(get_conf_value(confg, "PORT"));
    if (sex == -1)
        sex = atoi(get_conf_value(confg, "SEX"));
    if (strlen(name) == 0)
        strcpy(name, get_conf_value(confg, "NAME"));
    if (strlen(server_ip) == 0)
        strcpy(server_ip, get_conf_value(confg, "IP"));
    DBG(GREEN "port:[%d] sex:[%d] name:[%s] sever_ip[%s]\n" NONE, server_port, sex, name, server_ip);
    if ((sockfd = socket_connect(server_ip, server_port)) < 0) {
        perror("socket connect");
        exit(1);
    }
    DBG(GREEN "client connect successfully on[%d]\n" NONE, sockfd);
    struct wechat_msg msg;
    bzero(&msg, sizeof(msg));
    strcpy(msg.from, name);
    //strcpy(msg.msg, "hello\n");
    msg.sex = sex;
    if (mode == 0) {
        msg.type = WECHAT_SIGNUP;
    } else {
        msg.type = WECHAT_SIGNIN;
    }
    write(sockfd, &msg, sizeof(msg)); //发送消息
    fd_set rfds; //使用select 避免长期等待
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
    struct timeval time;
    time.tv_sec = 2;
    time.tv_usec = 0;
    if (select(sockfd + 1, &rfds, NULL, NULL, &time) < 0) { //使用select超时则退出
        fprintf(stderr, RED "system err\n" NONE);
        exit(1);
    }
    bzero(&msg, sizeof(msg));
    int ret = read(sockfd, (void*)&msg, sizeof(msg)); //接收消息
    if (ret < 0) {
        fprintf(stderr, RED "server close connection\n" NONE);
        exit(1);
    }
    if (msg.type & WECHAT_ACK) {
        DBG(GREEN "receive successfully\n" NONE);
        if (!mode) { //未注册
            printf(GREEN "please login after this\n" NONE);
            exit(0);
        }
    } else { //服务端拒绝响应
        DBG(RED "receive failure\n" NONE);
        close(sockfd);
        exit(1);
    }
    DBG(BLUE "login successfully\n" NONE);
    pthread_t tid;
    pthread_create(&tid, NULL, client_work, (void*)&sockfd); //子线程负责收 收到进行打印

    //客户端退出
    signal(SIGINT, logout);
    while (1) { //主线程循环发送
        /*printf("Please Input:\n");
        char buff[1024] = {0};
        scanf("%[^\n]", buff);
        getchar();
        if(!strlen(buff)) continue;
        msg.type = WECHAT_WALL;
        strcpy(msg.msg, buff);
        send(sockfd, (void*)&msg, sizeof(msg), 0);*/
        bzero(&msg.msg, sizeof(msg.msg));
        echo(); //显示文字
        nocbreak(); //按回车显示
        mvwscanw(input_win, 2, 1, "%[^\n]", msg.msg); //窗口输入文字
        msg.type = WECHAT_WALL;
        if (!strlen(msg.msg))
            continue;
        if (msg.msg[0] == '@') {
            if(!strstr(msg.msg, " ")){
                strcpy(msg.msg, "输入格式有误\n");
                show_msg(&msg);
                continue;
            }
            msg.type = WECHAT_MSG;
            strncpy(msg.to, msg.msg + 1, strchr(msg.msg, ' ') -msg.msg -1);//strchar找到第一个字符出现的位置 
        }
        send(sockfd, (void*)&msg, sizeof(msg), 0);
        wclear(input_win);
        box(input_win, 0, 0); //显示方框
        noecho();
        cbreak();
    }
    return 0;
}

