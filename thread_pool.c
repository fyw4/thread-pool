/*****************************************************************************/
//  文件名   ：thread_poor.c
//  概要     ：
//  作者     ： wangqingchuan
//  版本控制 ； 初版  2019-02-13
/*****************************************************************************/
#include <stdlib.h>
#include <string.h>


#include "thread_pool.h"

//获得线程池
void get_thread_pool(tp_thread_pool **pPooltemp)
{
    *pPooltemp = pool;
}

//线程池创造以及初始化
int thread_pool_init(int min_num,int max_num)
{
    pool = creat_thread_pool(min_num,max_num);		//创造线程池
    if(tp_init(pool)==FALSE)						//初始化线程池
    {
        printf("Thread Pool : tp_init is wrong!\n");
        return TRUE;
    }
    return FALSE;
}

//创造线程池
tp_thread_pool *creat_thread_pool(int min_num, int max_num)
{
    tp_thread_pool *this;											//有关线程池操作的接口信息
    this = (tp_thread_pool*)malloc(sizeof(tp_thread_pool));

    memset(this, 0, sizeof(tp_thread_pool));

	//初始化指向各功能函数
    this->init = tp_init;
    this->close = tp_close;
    this->process_job = tp_process_job;
    this->get_thread_by_id = tp_get_thread_by_id;
    this->add_thread = tp_add_thread;
    this->delete_thread = tp_delete_thread;
    this->get_tp_status = tp_get_tp_status;


    this->min_th_num = min_num;									//最小线程数量
    this->cur_th_num = this->min_th_num;						//当前线程数量
    this->max_th_num = max_num;									//最大线程数量

    pthread_mutex_init(&this->tp_lock, NULL);

    if(NULL != this->thread_info)
      free(this->thread_info);

    this->thread_info = (tp_thread_info*)malloc(sizeof(tp_thread_info)*this->max_th_num); //申请max_th_num个相关线程信息内存空间

    return this;
}

//线程池初始化
int tp_init(tp_thread_pool *this)
{
    int i;
    int err;

    for(i=0;i<this->min_th_num;i++) //创建min_th_num个线程
    {
    	//线程池各个线程的信息初始化
        pthread_cond_init(&this->thread_info[i].thread_cond, NULL);		//为每个线程的条件变量初始化
        pthread_mutex_init(&this->thread_info[i].thread_lock, NULL);	//为每个线程上锁

        this->thread_info[i].is_signal = FALSE;
        this->thread_info[i].is_wait = FALSE;
        this->thread_info[i].is_busy = FALSE;
        this->thread_info[i].is_destroy = FALSE;

        err = pthread_create(&this->thread_info[i].thread_id, NULL, tp_work_thread, this);	//创建工作线程
        if(0 != err)
        {
            printf("Thread Pool : creat work thread failed when init\n");
            return FALSE;
        }
    }

    err = pthread_create(&this->manage_thread_id, NULL, tp_manage_thread, this);			//创建管理线程
    if(0 != err)
    {
        printf("Thread Pool : creat manage thread failed\n");
        return FALSE;
    }

    return TRUE;
}

//销毁线程池
void tp_close(tp_thread_pool *this)
{
    int i;

    for(i=0;i<this->cur_th_num;i++)
    {
    	//销毁线程池中当前所有线程
        //pthread_kill(this->thread_info[i].thread_id, SIGKILL);
        pthread_cancel(this->thread_info[i].thread_id);
        pthread_mutex_destroy(&this->thread_info[i].thread_lock);
        pthread_cond_destroy(&this->thread_info[i].thread_cond);
    }

	//销毁管理线程
    //pthread_kill(this->manage_thread_id, SIGKILL);
    pthread_cancel(this->manage_thread_id);
    pthread_mutex_destroy(&this->tp_lock);

    printf("tp_close: kill manage thread %lu\n", this->manage_thread_id);

    free(this->thread_info);
}

