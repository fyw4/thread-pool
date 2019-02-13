/*****************************************************************************/
//  �ļ���   ��thread_poor.h
//  ��Ҫ     ��
//  ����     �� wangqingchuan
//  �汾���� �� ����  2019-02-13
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

#define BUSY_THRESHOLD 0.5  //(�����߳�)/(�̳߳������߳�)
#define MANAGE_INTERVAL (60*10)   //�����߳�ʱ����

#define VOS_OK 0
#define VOS_ERR 1

#define VOS_YES   1
#define VOS_NO    0
#define VOS_OVER  -1
#define VOS_MESSAGE_NULL 2

typedef struct tp_thread_info_s tp_thread_info;	//���������̵߳�id,�Ƿ���У�ִ�е��������Ϣ
typedef struct tp_thread_pool_s tp_thread_pool;  	//�й��̳߳ز����Ľӿ���Ϣ


//�߳���Ϣ
struct tp_thread_info_s{
    int          is_busy;		//�߳��Ƿ�æµ
    int          is_wait;		//�߳��Ƿ���Ա�����
    int          is_signal;		//�߳��Ƿ��յ���������
    int          is_destroy;	//�߳��Ƿ�����
    void (*func)(void *);
    void *input;
	void *output;
    pthread_cond_t   thread_cond;
    pthread_mutex_t  thread_lock;
    pthread_t        thread_id;  //thread id num
};

//main thread pool struct
struct tp_thread_pool_s{
    int (*init)(tp_thread_pool *this);  	//ָ��init������ָ�룬��ͬ
    void (*close)(tp_thread_pool *this);
    int (*process_job)(tp_thread_pool *this, void(*func)(void* input), void * input ,void *output);
    int (*get_thread_by_id)(tp_thread_pool *this, pthread_t id);
    int (*add_thread)(tp_thread_pool *this);
    int (*delete_thread)(tp_thread_pool *this);
    int (*get_tp_status)(tp_thread_pool *this);

    int min_th_num;     //�̳߳����߳���С����
    int cur_th_num;     //��ǰ�̳߳�����
    int max_th_num;     //�̳߳����߳��������
    pthread_mutex_t tp_lock;
    pthread_t manage_thread_id; 	//�����̵߳�ID
    tp_thread_info *thread_info;    //�����̵߳������Ϣ
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
