#include<stdio.h>
#include<stdlib.h>
#include <sys/types.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <string.h>
#include <unistd.h>
#include <sqlite3.h>
#include <signal.h>
#include <time.h>
#define  N 32
#define  R 1   //user - register
#define  L 2   //user - login
#define  Q 3   //user - query
#define  H 4   //user - history
#define  DATEBASE "my.db"

//定义通信双方的信息结构体
typedef struct{
    int type;
    char name[N];
    char data[256];
}MSG;

int do_client(int acceptfd,sqlite3 *db);
int do_register(int acceptfd,MSG *msg,sqlite3 *db);
int do_login(int acceptfd,MSG *msg,sqlite3 *db);
int do_query(int acceptfd,MSG *msg,sqlite3 *db);
int do_history(int acceptfd,MSG *msg,sqlite3 *db);

// .client 192.168.204.8 5002
int main(int argc,const char *argv[]){

    int sockfd;
    int n;
    struct sockaddr_in serveraddr;
    MSG msg;
    sqlite3 *db;
    int acceptfd;
    pid_t pid;

    if(argc != 3){
        printf("Usage:%s server ip port \n",argv[0]);
        return -1;
    }
    //打开数据库
    if(sqlite3_open(DATEBASE,&db) != SQLITE_OK){
        printf("%s\n",sqlite3_errmsg(db));
        return -1;
    }else{
        printf("open DATABASE success ...\n");
    }
    /*1.创建socket sockfd*/ 
    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
        perror("fail to socket");
        return -1;
    }
    	/*优化4： 允许绑定地址快速重用 */
	int b_reuse = 1;
	setsockopt (sockfd, SOL_SOCKET, SO_REUSEADDR, &b_reuse, sizeof (int));

    /*2.连接服务器 */

    /*2.1 填充struct sockaddr_in结构体变量 */
    bzero(&serveraddr,sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    serveraddr.sin_port = htons(atoi(argv[2]));
    /*2.2 绑定 */	
    if(bind(sockfd,(struct sockaddr *) &serveraddr,sizeof(serveraddr)) <0){
        perror("fail to bind");
        return -1;
    }

    /*3. 调用listen()把主动套接字变成被动套接字 */
	if(listen(sockfd,5) < 0) {
		perror("fail to listen");
		exit(1);
	}
    //处理僵尸进程
    signal(SIGCHLD,SIG_IGN);

    while (1)
    {
        if((acceptfd = accept(sockfd,NULL,NULL)) < 0){
            perror("fail to acceptfd");
            return -1;
        }
        printf("Client socket OK ...\n");

        pid = fork();
        if(pid < 0){
            perror("fail to fork");
        }else if(pid == 0){   //子进程 处理客户端具体信息
            close(sockfd);
            do_client(acceptfd,db);

        }else{  //父进程 用来接收客户端的请求
            close(acceptfd);
        }
        
    }

    return 0;
}

int do_client(int acceptfd,sqlite3 *db){
    
    MSG msg;
    while (recv(acceptfd,&msg,sizeof(msg),0) > 0)
    {
        printf("type:%d\n",msg.type);
        switch (msg.type)
        {
        case R:
            do_register(acceptfd, &msg, db);
            break;
        case L:
            do_login(acceptfd, &msg, db);
            break; 
        case Q:
            do_query(acceptfd, &msg, db);
            break;    
        case H:
            do_history(acceptfd, &msg,db);
            break;                              
        default:
            printf("Invalid data msg\n");
            break;
        }
    
    }
    printf("client exit ...\n");
    close(acceptfd);
    exit(0);
    return 0;
}

int do_register(int acceptfd,MSG *msg,sqlite3 *db){
    char * errmsg;
    char sql[314] = {0};
    sprintf(sql,"insert into usr values('%s',%s)",msg->name,msg->data);
    printf("%s\n",sql);
    if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
        printf("%s\n",errmsg);
        strcpy(msg->data,"usr name already exist");
    }else{
        printf("client register OK!");
        strcpy(msg->data,"OK");
    }
    if(send(acceptfd,msg,sizeof(MSG),0) < 0){
        perror("fail to send");
        return 0;
    }
    
    return 0;
}
int do_login(int acceptfd,MSG *msg,sqlite3 *db){
    char * errmsg;
    char sql[334] = {0};
    char **resultp;
    int  nrow;
    int  ncloumn;
    sprintf(sql,"select * from usr where name = '%s' and pass = '%s'",msg->name,msg->data);

    printf("%s\n",sql);

    if(sqlite3_get_table(db,sql,&resultp,&nrow,&ncloumn,&errmsg) != SQLITE_OK){
        printf("%s\n",errmsg);
        return -1;
    }else{
        printf("get_table ok!\n");
    }
    //查询成功
    if(nrow == 1){
        strcpy(msg->data,"OK");
        send(acceptfd,msg,sizeof(MSG),0);
        return 1;
    }
    if(nrow == 0){ //密码或者用户名错误
        strcpy(msg->data,"usr/passwd wrong");
        send(acceptfd,msg,sizeof(MSG),0);
    }
    return 0;
}