//对外接口
int tp_process_job(tp_thread_pool *this, void(*pFunc)(void*), void *pInput ,void *pOutput)
{
    int i;
    int waitLoop = 0;

    printf("Thread Pool : tp_process_job(%d/%d/%d).\n", this->min_th_num, this->cur_th_num, this->max_th_num);

    for (i = 0; i < this->cur_th_num; i++)
	{

        printf("Thread Pool : tp_process_job for(%d/%d).\n", i+1, this->cur_th_num);

        pthread_mutex_lock(&this->thread_info[i].thread_lock); 	//从队列中0开始找，上锁

        printf("Thread Pool : tp_process_job get lock(%d/%d).\n", i+1, this->cur_th_num);

        if(!this->thread_info[i].is_busy)  						//寻找在线程池中空闲线程
		{
            printf("Thread Pool : tp_process_job get free(%d/%d).\n", i+1, this->cur_th_num);

		    this->thread_info[i].is_busy = TRUE;				//找到空闲线程后，设置该线程为忙碌

		    if(this->thread_info[i].is_wait != TRUE)          	//判断该线程是否能够被调用
            {
                printf("Thread Pool : tp_process_job wait fail. id : %d, all num : %d, wait : %d, busy : %d, signal : %d.\n",i+1, this->cur_th_num,  this->thread_info[i].is_wait, this->thread_info[i].is_busy, this->thread_info[i].is_signal);

                this->thread_info[i].is_busy = FALSE;			//若该线程不能别调用将该标志位恢复原来的状态
                pthread_mutex_unlock(&this->thread_info[i].thread_lock);
                return 1; 										//失败返回
            }
            else
            {
                this->thread_info[i].is_signal = TRUE;          //设置该线程收到cond_signal信号
                this->thread_info[i].func = pFunc;				//函数指针指向消息分类处理函数
                this->thread_info[i].input = pInput;			//指向消息分类函数输入参数
                this->thread_info[i].output = pOutput;			//指向消息分类函数输出参数

                pthread_cond_signal(&this->thread_info[i].thread_cond);  //唤醒该线程（tp_work_thread）

                this->thread_info[i].is_busy = FALSE;

                printf("Thread Pool : tp_process_job send signal. id : %d, all num : %d, wait : %d, busy : %d, signal : %d.\n",i+1, this->cur_th_num, this->thread_info[i].is_wait, this->thread_info[i].is_busy, this->thread_info[i].is_signal);

                pthread_mutex_unlock(&this->thread_info[i].thread_lock);
                return 0;
            }
        }
        else
        {
            pthread_mutex_unlock(&this->thread_info[i].thread_lock);
    	}
    }


	//若线程池中当前所有线程都忙碌（thread_info[i].is_busy = TRUE），则在线程池中新加一个线程并使用
    pthread_mutex_lock(&this->tp_lock); //对线程池上锁

    if(this->add_thread(this))			//若添加线程成功
	{
        i = this->cur_th_num - 1;		//cur_th_num在add_thread已经加1，为了显示正确的线程队列位置i，减1

        printf("Thread Pool : add new one. id : %d, all num : %d.\n", i+1, this->cur_th_num);

        pthread_mutex_unlock(&this->tp_lock);

        waitLoop = 0;
        while (this->thread_info[i].is_wait != TRUE)		//判断该线程是否能够被调用
        {
            usleep(1000);									//等待
            waitLoop++;
            if (waitLoop >= 10)
            {
                break;
            }
        }

        pthread_mutex_lock(&this->thread_info[i].thread_lock);

        if (waitLoop >= 10)									//若规定的时间结束仍不能被调用
        {
            this->thread_info[i].is_busy = FALSE;			//将线程设置为空闲

            printf("Thread Pool : tp_process_job add waitloop fail. id : %d, all num : %d, waitLoop : %d, wait : %d, busy : %d, signal : %d.\n",i+1, this->cur_th_num, waitLoop, this->thread_info[i].is_wait, this->thread_info[i].is_busy, this->thread_info[i].is_signal);

            pthread_mutex_unlock(&this->thread_info[i].thread_lock);
            return 1;
        }
        else
        {
        	//同上
            this->thread_info[i].is_signal = TRUE;
            this->thread_info[i].is_busy = FALSE;
            this->thread_info[i].func = pFunc;
            this->thread_info[i].input = pInput;
    	    this->thread_info[i].output = pOutput;
            pthread_cond_signal(&this->thread_info[i].thread_cond);

            printf("Thread Pool : tp_process_job add send signal. id : %d, all num : %d, waitLoop : %d, wait : %d, busy : %d, signal : %d.\n",i+1, this->cur_th_num, waitLoop, this->thread_info[i].is_wait, this->thread_info[i].is_busy, this->thread_info[i].is_signal);

            pthread_mutex_unlock(&this->thread_info[i].thread_lock);
        }

    }
    else
    {
        pthread_mutex_unlock(&this->tp_lock);
        printf("Thread Pool : add new failed!\n");
        return TRUE;
    }
    return FALSE;
}


