/*************************************************************************
	> File Name: wechat_ui.h
	> Author: suyelu
	> Mail: suyelu@126.com
	> Created Time: Fri 24 Dec 2021 07:47:41 PM CST
 ************************************************************************/

#ifndef _WECHAT_UI_H
#define _WECHAT_UI_H
#define MSG_WIDTH 80
#define MSG_HEIGHT 20
#define INFO_WIDTH 10
void init_ui();
void gotoxy_puts(int x, int y, char *s);
void gotoxy_putc(int x, int y, char c);
void gotoxy(int x, int y);
void w_gotoxy_puts(WINDOW *win, int x, int y, char *s);
void w_gotoxy_putc(WINDOW *win, int x, int y, char c);
void destroy_win(WINDOW *win);
WINDOW *create_newwin(int width, int height, int x, int y);
void show_msg(struct wechat_msg *msg);
#endif
