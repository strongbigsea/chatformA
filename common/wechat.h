/*************************************************************************
	> File Name: wechat.h
	> Author: 
	> Mail: 
	> Created Time: Fri 15 Jul 2022 06:12:39 PM CST
 ************************************************************************/

#ifndef _WECHAT_H
#define _WECHAT_H

#define MAXEVENTS 5
#define MAXCLIENTS 1000

struct wechat_user{//定义用户结构体
    char name[50];
    char passwd[200];
    int sex;
    int fd;
    int isOnline;
};
#define WECHAT_SIGNUP 0x01
#define WECHAT_SIGNIN 0x02
#define WECHAT_SIGNOUT 0x04
#define WECHAT_ACK 0x08
#define WECHAT_NAK 0x10
#define WECHAT_SYS 0x20
#define WECHAT_WALL 0x40
#define WECHAT_MSG 0x80
#define WECHAT_FIN 0x100
#define WECHAT_HEART 0x200
#define HEART_SLEEP_TIME 10
struct wechat_msg{//定义消息结构体
    int type;
    int sex;
    char from[50];
    char to[50];
    char msg[512];
};
void* sub_reactor(void* arg);
void* client_work(void* arg);
void* heart_beat(void* arg);
int add_to_reactor(int epfd, int fd);
struct wechat_user* users;
void send_all(struct wechat_msg*);
void send_all_notme(struct wechat_msg*);
void send_to(struct wechat_msg*);
#endif
