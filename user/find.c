#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


char* fmtname(char *path);
void find(char * path, char * filename);

int main(int argc, char *argv[])
{
    if(argc != 3){
        fprintf(2, "find needs 3 arguments");
        exit(0);
    }

    find(*(argv+1), *(argv+2));

    exit(0);
}



char* fmtname(char *path)
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
  memset(buf+strlen(p), '\0', DIRSIZ-strlen(p));
  return buf;
}


void find(char * path, char * filename){
    char buf[512], *p;
    int fd;
    struct dirent de;
    struct stat st;


    if((fd = open(path, 0)) < 0){
        fprintf(2, "find: cannot open %s\n", path);
        return;
    }
    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    switch(st.type){
        case T_FILE:                                                                //当前文档类型是普通文件
            if((strcmp(fmtname(path), filename) == 0)){                             //其路径尾部即文件名与目标文件比对
                fprintf(1, "%s\n", path);                                           
            }
            break;
        case T_DIR:                                                                 //当前文档类型是路径
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("ls: path too long\n");
                break;
            }
            strcpy(buf, path);                      
            p = buf + strlen(buf);                                                  //获取路径字符串尾部地址
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){                         //读取路径文档中的文件
                if(de.inum == 0){
                    continue;
                }                                               
                else if((strcmp(de.name, ".")==0)|((strcmp(de.name, "..")==0))){    //不递归进入. 与 ..
                    continue;
                }
                memmove(p, de.name, DIRSIZ);
                p[DIRSIZ] = 0;
                find(buf, filename);                                                //子目录内查找
            }
            break;
    }
    close(fd);

}



