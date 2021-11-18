#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//#include <ctype.h>
//#include <strings.h>
#include <unistd.h>
//#include <errno.h>
#include <fcntl.h>
//#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
//#include <sys/stat.h>

#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
//#include <pthread.h>

#define BUFSIZE 8192

struct
{
    char *ext;
    char *filetype;
} extensions [] = {
        {"gif", "image/gif" },
        {"jpg", "image/jpeg"},
        {"jpeg","image/jpeg"},
        {"png", "image/png" },
        {"zip", "image/zip" },
        {"gz",  "image/gz"  },
        {"tar", "image/tar" },
        {"htm", "text/html" },
        {"html","text/html" },
        {"exe","text/plain" },
        {0,0} };

void handle_socket(int fd)
{
    int j, file_fd, buflen, len;
    long i, ret;
    char * fstr;
    static char buffer[BUFSIZE+1];

    //讀取瀏覽器要求
    ret = read(fd,buffer,BUFSIZE);
    //連線有問題，結束
    if(ret==0||ret==-1)
    {   exit(3);}

    //處理字串:結尾補0,刪換行
    if(ret>0&&ret<BUFSIZE)
    {   buffer[ret] = 0;}
    else
    {   buffer[0] = 0;}

    for(i=0;i<ret;i++)
    {
        if(buffer[i]=='\r'||buffer[i]=='\n')
        {   buffer[i] = 0;}
    }

    //判斷GET命令
    if(strncmp(buffer,"GET ",4)&&strncmp(buffer,"get ",4))
    {   exit(3);}

    //分隔HTTP/1.0
    for(i=4;i<BUFSIZE;i++)
    {
        if(buffer[i]==' ')
        {
            buffer[i]=0;
            break;
        }
    }

    //當客戶端要求根目錄時讀取 index.html
    if(!strncmp(&buffer[0],"GET /\0",6)||!strncmp(&buffer[0],"get /\0",6) )
    {   strcpy(buffer,"GET /index.html\0");}
    //檢查客戶端所要求的檔案格式
    buflen = strlen(buffer);
    fstr = (char *)0;

    for(i=0;extensions[i].ext!=0;i++)
    {
        len = strlen(extensions[i].ext);
        if(!strncmp(&buffer[buflen-len], extensions[i].ext, len))
        {
            fstr = extensions[i].filetype;
            break;
        }
    }

    //檔案格式不支援
    if(fstr == 0)
    {   fstr = extensions[i-1].filetype;}
    //開檔
    if((file_fd=open(&buffer[5],O_RDONLY))==-1)
    {   write(fd, "Failed to open file", 19);}
    //回傳 200OK 內容格式
    sprintf(buffer,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\n\r\n", fstr);
    write(fd,buffer,strlen(buffer));
    //讀檔，輸出到客戶端
    while((ret=read(file_fd, buffer, BUFSIZE))>0)
    {   write(fd,buffer,ret);}

    exit(1);
}

void catch_child(int signum)
{
    while(waitpid(0,NULL,WNOHANG)>0);
}


int main(int argc, char **argv)
{
    struct sockaddr_in server_addr,client_addr;
    int lfd,cfd;
    int ret,sig;
    pid_t pid;
    socklen_t c_addr_size;

    lfd=socket(AF_INET,SOCK_STREAM,0);

    bzero(&server_addr,sizeof(server_addr));
    server_addr.sin_family=AF_INET;         //跟sockets()的Aparameter一樣
    server_addr.sin_port=htons(80);       //端口port：
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    bind(lfd,(struct sockaddr *)&server_addr,sizeof(server_addr));
    listen(lfd,100);


    while(1)
    {
        c_addr_size=sizeof(client_addr);
        cfd =accept(lfd,(struct sockaddr *)&client_addr,&c_addr_size);
        pid=fork();

        if(pid==0)              //son
        {   close(lfd);
            handle_socket(cfd);
            break;
        }
        else                    //father
        {
            struct sigaction act;
            act.sa_handler= catch_child;
            sigemptyset(&act.sa_mask);
            act.sa_flags=0;
            sig=sigaction(SIGCHLD,&act,NULL);

            close(cfd);
            continue;
        }

    }
    if(pid>0)wait(NULL);

    return 0;
}
