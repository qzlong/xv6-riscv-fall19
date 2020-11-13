#include "kernel/types.h"
#include "user/user.h"
#include "kernel/fcntl.h"

/*  Define const values  */
#define STDERR    2
#define BUFSIZE   128
#define MAXWORD   16
#define MAXARGS   16
#define INPUTEND  0
#define OUTPUTEND 1

/*  Define functions  */
void setargs(char*,char*[],int*);
int getcmd(char*,int);
void runcmd(char*[],int);
void redirect(int*,int);


char whitespace[] = " \t\r\n\v";

int main(void){
    static char buffer[BUFSIZE];
    while(getcmd(buffer,sizeof buffer)>=0){
        if(fork()==0){
            char* argv[MAXARGS];
            memset(argv,0,sizeof argv);
            int argc = 0;
            setargs(buffer,argv,&argc);
            runcmd(argv,argc);
        }
        wait(0);
    }
    exit(0);
}
void setargs(char *cmd,char* argv[],int* argc){
  //要求每个参数得用空格隔开
  for(int j=0;cmd[j]!='\n'&& cmd[j]!='\0';j++){
    while(strchr(whitespace,cmd[j]))
      ++j;
    argv[(*argc)++] = cmd+j;
    while(!strchr(whitespace,cmd[j]))
      ++j;
    cmd[j]='\0';
  }
}
//读入一条指令
int getcmd(char* buffer,int nbuf){
    fprintf(2,"@ ");
    memset(buffer,0,nbuf);
    gets(buffer,nbuf); //最后的回车符会被丢掉
    if(buffer[0]==0)
        return -1;
    return 0;
}

//重定向
void redirect(int* p,int k){
  close(k);
  dup(p[k]);
  close(p[1]);
  close(p[0]);
}

void runcmd(char* argv[],int argc){
  for(int i=0;i<argc;i++){
    if(!strcmp(argv[i],"|")){
      //命令中有管道
      argv[i]=0;
      int p[2];
      pipe(p);

      if(fork()==0){
        redirect(p,OUTPUTEND);
        runcmd(argv,i);
      }
      if(fork()==0){
        redirect(p,INPUTEND);
        runcmd(argv+i+1,argc-i-1);
      }

      close(p[INPUTEND]);
      close(p[OUTPUTEND]);
      wait(0);
      wait(0);
      exit(0);
    }
  }

  for(int i=0;i<argc;i++){
    if(!strcmp(argv[i],">")){
      close(OUTPUTEND);
      open(argv[i+1],O_CREATE|O_WRONLY);
      argv[i]=0;
    }
    
    if(!strcmp(argv[i],"<")){
      close(INPUTEND);
      open(argv[i+1],O_RDONLY);
      argv[i]=0;
    }
  }
  exec(argv[0],argv);
  exit(0);
}