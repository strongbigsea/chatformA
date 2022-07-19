/*************************************************************************
	> File Name: 1.server.c
	> Author: 
	> Mail: 
	> Created Time: Fri 15 Jul 2022 04:03:30 PM CST
 ************************************************************************/
#include "../common/head.h"

#define INS 2
WINDOW *msg_win, *sub_msg_win, *info_win, *sub_info_win, *input_win, *sub_input_win;
const char* config = "./wechat.conf";
extern struct wechat_user* users; //指针指向users
extern int epollfd,subefd1, subefd2;
int main(int argc, char** argv)
{
    int opt, port = 0, sever_listen, sockfd; //一主两从
    users = (struct wechat_user*)calloc(MAXCLIENTS, sizeof(struct wechat_user));
    while ((opt = getopt(argc, argv, "p:")) != -1) {
        switch (opt) {
        case 'p':
            port = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s -p port\n", argv[0]);
            exit(1);
        }
    }
    if (access(config, R_OK)) {
        DBG(RED "config" NONE "file is not ready\n");
        exit(1);
    }
    if (port == 0) { //读配置文件
        port = atoi(get_conf_value(config, "PORT"));
        DBG(GREEN "<D> port = %d!\n" NONE, port);
    }
    #ifdef UI
    setlocale(LC_ALL, ""); //显示中文
    init_ui(); //初始化界面
    #endif
    if ((sever_listen = socket_create(port)) < 0) {
        perror("socket create\n");
        exit(1);
    }
    //心跳 定时器会干扰其他。因此守护线程
    /*struct itimerval itv;
    itv.it_value.tv_sec = 2;
    itv.it_value.tv_usec = 0;
    itv.it_interval.tv_sec = 2;
    itv.it_interval.tv_usec = 0;
    setitimer(ITIMER_REAL, &itv, NULL);

    signal(SIGALRM, heart_beat);*/

    DBG(GREEN "sever_listen[%d] is created\n" NONE, sever_listen);
    if ((epollfd = epoll_create(1)) < 0) {
        perror("epoll create  main_epoll");
        exit(1);
    }
    if ((subefd1 = epoll_create(1)) < 0) {
        perror("epoll create subefd1");
        exit(1);
    }
    if ((subefd2 = epoll_create(1)) < 0) {
        perror("epoll create  subefd2");
        exit(1);
    }
    DBG(GREEN "epoll create mainpoll[%d] subefd1[%d] subefd2[%d]\n" NONE, epollfd, subefd1, subefd2);

    pthread_t tid1, tid2, heart_tid;
    pthread_create(&tid1, NULL, sub_reactor, (void*)&subefd1); //创建从反应堆
    pthread_create(&tid2, NULL, sub_reactor, (void*)&subefd2);
    pthread_create(&heart_tid, NULL, heart_beat, NULL);

    DBG(GREEN "thread is created tid1[%ld] tid2[%ld]\n", tid1, tid2);
    struct epoll_event events[MAXEVENTS], ev;
    ev.data.fd = sever_listen;
    ev.events = EPOLLIN;
    epoll_ctl(epollfd, EPOLL_CTL_ADD, sever_listen, &ev);
    
    //处理信号集，防止打断epoll
    

    while (1) {
        int nfds = epoll_wait(epollfd, events, MAXEVENTS, -1);
        if (nfds < 0) {
            perror("epoll_wait");
            exit(1);
        }
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            if (fd == sever_listen && (events[i].events & EPOLLIN)) {
                sockfd = accept(sever_listen, NULL, NULL); //这两个可以为空
                if (sockfd < 0) {
                    perror("accept");
                    exit(1);
                }
                ev.data.fd = sockfd;
                ev.events = EPOLLIN;
                epoll_ctl(epollfd, EPOLL_CTL_ADD, sockfd, &ev);
            } else if (events[i].events & EPOLLIN) {
                //收数据 验证
                struct wechat_msg msg;
                bzero(&msg, sizeof(msg));
                int ret = recv(fd, (void*)&msg, sizeof(msg), 0);
                if (ret <= 0) { //这里不应该exit接收文件出错应该continue
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
                    close(fd);
                    continue;
                }
                if (ret != sizeof(msg)) { //读到的数据有误
                    DBG(RED "read erro : mag size err: ret=%d, sizeof(msg)=%ld\n" NONE, ret, sizeof(msg));
                    continue;
                }
                if (msg.type & WECHAT_SIGNUP) {
                    //注册用户,更新用户信息到文件中，判断是否可以注册
                    msg.type = WECHAT_ACK;
                    write(fd, (void*)&msg, sizeof(msg));
                } else if (msg.type & WECHAT_SIGNIN) {
                    //登录信息.判断是否可以登录
                    //加到反应堆中去
                    users[fd].isOnline = 5;
                    msg.type = WECHAT_ACK;
                    write(fd, (void*)&msg, sizeof(msg));
                    sprintf(msg.msg, "你的好友%s上线了，快和他打招呼吧\n", msg.from);
                    show_msg(&msg); //发送数据到界面

                    strcpy(users[fd].name, msg.from);
                    users[fd].fd = fd;
                    users[fd].sex = msg.sex;
                    int which = users[fd].sex ? subefd1 : subefd2;
                    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL); //分发后从主反应堆中剔除
                    add_to_reactor(which, fd);
                }else {
                    //报文信息有误
                }
            }
        }
    }
    return 0;
}

