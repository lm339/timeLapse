#include <stdio.h>
#include <stdlib.h>

#include <glib.h>
#include <wiringPi.h>
#include "time.h"
#include "unistd.h"
#include <sys/time.h>
static GMainLoop *m_main_loop;

#define CLOUD_NAME "lighthouse@lm339.cn"
#define LOCAL_PATH "/home/pi/image/"
#define CLOID_PATH "/home/lighthouse/image/"

typedef struct
{
	char pathName[128];
	char oldFrameNmae[128];
	unsigned char PrintState ;

}  sPrintStatus;

sPrintStatus PrintStatus = {"\0","\0",0};

#define KEY_IO 		9

unsigned char key_down =0;

void key_interrupt_(void)
{
	key_down = 1;
}

unsigned char key_io_init(void)
{
	wiringPiSetup();
	pinMode(KEY_IO,INPUT);            //配置1脚为输入
    pullUpDnControl(KEY_IO,PUD_UP);  //将1脚上拉到3.3v
	return wiringPiISR (KEY_IO, INT_EDGE_FALLING,  key_interrupt_);
}

unsigned int gettime(void)
{
	struct timeval temp;
	gettimeofday(&temp,NULL);
	return temp.tv_sec;
}

unsigned char lm339GetTime(char *data)
{

	struct tm *t;
	time_t now;

	time(&now);

	t = localtime(&now);

	sprintf(data,"%08d_%02d%02d%02d%02d%02d%02d",
			gettime(),
			t->tm_year+1900,
			t->tm_mon+1,t->tm_mday,
			t->tm_hour+7,t->tm_min,t->tm_sec);
	return 0;
}
//短按
unsigned char key_response(void)
{
 	char tempCmd[512]  ={'\0'};
	char nameTime[128] = {'\0'};
	char oldName[256] = {'\0'};
	lm339GetTime(nameTime);

	sprintf(oldName,"%s%s/%s.jpg",LOCAL_PATH,
			PrintStatus.PrintState?PrintStatus.pathName:"temp",nameTime);

	memcpy(PrintStatus.oldFrameNmae,oldName,strlen(oldName)+1);

	sprintf(tempCmd,"raspistill -t 200  -br 55 -o %s",oldName);
	printf("tempCmd = %s \n",tempCmd);
	system(tempCmd); 
	printf("cmd over \n");


	//上传一帧
	sprintf(tempCmd,"scp %s %s:%s%s/%s.jpg &\n",PrintStatus.oldFrameNmae,
			CLOUD_NAME,CLOID_PATH,PrintStatus.PrintState?PrintStatus.pathName:"temp",nameTime);
	printf("tempCmd = %s",tempCmd);
	system(tempCmd); 


	return 0;
}

//g_thread_new("hex_download",(GThreadFunc)hex_download_jlink, NULL);
//需要一个变量存储上一帧的文件名，避免去找
unsigned char longLongClick(void)//开始打印，刚刚拍的是第一帧
{
	//在云端和本地同时生成一个以当前时间戳为名字的文件夹，保存文件夹的name
	//上传刚刚拍的那一帧图片
	char cmd_temp[1024] = {'\0'};
    char nameTime[256] = {'\0'};
	lm339GetTime(nameTime);
	memcpy(PrintStatus.pathName,nameTime,strlen(nameTime)+1);//存储这个一次的路径名
	PrintStatus.PrintState = 1;//标志设置为打印中

	//本地创建文件夹
	sprintf(cmd_temp,"mkdir %s%s\n",LOCAL_PATH,nameTime);
	printf("cmd_temp = %s",cmd_temp);
	system(cmd_temp); //创建文件夹

	//云端创建文件夹
	sprintf(cmd_temp,"ssh %s \"mkdir %s%s\"\n",CLOUD_NAME,CLOID_PATH,nameTime);
	printf("cmd_temp = %s",cmd_temp);
	system(cmd_temp); //创建文件夹

	//把上一帧复制到新的文件夹，第一帧
	sprintf(cmd_temp,"cp %s %s%s/%s.jpg\n",PrintStatus.oldFrameNmae,LOCAL_PATH,nameTime,nameTime);
	printf("cmd_temp = %s",cmd_temp);
	system(cmd_temp); 

	//上传第一帧
	sprintf(cmd_temp,"scp %s %s:%s%s/%s.jpg &\n",PrintStatus.oldFrameNmae,CLOUD_NAME,CLOID_PATH,nameTime,nameTime);
	printf("cmd_temp = %s",cmd_temp);
	system(cmd_temp); 


	return 0;
}

unsigned char longClick(void)
{
	if(PrintStatus.PrintState ==0 ) return 1;

	PrintStatus.PrintState = 0;
	
	//开始压制视频
	char cmd_temp[1024] = {'\0'};
	sprintf(cmd_temp,"ssh %s \"%s../image2video/image2video %s%s/\"\n",
			CLOUD_NAME,CLOID_PATH,CLOID_PATH,PrintStatus.pathName);
	printf("cmd_temp = %s",cmd_temp);
	system(cmd_temp); //开始压制视频

	//下载视频到本地
	sprintf(cmd_temp,"scp %s:%s%s/output.avi /%s%s/%s.avi",
			CLOUD_NAME,CLOID_PATH,PrintStatus.pathName,
			LOCAL_PATH,PrintStatus.pathName,PrintStatus.pathName);
	printf("cmd_temp = %s",cmd_temp);
	system(cmd_temp); 

	return 0;	
}

static unsigned char key1_state =0;
unsigned char key1_logic(void)
{
	
	static unsigned int time_cout = 0;
	if(key1_state != 0) return 1;
	if(key_down == 0)
	{
		time_cout = 0;	
		return 1;
	}
	time_cout ++;

	if(digitalRead(KEY_IO) == 1 &&time_cout != 0 )
	{
		
		key_down = 0;
		printf("time_cout = %d \n",time_cout);
		if(time_cout > 200)
		{
			printf("long long click \n");
			longLongClick();
		}
		else if(time_cout > 80)
		{
			printf("long  click \n");
			longClick();
		}
		time_cout =0;
		return 1;
	} 
	if(time_cout == 3)  
	{
		key1_state = 1;
		printf("key test \r\n");
		key_response();
		key1_state = 0;
	}
	return 1;
}


gboolean key_timer_h(gpointer data)
{
	key1_logic();
	return 1;
}

int main()
{

	printf("key init,%x \r\n",key_io_init());

	g_timeout_add(30,key_timer_h,NULL);
	m_main_loop = g_main_loop_new(NULL, 0);
	g_main_loop_run(m_main_loop);
}
