#include "kernel/types.h"
#include "user/user.h"

int main(int argc, char*argv[]){
    
    if (argc != 1){
        printf("pingpong doesn't need argument!");
    }

    int par2child[2];
    int child2par[2];
    
    int pid;


    char parent_get[10];
    char child_get[10];

    if (pipe(par2child) < 0){
        printf("Error in creating pipe");
    }
    if(pipe(child2par) < 0){
        printf("Error in creating pipe");
    }

    if ((pid = fork()) == 0){
        // child
        pid = getpid();
        close(par2child[1]);                                    //子进程读取父进程，读不到则阻塞
        read (par2child[0], child_get, 10);
        close(par2child[0]);
        printf("%d: received %s\n", pid, child_get);
        close(child2par[0]);                                    //子进程发送pong
        write(child2par[1], "pong", 5);
        close(child2par[1]);
    }
    else if(pid < 0){
        printf("Error in using fork()!");
    }

    else{
        //parent
        pid = getpid();
        close(par2child[0]);                                    //父进程发送ping
        write(par2child[1], "ping", 5);
        close(par2child[1]);
        close(child2par[1]);
        read(child2par[0], parent_get,10);                      //父进程读取子进程
        close(child2par[1]);
        printf("%d: received %s\n", pid, parent_get);
        wait(&pid);                                             //父进程等待子进程退出
    }
    exit(0);
}