#include "kernel/types.h"
#include "user/user.h"

int main(int argc,char* argv[]){
    switch (argc)
    {
    case 2:
        printf("Sleeping...(Nothing to do for a while)\n");
        sleep(atoi(argv[1]));
        printf("Awake now!(Quit)\n");
        exit();
        break;
    default:
        printf("Error! Usage:sleep <number>\n");
        exit();
        break;
    }
}