/*****************************************************************************/
//  �ļ���   ��thread_poor.c
//  ��Ҫ     ��
//  ����     �� wangqingchuan
//  �汾���� �� ����  2019-02-13
/*****************************************************************************/
#include <stdlib.h>
#include <string.h>


#include "thread_pool.h"

//����̳߳�
void get_thread_pool(tp_thread_pool **pPooltemp)
{
    *pPooltemp = pool;
}

//�̳߳ش����Լ���ʼ��
int thread_pool_init(int min_num,int max_num)
{
    pool = creat_thread_pool(min_num,max_num);		//�����̳߳�
    if(tp_init(pool)==FALSE)						//��ʼ���̳߳�
    {
        printf("Thread Pool : tp_init is wrong!\n");
        return TRUE;
    }
    return FALSE;
}

//�����̳߳�
tp_thread_pool *creat_thread_pool(int min_num, int max_num)
{
    tp_thread_pool *this;											//�й��̳߳ز����Ľӿ���Ϣ
    this = (tp_thread_pool*)malloc(sizeof(tp_thread_pool));

    memset(this, 0, sizeof(tp_thread_pool));

	//��ʼ��ָ������ܺ���
    this->init = tp_init;
    this->close = tp_close;
    this->process_job = tp_process_job;
    this->get_thread_by_id = tp_get_thread_by_id;
    this->add_thread = tp_add_thread;
    this->delete_thread = tp_delete_thread;
    this->get_tp_status = tp_get_tp_status;


    this->min_th_num = min_num;									//��С�߳�����
    this->cur_th_num = this->min_th_num;						//��ǰ�߳�����
    this->max_th_num = max_num;									//����߳�����

    pthread_mutex_init(&this->tp_lock, NULL);

    if(NULL != this->thread_info)
      free(this->thread_info);

    this->thread_info = (tp_thread_info*)malloc(sizeof(tp_thread_info)*this->max_th_num); //����max_th_num������߳���Ϣ�ڴ�ռ�

    return this;
}

//�̳߳س�ʼ��
int tp_init(tp_thread_pool *this)
{
    int i;
    int err;

    for(i=0;i<this->min_th_num;i++) //����min_th_num���߳�
    {
    	//�̳߳ظ����̵߳���Ϣ��ʼ��
        pthread_cond_init(&this->thread_info[i].thread_cond, NULL);		//Ϊÿ���̵߳�����������ʼ��
        pthread_mutex_init(&this->thread_info[i].thread_lock, NULL);	//Ϊÿ���߳�����

        this->thread_info[i].is_signal = FALSE;
        this->thread_info[i].is_wait = FALSE;
        this->thread_info[i].is_busy = FALSE;
        this->thread_info[i].is_destroy = FALSE;

        err = pthread_create(&this->thread_info[i].thread_id, NULL, tp_work_thread, this);	//���������߳�
        if(0 != err)
        {
            printf("Thread Pool : creat work thread failed when init\n");
            return FALSE;
        }
    }

    err = pthread_create(&this->manage_thread_id, NULL, tp_manage_thread, this);			//���������߳�
    if(0 != err)
    {
        printf("Thread Pool : creat manage thread failed\n");
        return FALSE;
    }

    return TRUE;
}

//�����̳߳�
void tp_close(tp_thread_pool *this)
{
    int i;

    for(i=0;i<this->cur_th_num;i++)
    {
    	//�����̳߳��е�ǰ�����߳�
        //pthread_kill(this->thread_info[i].thread_id, SIGKILL);
        pthread_cancel(this->thread_info[i].thread_id);
        pthread_mutex_destroy(&this->thread_info[i].thread_lock);
        pthread_cond_destroy(&this->thread_info[i].thread_cond);
    }

	//���ٹ����߳�
    //pthread_kill(this->manage_thread_id, SIGKILL);
    pthread_cancel(this->manage_thread_id);
    pthread_mutex_destroy(&this->tp_lock);

    printf("tp_close: kill manage thread %lu\n", this->manage_thread_id);

    free(this->thread_info);
}