//找到当前线程的队列编号
int tp_get_thread_by_id(tp_thread_pool *this, pthread_t id)
{
    int i;

    for(i=0; i<this->cur_th_num; i++)
    {
        if(id == this->thread_info[i].thread_id) //在线程池中寻找和传参一样的ID值，返回该线程在线程池中的队列编号
            return i;
    }
    return -1;
}

//增加一个新的线程
int tp_add_thread(tp_thread_pool *this)
{
    int err;
    tp_thread_info *new_thread;					//新建一个线程属性

    if(this->max_th_num <= this->cur_th_num)	//若新建了一个线程之后，当前线程数超过线程池最大线程数则返回0
    {
        printf("Thread Pool : The thread poor is full(%d/%d).\n", this->cur_th_num, this->max_th_num);
        return FALSE;
    }

    new_thread = &this->thread_info[this->cur_th_num]; //将新建线程插入到线程池队列

	//互斥锁和条件变量等初始化
    err = pthread_cond_init(&new_thread->thread_cond, NULL);
    if (0 != err)
    {
        printf("Thread Pool : add cond_init ret %d, all: %d\n", err, this->cur_th_num);
        return FALSE;
    }

    err = pthread_mutex_init(&new_thread->thread_lock, NULL);
    if (0 != err)
    {
        printf("Thread Pool : add mutex_init ret %d, all: %d\n", err, this->cur_th_num);
        return FALSE;
    }

    new_thread->is_signal = FALSE;
    new_thread->is_wait = FALSE;
    new_thread->is_destroy = FALSE;
    new_thread->is_busy = TRUE;

    this->cur_th_num = this->cur_th_num + 1; //将当前线程数量加1

    err = pthread_create(&new_thread->thread_id, NULL, tp_work_thread, this); //创建新工作线程tp_work_thread
    if(0 != err) //若失败，将当前线程数量减1恢复原数量
    {
        printf("Thread Pool : create new thread failed.\n");
        this->cur_th_num = this->cur_th_num - 1;
        return FALSE;
    }

    return TRUE;
}

//对于删除操作
void tp_doNothing_forDelete(tp_thread_pool *this)
{
    int waitLoop = 0;

    if (NULL == this)
    {
        printf("Thread Pool : delete thread FAIL because of NULL!\n");
        return;
    }

    this->thread_info[this->cur_th_num-1].is_destroy = TRUE;//查看该线程（编号最后一个线程）是否销毁
    printf("Thread Pool : tp_delete_thread set destory %d TRUE. \n", (this->cur_th_num));

    while (1)
    {
        usleep(10000);
        waitLoop++;
        if (waitLoop >= 1000)
        {
            break;
        }
    }

    this->thread_info[this->cur_th_num-1].is_destroy = FALSE; //查看该线程（编号最后一个线程）是否销毁

    printf("Thread Pool : delete thread %d FAIL because of timeout!\n", (this->cur_th_num));

    return;
}

