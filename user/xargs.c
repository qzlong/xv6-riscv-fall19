#include "kernel/param.h"
#include "kernel/types.h"
#include "user/user.h"
#define MAXLINE 1024

int main(int argc, char *argv[])
{
    char line[MAXLINE];
    char* params[MAXARG];  //max exec arguments：32
    int n, args_index = 0;

    char* cmd = argv[1]; //get the command
    for (int i = 1; i < argc; i++) 
        params[args_index++] = argv[i]; //get the arguments

    while ((n = read(0, line, MAXLINE)) > 0)
    {
        if (fork() == 0) // child process
        {
            char *arg = (char*) malloc(sizeof(line));
            int index = 0;
            for (int i = 0; i < n; i++)
            {
                if (line[i] == ' ' || line[i] == '\n')
                {
                    arg[index] = 0;
                    params[args_index++] = arg;
                    index = 0;
                    arg = (char*) malloc(sizeof(line));
                }
                else 
                    arg[index++] = line[i];
            }
            arg[index] = 0;
            params[args_index] = 0;
            exec(cmd, params);
        }
        else 
            wait();
    }
    exit();
}