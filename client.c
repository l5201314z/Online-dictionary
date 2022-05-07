#include<stdio.h>
#include<stdlib.h>
#include <sys/types.h>  
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <string.h>
#include <unistd.h>
#define  N 32
#define  R 1   //user - register
#define  L 2   //user - login
#define  Q 3   //user - query
#define  H 4   //user - history

//定义通信双方的信息结构体
typedef struct{
    int type;
    char name[N];
    char data[256];
}MSG;
int do_register(int sockfd,MSG *msg);  //注册模块
int do_login(int sockfd,MSG *msg);     //登录模块
int do_query(int sockfd,MSG *msg);     //查询模块
int do_history(int sockfd,MSG *msg);   //历史记录模块

// .client 192.168.204.8 5002
int main(int argc,const char *argv[]){

    struct sockaddr_in serveraddr;
    int sockfd;
    int cmd;
    MSG msg;
    //入口参数判断
    if(argc != 3){
        printf("Input Error: %s <ip> <port>\n", argv[0]);
        return -1;
    }
    //端口号限制
    if (atoi(argv[2]) < 5000 )
	{
		printf("Input Port Error.\n");
		return -1;
	}

    /*1.创建socket sockfd*/ 
    if((sockfd = socket(AF_INET,SOCK_STREAM,0)) < 0){
        perror("fail to socket\n");
        return -1;
    }
    /*2.连接服务器 */

    /*2.1 填充struct sockaddr_in结构体变量 */
    bzero(&serveraddr,sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = inet_addr(argv[1]);
    serveraddr.sin_port = htons(atoi(argv[2]));

    if(connect(sockfd,(struct sockaddr *) &serveraddr,sizeof(serveraddr)) < 0){
        perror("fail to connect\n");
        return -1;
    }
    while (1)
    {
        printf("***********************************\n");
        printf("* 1.register    2.login    3.quit *\n");
        printf("***********************************\n");
        printf("Please choose:");
        scanf("%d",&cmd);
        scanf("%d",&cmd);
        scanf("%d",&cmd);
        //getchar();
        switch (cmd)
        {
        case 1:
            do_register(sockfd,&msg);
            break;
        case 2:
            if(do_login(sockfd,&msg)==1){
                goto next;
            }

            break;
        case 3:
            close(sockfd);
            exit(0);
        default:
            printf("Invalid data cmd\n");
            break;
        }
        
    }

next:
    while(1){
        printf("*********************************************\n");
        printf("* 1.query_word    2.history_record   3.quit *\n");
        printf("*********************************************\n");
        printf("Please choose:");
        scanf("%d",&cmd);
        getchar();
        switch (cmd)
        {
            case 1:
                do_query(sockfd,&msg);
                break;
            case 2:
                do_history(sockfd,&msg);
                break;
            case 3:
                close(sockfd);
                exit(0);
                break;
            default:
                printf("Invalid data cmd\n");
        }

    }
    return 0;
}

//注册模块
int do_register(int sockfd,MSG *msg){
    printf("register ...\n");
    msg->type = R;
    printf("Input name:");
    scanf("%s",msg->name);
    getchar();
    printf("Input passwd:");
    scanf("%s",msg->data);
    getchar();
    if(send(sockfd,msg,sizeof(MSG),0) < 0){//send发送
        printf("fail to send ..\n");
        return -1;
    }
    if(recv(sockfd,msg,sizeof(MSG),0) < 0){
        printf("fail to send ..\n");
        return -1;
    }
    // OK! or user alread exist
    printf("%s\n",msg->data);

    return 0;

}

int do_login(int sockfd,MSG *msg){
    printf("login ...\n");
    msg->type = L;
    printf("Input name:");
    scanf("%s",msg->name);
    getchar();
    printf("Input passwd:");
    scanf("%s",msg->data);
    getchar();
    if(send(sockfd,msg,sizeof(MSG),0) < 0){//send发送
        printf("fail to send ..\n");
        return -1;
    }
    if(recv(sockfd,msg,sizeof(MSG),0) < 0){
        printf("fail to send ..\n");
        return -1;
    }
    if(strncmp(msg->data,"OK",3) == 0){
        printf("Login OK!\n");
        return 1;
    }else{
        printf("%s\n",msg->data);
    }
    
    return 0;
}

int do_query(int sockfd,MSG *msg){
    printf("query ...\n");
    msg->type = Q;
    puts("---------------");
    while (1)
    {
        printf("Input word:");
        scanf("%s",msg->data);
        getchar();
        //客户端输入#返回到上一级菜单
        if(strncmp(msg->data,"#",1) == 0){
            break;
        }
        //将查询的单词发送服务器
        if(send(sockfd,msg,sizeof(MSG),0) < 0){//send发送
            printf("fail to send ..\n");
            return -1;
        }
        //等待接收服务器传递回来的单词
        if(recv(sockfd,msg,sizeof(MSG),0) < 0){//recv接收
            printf("fail to recv ..\n");
            return -1;
        }
        printf("%s\n", msg->data);

    }
    return 0;
}

int do_history(int sockfd,MSG *msg){
    printf("history ...\n");
    msg->type = H;
    puts("---------------");
    //发送服务器
    if(send(sockfd,msg,sizeof(MSG),0) < 0){//send发送
        printf("fail to send ..\n");
        return -1;
    }
    //接受服务器,传递回来的历史记录信息
    int index = 0;
    while (1)
    {
        //等待接收服务器传递回来的单词
        if(recv(sockfd,msg,sizeof(MSG),0) < 0){//recv接收
            printf("fail to recv ..\n");
            return -1;
        }
        if(msg->data[0] == '\0'){
            break;
        }
        //输出历史记录信息
        printf("%s\n", msg->data);
        index++;
    }
    printf("index:%d\n",index);
    return 0;
}