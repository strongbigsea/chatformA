/*************************************************************************
	> File Name: common.h
	> Author: 
	> Mail: 
	> Created Time: Thu 14 Jul 2022 01:47:42 PM CST
 ************************************************************************/

#ifndef _COMMON_H
#define _COMMON_H
int socket_create(int port);
int socket_connect(const char* ip, int port);
void make_nonblock(int fd);
void make_block(int fd);
const char *get_conf_value(const char* file, const char* key);
char port[512];
#endif