//线程池销毁线程，从编号最后的、空闲的线程开始销毁
int tp_delete_thread(tp_thread_pool *this)
{
    int ret = 0;
    int i = 0;
    int waitloop = 0;

    pthread_mutex_lock(&this->tp_lock);

    if(this->cur_th_num <= this->min_th_num)				//若当前线程数量小于线程池最小数量，无需销毁线程
    {
        pthread_mutex_unlock(&this->tp_lock);
        printf("Thread Pool : No need to delete thread!\n");
        return FALSE;
    }

    i = this->cur_th_num - 1;
    pthread_mutex_lock(&this->thread_info[i].thread_lock); 	//为线程池中编号最后的、空闲的线程上锁

    if(this->thread_info[i].is_busy)						//若该线程在工作，解锁
    {
        pthread_mutex_unlock(&this->thread_info[i].thread_lock);
        pthread_mutex_unlock(&this->tp_lock);
        printf("Thread Pool : delete the last thread(%d) busy.\n", (i+1));
        return FALSE;
    }

	//删除该空闲线程操作
    this->thread_info[i].is_busy  = FALSE;
    this->thread_info[i].func = (void (*)(void *))tp_doNothing_forDelete; 	//将当前线程指向tp_work_thread func函数//(void (*)(void *)):函数返回值数据类型 （*指针变量名）（函数的实际参数或者函数参数的类型）
    this->thread_info[i].input = this;										//tp_work_thread input参数
    pthread_cond_signal(&this->thread_info[i].thread_cond); 				//向tp_work_thread的thread_cond发信号

    printf("Thread Pool : delete thread %d, cur num: %d, wait: %d, signal: %d, busy: %d\n",(i+1), this->cur_th_num, this->thread_info[i].is_wait, this->thread_info[i].is_signal, this->thread_info[i].is_busy);

    pthread_mutex_unlock(&this->thread_info[i].thread_lock);				//解锁当前线程

    while (this->thread_info[i].is_destroy != TRUE)							//为该线程等待一段时间
    {
        usleep(10000);
        waitloop++;
        if (waitloop >= 100)
        {
            break;
        }
    }

    ret = pthread_mutex_destroy(&this->thread_info[i].thread_lock);		//销毁当前线程互斥锁
    if (0 != ret)
    {
        printf("Thread Pool : mutex destroy failed (ret %d) on thread %d (waitloop %d).\n", ret, (i+1), waitloop);
    }

    ret = pthread_cond_destroy(&this->thread_info[i].thread_cond); 		//销毁当前线程条件变量
    if (0 != ret)
    {
        printf("Thread Pool : cond destroy failed (ret %d) on thread %d (waitloop %d).\n", ret, (i+1), waitloop);
    }

    //ret = pthread_kill(this->thread_info[this->cur_th_num-1].thread_id, SIGKILL);
    ret = pthread_cancel(this->thread_info[i].thread_id);				//申请销毁线程
    if (0 != ret)
    {
       	printf("Thread Pool : cancel failed (ret %d) on thread %d (waitloop %d).\n", ret, (i+1), waitloop);
    }

    this->cur_th_num--;					//销毁了一个线程，使得当前线程池线程数量减1
    pthread_mutex_unlock(&this->tp_lock);

    printf("Thread Pool : cancel success thread %d, cur num %d (waitloop %d)!\n", (i+1), this->cur_th_num, waitloop);

    return TRUE;
}


//获得线程池信息，检查是否需要销毁部分空闲线程，回收系统资源
int  tp_get_tp_status(tp_thread_pool *this)
{
    int busy_num = 0;
    int i;
    for(i=0;i<this->cur_th_num;i++) //查看当前线程池线程中的工作线程数量
    {
        if(this->thread_info[i].is_busy)
            busy_num++;
    }

    printf("Thread Pool : get_tp_status(%d/%d/%d)(busy %d) .\n", this->min_th_num, this->cur_th_num, this->max_th_num, busy_num);

    if(busy_num/(this->cur_th_num) < BUSY_THRESHOLD)//当线程池内只有小部分线程处于工作状态时，线程池将自动销毁一部分的线程，回收系统资源。BUSY_THRESHOLD为0.5
    {

		printf("Thread Pool : get_tp_status(%d/%d) to delete.\n", busy_num, this->cur_th_num);
        return FALSE;
    }
    else
    {
        printf("Thread Pool : get_tp_status(%d/%d) not to delete.\n", busy_num, this->cur_th_num);
        return TRUE;
    }
}