int do_searcword(int acceptfd,MSG *msg,char word[]){
    FILE *fp;
    int len = 0;
    char temp[512];
    int  result;
    char *p;
    //打开文件,读取文件,进行比对
    if((fp = fopen("dict.txt","r")) == NULL){
        perror("fail to open \n");
        strcpy(msg->data,"Failed to open dict.txt");
        send(acceptfd,msg,sizeof(MSG),0);
        return -1;
    }
    //打印出客户端要查询的单词
    len = strlen(word);

    printf("%s,len =%d\n",word,len);
    //读文件来查询单词
    while (fgets(temp,512,fp) != NULL){
        //printf("temp:%s\n",temp);

        result = strncmp(temp,word,len);

        //printf("result:%d,%s\n",result,temp);

        if(result < 0){
            continue;
        }
        if(result > 0 || ((result==0)&&(temp[len]!=' '))){
            break;
        }

        //找打查询的单词
        p = temp + len;
        //printf("found word:%s\n",p);
        while(*p == ' '){
            p++;
        }
        //找到注释,跳跃过所有的空格
        strcpy(msg->data,p);

        //拷贝完毕 关闭文件
        fclose(fp);

        return 1;
    }

    fclose(fp);

    return 0;
}
int get_date(char *date){
    time_t rawtime;
    struct tm *tp;
    time(&rawtime);
    //进行时间的格式转换
    tp = localtime(&rawtime);

    sprintf(date,"%d-%d-%d %d:%d:%d",tp->tm_year+1900,tp->tm_mon+1,tp->tm_mday,tp->tm_hour,tp->tm_min,tp->tm_sec);
    return 0;
}

int do_query(int acceptfd,MSG *msg,sqlite3 *db){
    char word[64];
    int  found = 0;
    char date[128] = {};
    char sql[128] = {};
    char *errmsg;
    //拿出信息结构体中要查询的单词
    strcpy(word,msg->data);

    found = do_searcword(acceptfd,msg,word);

    //表示找到单词,将用户名 时间 单词 插入历史记录表中去
    if(found ==1){
        //需要获取系统时间
        get_date(date);
        //插入用户历史记录
        sprintf(sql,"insert into record values ('%s','%s','%s')",msg->name,date,word);
        printf("%s\n",sql);

        if(sqlite3_exec(db,sql,NULL,NULL,&errmsg) != SQLITE_OK){
            printf("%s\n",errmsg);
            return -1;
        }else{
            printf("Insert record done.\n");
        }

    }else{  //表示没有找到
        strcpy(msg->data,"Not found!");
    }
    printf("%s\n",msg->data);
    //将查询的结果发送给客户端
    if(send(acceptfd,msg,sizeof(MSG),0) < 0){//send发送
        printf("fail to send ..\n");
        return -1;
    }

    return 0;
}
//得到查询结果并且发送
//数据库中每找到一条记录自动执行一次回调函数
int history_callback(void *para, int f_num, char **f_value, char **f_name){
    //record name date word 
    int acceptfd;
    MSG msg;

    acceptfd = *((int*)para);

    sprintf(msg.data,"%s | %s",f_value[1],f_value[2]);

    send(acceptfd,&msg,sizeof(MSG),0);

    return 0;
}
int do_history(int acceptfd,MSG *msg,sqlite3 *db){
    char sql[128] = {};
    char *errmsg;

    sprintf(sql,"select  * from record where name = '%s' ",msg->name);

    if(sqlite3_exec(db,sql,history_callback,(void *)&acceptfd,&errmsg)!=SQLITE_OK){
        printf("%s\n",errmsg);
    }else{
        printf("Query record done ...\n");
    }
    //所有的记录发送完毕
    msg->data[0] = '\0';
    send(acceptfd,msg,sizeof(MSG),0);
    return 0;
}