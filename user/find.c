#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

int find_flag = 0;
char* fmtname(char * path)
{
  static char buf[DIRSIZ+1];
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  // Return blank-padded name.
  if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  //memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  buf[strlen(p)]= 0;
  return buf;
}

void find(char* path, char *name)
{
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;
    
    if ((fd = open(path, 0)) < 0){
        fprintf(2, "find open %s error\n", path);
        exit();
    }

    if (fstat(fd, &st) < 0){
        fprintf(2, "fstat %s error\n", path);
        close(fd);
        exit();
    }

    switch (st.type)
    {
    case T_FILE: //file:check the file name
        if (strcmp(fmtname(path), name) == 0) {
            find_flag = 1;
            printf("%s\n", path);
        }
        break;
    
    case T_DIR: //directory: call find recursively
        if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf))
        {
            printf("find: path too long\n");
            break;
        }
        strcpy(buf, path);
        p = buf + strlen(buf);
        *p++ = '/';
        
        while (read(fd, &de, sizeof(de)) == sizeof(de))
        {
            if (de.inum == 0) 
                continue;
            // remove directory "." and ".."
            if (!strcmp(de.name, ".") || !strcmp(de.name, "..")) 
                continue;

            // add de.name to path
            memmove(p, de.name, DIRSIZ);
            p[DIRSIZ] = 0;
            // printf("%s\n",buf);
            // recursive call find
            find(buf, name);
        }
        break;
    }
    close(fd);
}


int main(int argc, char *argv[])
{
    if(argc<3){
        printf("Arguments Error! Usage:find <file path> <expression> <...>\n");
        exit();
    }else{
        for(int i=2;i<argc;i++){
            find_flag = 0;
            find(argv[1],argv[i]);
            if(!find_flag){
                printf("file %s doesn't exist\n",argv[i]);
            }
        }
    }
    exit();
}