//工作线程
void *tp_work_thread(void *pthread)
{
    pthread_t curid;
    int nseq;
    tp_thread_pool *this = (tp_thread_pool*)pthread;

    curid = pthread_self(); 						//获得该线程自身ID
    nseq = this->get_thread_by_id(this, curid);		//返回该线程在线程池中的队列编号

    pthread_detach(pthread_self());

    if(nseq < 0)
    {
        printf("Thread Pool : work thread(ID %d / curid %d) is wrong.\n", nseq+1, (int)curid);
        pthread_exit(NULL);
    }

    while(TRUE)
    {
        pthread_mutex_lock(&this->thread_info[nseq].thread_lock); 	//在线程池中对编号为nseq线程加锁
        this->thread_info[nseq].is_wait = TRUE;						//设置为真，等待被调用

        do
        {
            printf("Thread Pool : tp_work_thread begin wait id: %d, wait: %d, signal: %d, busy: %d\n",(nseq + 1), this->thread_info[nseq].is_wait, this->thread_info[nseq].is_signal, this->thread_info[nseq].is_busy);

			pthread_cond_wait(&this->thread_info[nseq].thread_cond, &this->thread_info[nseq].thread_lock); //挂起该线程

            printf("Thread Pool : tp_work_thread after wait id: %d, wait: %d, signal: %d, busy: %d\n",(nseq + 1), this->thread_info[nseq].is_wait, this->thread_info[nseq].is_signal, this->thread_info[nseq].is_busy);

        }while((this->thread_info[nseq].is_signal) != TRUE);  	//挂起该线程，等待信号

        this->thread_info[nseq].is_busy = TRUE;				  	//被唤醒后，该线程为工作状态
        this->thread_info[nseq].is_wait = FALSE;				//表示不能被调用
        this->thread_info[nseq].is_signal = FALSE;				//没有收到signal_cond信号

        printf("Thread Pool : tp_work_thread got signal (%d/%d) busy %d.\n", (nseq + 1), this->cur_th_num, this->thread_info[nseq].is_busy);

        pthread_mutex_unlock(&this->thread_info[nseq].thread_lock);

        printf("Thread Pool : tp_work_thread got signal unlock(%d/%d) busy %d.\n", (nseq + 1), this->cur_th_num, this->thread_info[nseq].is_busy);

        (*(this->thread_info[nseq].func))(this->thread_info[nseq].input); //执行用户定义的函数func以及参数input，直至退出

        printf("Thread Pool : the callback return(%d/%d).\n", (nseq + 1), this->cur_th_num);

        pthread_mutex_lock(&this->thread_info[nseq].thread_lock);
        this->thread_info[nseq].is_busy = FALSE; 						   //表示该线程现在处于空闲状态
        pthread_mutex_unlock(&this->thread_info[nseq].thread_lock);

        printf("Thread Pool : set free (%d/%d) after the callback return.\n", (nseq + 1), this->cur_th_num);
    }
}


//管理线程，监测是否需要销毁部分空闲线程
void *tp_manage_thread(void *pthread)
{
    tp_thread_pool *this = (tp_thread_pool*)pthread;

    pthread_detach(pthread_self());

    sleep(MANAGE_INTERVAL);

    printf("Thread Pool : tp_manage_thread(%d/%d/%d) .\n", this->min_th_num, this->cur_th_num, this->max_th_num); //当程池中的最小、当前、最大线程数量

    do{
        printf("Thread Pool : manage_thread do\n");
        if( this->get_tp_status(this) == 0 ) //获得当前线程池状态信息，检查是否需要销毁部分空闲线程。返回0需要销毁一部分空闲线程，1不销毁
        {
            printf("Thread Pool : manage_thread begin to delete.\n");
            do
            {
                if( !this->delete_thread(this) ) //
                {
                    printf("Thread Pool : manage_thread delete done.\n");
                    break;
                }
            }while(TRUE);
        }
        sleep(MANAGE_INTERVAL);
    }while(TRUE);
    pthread_exit(NULL);
}
