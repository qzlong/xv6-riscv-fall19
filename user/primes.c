#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#define WRITEEND 1
#define READEND 0

int redirect(int k, int pd[]) {
    //re-direct,avoid opening too much
  int fp = dup(pd[k]);
  close(pd[READEND]);
  close(pd[WRITEEND]);
  return fp;
}


int main(int argc, char *argv[]) {
    int pd[2];
    int index = 0;
    int numbers[34];
    for(;index<34;++index){
        numbers[index]=index+2;
    }
    while(index>0){
        pipe(pd);
        if(fork() > 0){
            //parent process
            int write_end = redirect(WRITEEND,pd); 
            for(int i=0;i<index;++i){
                write(write_end,&numbers[i],sizeof numbers[i]);
            }
            close(write_end);
            wait();
            exit();
        }else{
            //child process
            int read_end = redirect(READEND,pd);
            int prime;
            index = 0;
            if(read(read_end,&prime,sizeof prime)!=0){
                printf("prime %d\n",prime);
                int num;
                while(read(read_end,&num,sizeof num)!=0){
                    if(num % prime != 0){
                        numbers[index++] = num;
                    }
                }
                close(read_end);
            }
        }
    }
    exit();
}