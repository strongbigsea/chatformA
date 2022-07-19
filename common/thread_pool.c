/*************************************************************************
	> File Name: thread_pool.c
	> Author: 
	> Mail: 
	> Created Time: Wed 13 Jul 2022 11:17:14 AM CST
 ************************************************************************/

#include "./head.h"
extern char* data[2000];
extern int epollfd;
extern pthread_mutex_t mutex[2000];
void task_queue_init(struct task_queue* taskQueue, int size){
    taskQueue->size= size;
    taskQueue->head = taskQueue->tail = taskQueue->total = 0;
    taskQueue->data = calloc(size, sizeof(void*));//给二级指针分配地址
    pthread_mutex_init(&taskQueue->lock, NULL); 
    pthread_cond_init(&taskQueue->cond, NULL); 
    return;
}
void task_queue_push(struct task_queue* taskQueue, void* data){
    pthread_mutex_lock(&taskQueue->lock);
    if(taskQueue->total == taskQueue->size){
        DBG(YELLOW"<push>: taskQueue is full\n"NONE);
        pthread_mutex_unlock(&taskQueue->lock);//这里释放锁
        return;
    }
    taskQueue->total++;
    taskQueue->data[taskQueue->tail] = data;
    taskQueue->tail++;
    DBG(GREEN"<push>: "RED" data "NONE" is pushed\n");
    if(taskQueue->tail == taskQueue->size){
        DBG(YELLOW"<push>: taskQueue reach tail!\n"NONE);
        taskQueue->tail = 0;
    }
    pthread_cond_broadcast(&taskQueue->cond);//这里用signal 也可
    pthread_mutex_unlock(&taskQueue->lock);
    return ;
}
void* task_queue_pop(struct task_queue* taskQueue){
    pthread_mutex_lock(&taskQueue->lock);
    while(taskQueue->total == 0){//这里是while
        pthread_cond_wait(&taskQueue->cond, &taskQueue->lock);
    }
    void *data =  taskQueue->data[taskQueue->head];
    DBG(GREEN"<pop>: "BLUE" data "NONE" is poped\n");
    taskQueue->total--;
    taskQueue->head++;
    if(taskQueue->head == taskQueue->size){
        DBG(YELLOW"<pop>: taskQueue reach empty!\n"NONE);
        taskQueue->head = 0;
    }
    pthread_mutex_unlock(&taskQueue->lock);
    return data;
}
void* pthread_run(void* arg){
    pthread_detach(pthread_self());
    struct task_queue *taskQueue = (struct task_queue*) arg;
    while(1){
        void *data = task_queue_pop(taskQueue);
        printf("%s", (char *)data);
    }
    return (void*) 0;
}
void do_work(int fd){
    char buff[4096] = {};
    int ind = strlen(data[fd]);
    int n;
    DBG(BLUE"<R>: data is read on\n"NONE);
    if((n = read(fd, buff, 4096)) < 0){
        if(errno != EAGAIN){//如果错误号不是EAGAIN,才退出
            epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);//出错将文件描述符删除
            DBG(RED"<C>: %d is closed! \n"NONE, fd);
            close(fd);
        }
        return ;
    }
    pthread_mutex_lock(&mutex[fd]);    
    for(int i = 0; i < n; i++){
        if(buff[i] >= 'A' && buff[i] <= 'Z')
            data[fd][ind++] = tolower(buff[i]);
        else if(buff[i] >= 'a' && buff[i] <= 'z')
            data[fd][ind++] = toupper(buff[i]);
        else{
            data[fd][ind++] = buff[i];
            if(buff[i] == '\n'){//收到\n后发送数据
                DBG(GREEN"<END>: \\n is received! \n"NONE);
                write(fd, data[fd],ind); 
                bzero(data[fd], sizeof(data[fd]));
            }
        } 
    } 
    pthread_mutex_unlock(&mutex[fd]);    
    return ;
}
void* worker(void * arg){
    pthread_detach(pthread_self());
    struct task_queue *taskQueue = (struct task_queue*) arg;
    while(1){
        int *fd = (int*)task_queue_pop(taskQueue);
        do_work(*fd);
    }
    return (void*)0;
}
