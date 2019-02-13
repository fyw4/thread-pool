/*****************************************************************************/
//  文件名   ：thread_poor.h
//  概要     ：
//  作者     ： wangqingchuan
//  版本控制 ； 初版  2019-02-13
/*****************************************************************************/


#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>


#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#define BUSY_THRESHOLD 0.5  //(工作线程)/(线程池所有线程)
#define MANAGE_INTERVAL (60*10)   //管理线程时间间隔

#define VOS_OK 0
#define VOS_ERR 1

#define VOS_YES   1
#define VOS_NO    0
#define VOS_OVER  -1
#define VOS_MESSAGE_NULL 2

typedef struct tp_thread_info_s tp_thread_info;	//描述各个线程的id,是否空闲，执行的任务等信息
typedef struct tp_thread_pool_s tp_thread_pool;  	//有关线程池操作的接口信息


//线程信息
struct tp_thread_info_s{
    int          is_busy;		//线程是否忙碌
    int          is_wait;		//线程是否可以被调用
    int          is_signal;		//线程是否收到条件变量
    int          is_destroy;	//线程是否被销毁
    void (*func)(void *);
    void *input;
	void *output;
    pthread_cond_t   thread_cond;
    pthread_mutex_t  thread_lock;
    pthread_t        thread_id;  //thread id num
};

//main thread pool struct
struct tp_thread_pool_s{
    int (*init)(tp_thread_pool *this);  	//指向init函数的指针，下同
    void (*close)(tp_thread_pool *this);
    int (*process_job)(tp_thread_pool *this, void(*func)(void* input), void * input ,void *output);
    int (*get_thread_by_id)(tp_thread_pool *this, pthread_t id);
    int (*add_thread)(tp_thread_pool *this);
    int (*delete_thread)(tp_thread_pool *this);
    int (*get_tp_status)(tp_thread_pool *this);

    int min_th_num;     //线程池中线程最小数量
    int cur_th_num;     //当前线程池数量
    int max_th_num;     //线程池中线程最大数量
    pthread_mutex_t tp_lock;
    pthread_t manage_thread_id; 	//管理线程的ID
    tp_thread_info *thread_info;    //工作线程的相关信息
};
tp_thread_pool * pool;

void get_thread_pool(tp_thread_pool **pPooltemp);

tp_thread_pool *creat_thread_pool(int min_num, int max_num);

int thread_pool_init(int min_num,int max_num);

void *tp_work_thread(void *pthread);

void *tp_manage_thread(void *pthread);

int tp_init(tp_thread_pool *this);

void tp_close(tp_thread_pool *this);

int tp_process_job(tp_thread_pool *this, void(*pFunc)(void*), void *pInput ,void *pOutput);

int  tp_get_thread_by_id(tp_thread_pool *this, pthread_t id);

int tp_add_thread(tp_thread_pool *this);

int tp_delete_thread(tp_thread_pool *this);

int  tp_get_tp_status(tp_thread_pool *this);

void my_process_job(tp_thread_pool *this, void (*fun)(void *), void * a);
#endif
