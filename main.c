/*****************************************************************************/
//  文件名   ：main.c
//  概要     ： 入口函数
//  作者     ： sunyifeng
//  版本控制 ； 初版  2018-05-10
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>	    /* for signal */
#include <execinfo.h> 	/* for backtrace() */

#include "thread_pool.h"

void work(void* arg)
{
	char *p= (char*) arg;

	printf("threadpoolp  callback fuction : %s.\n", p);

	sleep(1);

	return ;
}

int main()
{
    int ret = 0;

    ret = thread_pool_init(300, 500); //线程池初始化
    if (VOS_OK != ret)
    {
        printf("thread_pool_init Fail!\n");
    }


	tp_thread_pool *pPoolTemp;

	get_thread_pool(&pPoolTemp);

	ret = tp_process_job(pPoolTemp, work, "1",NULL);
	ret = tp_process_job(pPoolTemp, work, "2",NULL);
	ret = tp_process_job(pPoolTemp, work, "3",NULL);
	ret = tp_process_job(pPoolTemp, work, "4",NULL);
	ret = tp_process_job(pPoolTemp, work, "5",NULL);
	ret = tp_process_job(pPoolTemp, work, "6",NULL);
	ret = tp_process_job(pPoolTemp, work, "7",NULL);
	ret = tp_process_job(pPoolTemp, work, "8",NULL);
	ret = tp_process_job(pPoolTemp, work, "9",NULL);
	ret = tp_process_job(pPoolTemp, work, "10",NULL);


	sleep(5);

    tp_close(pPoolTemp);//销毁线程池

    return VOS_OK;


}


