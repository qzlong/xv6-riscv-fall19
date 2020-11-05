#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define WRITEEND 1
#define READEND 0
typedef int pid_t;

int main(int argc,char* argv[]){
    /*intialize the pipes*/
    int parent_write_pipe[2];
    int child_write_pipe[2];
    pipe(parent_write_pipe);
    pipe(child_write_pipe);
    pid_t pid;

    if((pid=fork())<0){
        printf("Fork Error!\n");
        exit();
    }else if(pid==0){
        //Child Process
        //Child read from parent_write_pipe[0]
        //Child write into child_write_pipe[1]
        char buf[5] = "pong";
        close(parent_write_pipe[WRITEEND]);
        close(child_write_pipe[READEND]);
        write(child_write_pipe[WRITEEND],buf,sizeof buf);
        read(parent_write_pipe[READEND],buf,sizeof buf);
        printf("%d: received %s\n",getpid(),buf);
        close(child_write_pipe[WRITEEND]);
        close(parent_write_pipe[READEND]);
    }else{
        //Parent Process
        char buf[5] = "ping";
        close(parent_write_pipe[READEND]);
        close(child_write_pipe[WRITEEND]);
        write(parent_write_pipe[WRITEEND],buf,sizeof buf);
        read(child_write_pipe[READEND],buf,sizeof buf);
        printf("%d: received %s\n",getpid(),buf);
        close(parent_write_pipe[WRITEEND]);
        close(child_write_pipe[READEND]);
    }
    exit();
}