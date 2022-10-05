#include "kernel/types.h"
#include "user/user.h"
#include "stddef.h"


int main(int argc, char*argv[]){

    if(argc != 1){
        fprintf(1, "primes need no argument\n");
    }


    int pid;
    int n = -1;
    int has_right = 0;
    int * leftpipe = (int *) malloc(2*sizeof(int));                         //进程左侧的管道
    int * rightpipe = (int *) malloc(2*sizeof(int));                        //进程右侧的管道
    int num;

    pipe(rightpipe);  
    pid = fork();
    if (pid > 0){                                                           //主进程    
        //parrent
        for (int i=2; i<=35; i++){                                          //向右侧管道写入2至35的数据
            close(*rightpipe);
            write(*(rightpipe+1), &i, 1);       
        }
        int i = 0;
        write(*(rightpipe+1), &i, 1);
        close(*(rightpipe+1));
        wait(&pid);                                                         //主进程等待子进程结束
    }
    else if(pid == 0){                                                      //子进程，此处为筛选2的进程
        free(leftpipe);                                                     //释放父进程左侧的资源
        leftpipe = rightpipe;                                               //子进程左侧是父进程右侧
        close(*(rightpipe+1));                                              //进入读取循环                  筛选2的进程及以后的进程都处于该循环中         
        do{                
                   
            read(*leftpipe , &num, 1);                                      //读取左侧管道传入数据            
            if(num == 0){                                                   //若传输结束              
                if(has_right != 0){                                         //该进程右侧有进程
                    close(*rightpipe);                                      //右侧管道结束
                    write(*(rightpipe+1), &num, 1);         

                    close(*(rightpipe+1));
                    free(rightpipe);                                        //释放右侧资源
                    wait(&pid);                                             //等待右侧管道退出
                }
                free(leftpipe);                                             //释放左侧资源
                break;                                                      //跳出while循环并退出
            }
            if(n == -1){                                                    //当前管道第一次接受数据
                n = num;                                                    //当前管道筛选的是num
                printf("prime %d\n",n);
            }
            else if((num % n) != 0){                                        //余数不为0
                if(has_right == 0){                                         //右侧无管道
                    rightpipe = (int *) malloc(2*sizeof(int));              //申请右侧管道资源
                    pipe(rightpipe);
                    if((pid = fork()) > 0){                                 //创建右侧管道进程
                        has_right = 1;                                      //本进程已有右侧管道
                        close(*rightpipe);                                  //向右侧管道写入数据
                        write(*(rightpipe+1), &num, 1);                     
                    }
                    else if(pid == 0){                                      //创建的右侧管道对应进程    __
                        free(leftpipe);                                     //释放左侧资源               |
                        leftpipe = rightpipe;                               //左侧管道赋值               |--->相当于在整个while循环之前的初始化部分
                        close(*(rightpipe+1));                              //                          |
                        n = -1;                                             //筛选置为未赋值状态       __|  
                    }
                }
                else{                                                       //已有右侧管道
                    close(*rightpipe);                                      //写入数据
                    write(*(rightpipe+1), &num, 1);
                }

            }
            
        }while(num != 0);
    }
    
    exit(0);

}