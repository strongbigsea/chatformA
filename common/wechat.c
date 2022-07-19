/*************************************************************************
	> File Name: wechat.c
	> Author: 
	> Mail: 
	> Created Time: Fri 15 Jul 2022 06:12:32 PM CST
 ************************************************************************/
#include "./head.h"
extern struct wechat_user* users;
int epollfd, subefd1, subefd2;
void* sub_reactor(void* arg)
{ //反应堆里面做的事和主反应堆类似
    int subfd = *(int*)arg;
    DBG(GREEN "sub reactor [%d]\n" NONE, subfd);
    struct epoll_event events[MAXEVENTS], ev;
    while (1) {
        //DBG(YELLOW "sub reactor[%d] start wait\n" NONE, subfd);
        int nfds = epoll_wait(subfd, events, MAXEVENTS, -1);
        if (nfds < 0) {
            DBG(RED "sub reactor erro [%d]\n" NONE, subfd);
            continue;
        }
        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            struct wechat_msg msg;
            bzero(&msg, sizeof(msg));
            int ret = recv(fd, &msg, sizeof(msg), 0); //当字节数为0 的时候返回0 出错返回- 1
            if (ret < 0 && !(errno & EAGAIN)) {
                close(fd);
                epoll_ctl(subfd, EPOLL_CTL_DEL, fd, NULL);
                users[fd].isOnline = 0;
                DBG(RED "sub reactor: connection of %d on %d is closed\n" NONE, fd, subfd);
                continue;
            }
            if (ret != sizeof(msg)) { //收到消息大小不一致
                DBG(RED "<sub reactor>" NONE ": msg size err<%ld, %d>\n", sizeof(msg), ret);
                continue;
            }
            users[fd].isOnline = 5;//每收到消就重置
            if (msg.type & WECHAT_WALL) {
                DBG(GREEN "<wall msg>" NONE "from: %s,msg is %s\n", msg.from, msg.msg);
                send_all(&msg);
                show_msg(&msg);
            } else if(msg.type & WECHAT_FIN){//收到下线消息
                //bzero(&msg, sizeof(msg));
                msg.type = WECHAT_SYS;
                sprintf(msg.msg, "您的好友 %s 下线了\n", msg.from);
                DBG(YELLOW"%s logouted\n"NONE, msg.from);
                send_all_notme(&msg); 
                show_msg(&msg);
                close(fd);
                epoll_ctl(subfd, EPOLL_CTL_DEL, fd, NULL);
                users[fd].isOnline = 0;
            } else if(msg.type &  WECHAT_MSG){
                send_to(&msg);
            } else {
                DBG(RED "<pink msg>" NONE " from: %s,msg is %s\n", msg.from, msg.msg);
            }
        }
    }
}
void send_all(struct wechat_msg* msg)
{ //给所有人发消息，包括自己
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (users[i].isOnline) {
            send(users[i].fd, msg, sizeof(struct wechat_msg), 0);
        }
    }
    return;
}
void send_to(struct wechat_msg* msg)
{ //私聊 
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (users[i].isOnline && !strcmp(msg->to, users[i].name)) {
            send(users[i].fd, msg, sizeof(struct wechat_msg), 0);
        }
    }
    return;
}
void send_all_notme(struct wechat_msg* msg)
{
    for (int i = 0; i < MAXCLIENTS; i++) {
        if (users[i].isOnline && strcmp(msg->from, users[i].name)) {
            send(users[i].fd, msg, sizeof(struct wechat_msg), 0);
        }
    }
    return;
}
void* client_work(void* arg)
{
    int sockfd = *(int*)arg;
    struct wechat_msg msg;
    int ret;
    while (1) {
        bzero(&msg, sizeof(msg));
        if ((ret = recv(sockfd, &msg, sizeof(msg), 0))<= 0) { //服务端关闭以后客户端会接收到0 perror 为seccess
            DBG(RED " server close[%d]\n" NONE, sockfd);
            perror("recv");
            exit(1);//客户端直接退出
        }
        if (msg.type & WECHAT_HEART) { //如果客户端收到心跳包
            //struct wechat_msg ack;
            //bzero(&ack, sizeof(ack));
            //ack.type = WECHAT_HEART | WECHAT_ACK;
            //send(sockfd, &ack, sizeof(ack), 0);
            strcpy(msg.msg, "❤");
            show_msg(&msg);
        } else {
            show_msg(&msg);
        }
        //printf("receive from: %s : %s\n", msg.from, msg.msg);
    }
}
void* heart_beat(void* arg)
{
    struct wechat_msg msg;
    msg.type = WECHAT_HEART;
    while (1) {
        sleep(HEART_SLEEP_TIME);
        for (int i = 0; i < MAXCLIENTS; i++) {
            if (users[i].isOnline) {
                users[i].isOnline--;
                send(users[i].fd, &msg, sizeof(msg), 0);
                DBG(GREEN"users:%d cnt = %d\n"NONE, i, users[i].isOnline);
                if (users[i].isOnline == 0) {
                    int which = users[i].sex ? subefd1 : subefd2;
                    close(users[i].fd);
                    epoll_ctl(which, EPOLL_CTL_DEL, users[i].fd, NULL);
                }
            }
        }
    }
    return (void*)0;
}
int add_to_reactor(int epfd, int fd)
{ //这个函数负责事件分发
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN | EPOLLET;
    //ev.events = EPOLLIN;
    make_nonblock(fd);
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev) < 0) {
        return -1;
    }
    return 0;
}