//����ӿ�
int tp_process_job(tp_thread_pool *this, void(*pFunc)(void*), void *pInput ,void *pOutput)
{
    int i;
    int waitLoop = 0;

    printf("Thread Pool : tp_process_job(%d/%d/%d).\n", this->min_th_num, this->cur_th_num, this->max_th_num);

    for (i = 0; i < this->cur_th_num; i++)
	{

        printf("Thread Pool : tp_process_job for(%d/%d).\n", i+1, this->cur_th_num);

        pthread_mutex_lock(&this->thread_info[i].thread_lock); 	//�Ӷ�����0��ʼ�ң�����

        printf("Thread Pool : tp_process_job get lock(%d/%d).\n", i+1, this->cur_th_num);

        if(!this->thread_info[i].is_busy)  						//Ѱ�����̳߳��п����߳�
		{
            printf("Thread Pool : tp_process_job get free(%d/%d).\n", i+1, this->cur_th_num);

		    this->thread_info[i].is_busy = TRUE;				//�ҵ������̺߳����ø��߳�Ϊæµ

		    if(this->thread_info[i].is_wait != TRUE)          	//�жϸ��߳��Ƿ��ܹ�������
            {
                printf("Thread Pool : tp_process_job wait fail. id : %d, all num : %d, wait : %d, busy : %d, signal : %d.\n",i+1, this->cur_th_num,  this->thread_info[i].is_wait, this->thread_info[i].is_busy, this->thread_info[i].is_signal);

                this->thread_info[i].is_busy = FALSE;			//�����̲߳��ܱ���ý��ñ�־λ�ָ�ԭ����״̬
                pthread_mutex_unlock(&this->thread_info[i].thread_lock);
                return 1; 										//ʧ�ܷ���
            }
            else
            {
                this->thread_info[i].is_signal = TRUE;          //���ø��߳��յ�cond_signal�ź�
                this->thread_info[i].func = pFunc;				//����ָ��ָ����Ϣ���ദ����
                this->thread_info[i].input = pInput;			//ָ����Ϣ���ຯ���������
                this->thread_info[i].output = pOutput;			//ָ����Ϣ���ຯ���������

                pthread_cond_signal(&this->thread_info[i].thread_cond);  //���Ѹ��̣߳�tp_work_thread��

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


	//���̳߳��е�ǰ�����̶߳�æµ��thread_info[i].is_busy = TRUE���������̳߳����¼�һ���̲߳�ʹ��
    pthread_mutex_lock(&this->tp_lock); //���̳߳�����

    if(this->add_thread(this))			//������̳߳ɹ�
	{
        i = this->cur_th_num - 1;		//cur_th_num��add_thread�Ѿ���1��Ϊ����ʾ��ȷ���̶߳���λ��i����1

        printf("Thread Pool : add new one. id : %d, all num : %d.\n", i+1, this->cur_th_num);

        pthread_mutex_unlock(&this->tp_lock);

        waitLoop = 0;
        while (this->thread_info[i].is_wait != TRUE)		//�жϸ��߳��Ƿ��ܹ�������
        {
            usleep(1000);									//�ȴ�
            waitLoop++;
            if (waitLoop >= 10)
            {
                break;
            }
        }

        pthread_mutex_lock(&this->thread_info[i].thread_lock);

        if (waitLoop >= 10)									//���涨��ʱ������Բ��ܱ�����
        {
            this->thread_info[i].is_busy = FALSE;			//���߳�����Ϊ����

            printf("Thread Pool : tp_process_job add waitloop fail. id : %d, all num : %d, waitLoop : %d, wait : %d, busy : %d, signal : %d.\n",i+1, this->cur_th_num, waitLoop, this->thread_info[i].is_wait, this->thread_info[i].is_busy, this->thread_info[i].is_signal);

            pthread_mutex_unlock(&this->thread_info[i].thread_lock);
            return 1;
        }
        else
        {
        	//ͬ��
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


//�ҵ���ǰ�̵߳Ķ��б��
int tp_get_thread_by_id(tp_thread_pool *this, pthread_t id)
{
    int i;

    for(i=0; i<this->cur_th_num; i++)
    {
        if(id == this->thread_info[i].thread_id) //���̳߳���Ѱ�Һʹ���һ����IDֵ�����ظ��߳����̳߳��еĶ��б��
            return i;
    }
    return -1;
}

//����һ���µ��߳�
int tp_add_thread(tp_thread_pool *this)
{
    int err;
    tp_thread_info *new_thread;					//�½�һ���߳�����

    if(this->max_th_num <= this->cur_th_num)	//���½���һ���߳�֮�󣬵�ǰ�߳��������̳߳�����߳����򷵻�0
    {
        printf("Thread Pool : The thread poor is full(%d/%d).\n", this->cur_th_num, this->max_th_num);
        return FALSE;
    }

    new_thread = &this->thread_info[this->cur_th_num]; //���½��̲߳��뵽�̳߳ض���

	//�����������������ȳ�ʼ��
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

    this->cur_th_num = this->cur_th_num + 1; //����ǰ�߳�������1

    err = pthread_create(&new_thread->thread_id, NULL, tp_work_thread, this); //�����¹����߳�tp_work_thread
    if(0 != err) //��ʧ�ܣ�����ǰ�߳�������1�ָ�ԭ����
    {
        printf("Thread Pool : create new thread failed.\n");
        this->cur_th_num = this->cur_th_num - 1;
        return FALSE;
    }

    return TRUE;
}

//����ɾ������
void tp_doNothing_forDelete(tp_thread_pool *this)
{
    int waitLoop = 0;

    if (NULL == this)
    {
        printf("Thread Pool : delete thread FAIL because of NULL!\n");
        return;
    }

    this->thread_info[this->cur_th_num-1].is_destroy = TRUE;//�鿴���̣߳�������һ���̣߳��Ƿ�����
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

    this->thread_info[this->cur_th_num-1].is_destroy = FALSE; //�鿴���̣߳�������һ���̣߳��Ƿ�����

    printf("Thread Pool : delete thread %d FAIL because of timeout!\n", (this->cur_th_num));

    return;
}

//�̳߳������̣߳��ӱ�����ġ����е��߳̿�ʼ����
int tp_delete_thread(tp_thread_pool *this)
{
    int ret = 0;
    int i = 0;
    int waitloop = 0;

    pthread_mutex_lock(&this->tp_lock);

    if(this->cur_th_num <= this->min_th_num)				//����ǰ�߳�����С���̳߳���С���������������߳�
    {
        pthread_mutex_unlock(&this->tp_lock);
        printf("Thread Pool : No need to delete thread!\n");
        return FALSE;
    }

    i = this->cur_th_num - 1;
    pthread_mutex_lock(&this->thread_info[i].thread_lock); 	//Ϊ�̳߳��б�����ġ����е��߳�����

    if(this->thread_info[i].is_busy)						//�����߳��ڹ���������
    {
        pthread_mutex_unlock(&this->thread_info[i].thread_lock);
        pthread_mutex_unlock(&this->tp_lock);
        printf("Thread Pool : delete the last thread(%d) busy.\n", (i+1));
        return FALSE;
    }

	//ɾ���ÿ����̲߳���
    this->thread_info[i].is_busy  = FALSE;
    this->thread_info[i].func = (void (*)(void *))tp_doNothing_forDelete; 	//����ǰ�߳�ָ��tp_work_thread func����//(void (*)(void *)):��������ֵ�������� ��*ָ�����������������ʵ�ʲ������ߺ������������ͣ�
    this->thread_info[i].input = this;										//tp_work_thread input����
    pthread_cond_signal(&this->thread_info[i].thread_cond); 				//��tp_work_thread��thread_cond���ź�

    printf("Thread Pool : delete thread %d, cur num: %d, wait: %d, signal: %d, busy: %d\n",(i+1), this->cur_th_num, this->thread_info[i].is_wait, this->thread_info[i].is_signal, this->thread_info[i].is_busy);

    pthread_mutex_unlock(&this->thread_info[i].thread_lock);				//������ǰ�߳�

    while (this->thread_info[i].is_destroy != TRUE)							//Ϊ���̵߳ȴ�һ��ʱ��
    {
        usleep(10000);
        waitloop++;
        if (waitloop >= 100)
        {
            break;
        }
    }

    ret = pthread_mutex_destroy(&this->thread_info[i].thread_lock);		//���ٵ�ǰ�̻߳�����
    if (0 != ret)
    {
        printf("Thread Pool : mutex destroy failed (ret %d) on thread %d (waitloop %d).\n", ret, (i+1), waitloop);
    }

    ret = pthread_cond_destroy(&this->thread_info[i].thread_cond); 		//���ٵ�ǰ�߳���������
    if (0 != ret)
    {
        printf("Thread Pool : cond destroy failed (ret %d) on thread %d (waitloop %d).\n", ret, (i+1), waitloop);
    }

    //ret = pthread_kill(this->thread_info[this->cur_th_num-1].thread_id, SIGKILL);
    ret = pthread_cancel(this->thread_info[i].thread_id);				//���������߳�
    if (0 != ret)
    {
       	printf("Thread Pool : cancel failed (ret %d) on thread %d (waitloop %d).\n", ret, (i+1), waitloop);
    }

    this->cur_th_num--;					//������һ���̣߳�ʹ�õ�ǰ�̳߳��߳�������1
    pthread_mutex_unlock(&this->tp_lock);

    printf("Thread Pool : cancel success thread %d, cur num %d (waitloop %d)!\n", (i+1), this->cur_th_num, waitloop);

    return TRUE;
}


//����̳߳���Ϣ������Ƿ���Ҫ���ٲ��ֿ����̣߳�����ϵͳ��Դ
int  tp_get_tp_status(tp_thread_pool *this)
{
    int busy_num = 0;
    int i;
    for(i=0;i<this->cur_th_num;i++) //�鿴��ǰ�̳߳��߳��еĹ����߳�����
    {
        if(this->thread_info[i].is_busy)
            busy_num++;
    }

    printf("Thread Pool : get_tp_status(%d/%d/%d)(busy %d) .\n", this->min_th_num, this->cur_th_num, this->max_th_num, busy_num);

    if(busy_num/(this->cur_th_num) < BUSY_THRESHOLD)//���̳߳���ֻ��С�����̴߳��ڹ���״̬ʱ���̳߳ؽ��Զ�����һ���ֵ��̣߳�����ϵͳ��Դ��BUSY_THRESHOLDΪ0.5
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

//�����߳�
void *tp_work_thread(void *pthread)
{
    pthread_t curid;
    int nseq;
    tp_thread_pool *this = (tp_thread_pool*)pthread;

    curid = pthread_self(); 						//��ø��߳�����ID
    nseq = this->get_thread_by_id(this, curid);		//���ظ��߳����̳߳��еĶ��б��

    pthread_detach(pthread_self());

    if(nseq < 0)
    {
        printf("Thread Pool : work thread(ID %d / curid %d) is wrong.\n", nseq+1, (int)curid);
        pthread_exit(NULL);
    }

    while(TRUE)
    {
        pthread_mutex_lock(&this->thread_info[nseq].thread_lock); 	//���̳߳��жԱ��Ϊnseq�̼߳���
        this->thread_info[nseq].is_wait = TRUE;						//����Ϊ�棬�ȴ�������

        do
        {
            printf("Thread Pool : tp_work_thread begin wait id: %d, wait: %d, signal: %d, busy: %d\n",(nseq + 1), this->thread_info[nseq].is_wait, this->thread_info[nseq].is_signal, this->thread_info[nseq].is_busy);

			pthread_cond_wait(&this->thread_info[nseq].thread_cond, &this->thread_info[nseq].thread_lock); //������߳�

            printf("Thread Pool : tp_work_thread after wait id: %d, wait: %d, signal: %d, busy: %d\n",(nseq + 1), this->thread_info[nseq].is_wait, this->thread_info[nseq].is_signal, this->thread_info[nseq].is_busy);

        }while((this->thread_info[nseq].is_signal) != TRUE);  	//������̣߳��ȴ��ź�

        this->thread_info[nseq].is_busy = TRUE;				  	//�����Ѻ󣬸��߳�Ϊ����״̬
        this->thread_info[nseq].is_wait = FALSE;				//��ʾ���ܱ�����
        this->thread_info[nseq].is_signal = FALSE;				//û���յ�signal_cond�ź�

        printf("Thread Pool : tp_work_thread got signal (%d/%d) busy %d.\n", (nseq + 1), this->cur_th_num, this->thread_info[nseq].is_busy);

        pthread_mutex_unlock(&this->thread_info[nseq].thread_lock);

        printf("Thread Pool : tp_work_thread got signal unlock(%d/%d) busy %d.\n", (nseq + 1), this->cur_th_num, this->thread_info[nseq].is_busy);

        (*(this->thread_info[nseq].func))(this->thread_info[nseq].input); //ִ���û�����ĺ���func�Լ�����input��ֱ���˳�

        printf("Thread Pool : the callback return(%d/%d).\n", (nseq + 1), this->cur_th_num);

        pthread_mutex_lock(&this->thread_info[nseq].thread_lock);
        this->thread_info[nseq].is_busy = FALSE; 						   //��ʾ���߳����ڴ��ڿ���״̬
        pthread_mutex_unlock(&this->thread_info[nseq].thread_lock);

        printf("Thread Pool : set free (%d/%d) after the callback return.\n", (nseq + 1), this->cur_th_num);
    }
}


//�����̣߳�����Ƿ���Ҫ���ٲ��ֿ����߳�
void *tp_manage_thread(void *pthread)
{
    tp_thread_pool *this = (tp_thread_pool*)pthread;

    pthread_detach(pthread_self());

    sleep(MANAGE_INTERVAL);

    printf("Thread Pool : tp_manage_thread(%d/%d/%d) .\n", this->min_th_num, this->cur_th_num, this->max_th_num); //���̳��е���С����ǰ������߳�����

    do{
        printf("Thread Pool : manage_thread do\n");
        if( this->get_tp_status(this) == 0 ) //��õ�ǰ�̳߳�״̬��Ϣ������Ƿ���Ҫ���ٲ��ֿ����̡߳�����0��Ҫ����һ���ֿ����̣߳�1������
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
