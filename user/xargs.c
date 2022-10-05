#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"


#define MAXBUFF 50

int splite_line(char sorce[], char * buff[]);
int split_clear_sorce(char sorce[], char *target[]);

int main(int argc, char * argv[]){


    int pid;
    char buff[MAXBUFF];
    char *line[MAXBUFF];
    char *args_input[MAXBUFF];
    char *argv_new[MAXBUFF];
    int num_input, nline;

    while(1){

        if(read(0, buff, MAXBUFF) == 0){                                        //主进程结束输入则退出
            break;
        }
        nline = splite_line(buff, line);                                        //将读到的文件拆分成行，并将行的首地址存入line中，返回行总数
        for(int j=0; j<nline ; j++){
            num_input = split_clear_sorce(*(line+j), args_input);               //将读到的命令行按' '分成若干参数，返回参数总数目，并将输入缓冲区buff清空

            for(int i=1; i<argc; i++){                                          //将main函数得到的参数与缓冲区输入的参数合并到argv_new中
                *(argv_new + i - 1) = *(argv + i);                              
            }

            for(int i=argc - 1; i<num_input + argc - 1; i++){
                *(argv_new + i) = *(args_input + i - argc + 1);
            }
            *(argv_new + num_input + argc - 1) = 0;                             //添加结束符0

            if((pid = fork()) == 0){                                            //子进程执行并退出(跳出循环)
                //child
                exec(*(argv + 1), argv_new);
                break;
            }
            else if(pid < 0){
                printf("fork error");
            }
            else{                                                               //主进程等待子进程结束并继续循环
                //parent
                wait(&pid);
                for(int i=0; i<num_input; i++){
                    free(args_input[i]);
                }
            }
        }

    }

    exit(0);

}


int splite_line(char source[], char *buff[]){                           //该函数复制将读到的文件拆分成行
    buff[0] = source;
    int cnt = 0;
    int i=0;
    while(source[i] != '\0'){                                           //文件未结束
        if(source[i] == '\n'){                                          //拆分行
            cnt++;
            buff[cnt] = (source + i + 1);
        }
        i++;
    }
    return cnt;
}


int split_clear_sorce(char source[], char *target[]){                   //该函数负责将sorce按' '拆分成若干字符串，将其首地址存放在target中,并将buff清空
    int cnt = 0, i = 0, j = 0;
    char * arg = (char *) malloc(sizeof(char)*MAXBUFF);                 //申请拆分后字符串存放的空间 
    *arg = '\0';
    while (source[i] != '\n'){                                          //命令行未结束
        if(source[i] == ' '){                                           //当前为拆分符' '
            if(j != 0){                                                 //当前字符串长度不为0(防止出现连续空格)
                *(arg+j) = '\0';                                        //字符串结束
                target[cnt] = arg;                                      //保存字符串首地址
                cnt++;                                                  //参数数量+1
                j = 0;                                                  //字符串归为起始位置
                arg = (char *) malloc(sizeof(char)*MAXBUFF);            //申请下一部分空间
            }
        }
        else{                                                           //不是拆分符
            *(arg+j) = source[i];                                       //复制字符串
            j++;                                                        //拆分的字符串数组的地址+1
        }
        source[i] = '\0';                                               //将source字符串清空
        i++;                                                            //source字符串数组的地址+1
    }
    source[i] = '\0';                                                   //对source[i] = '\n'做尾处理
    if(j != 0){
        *(arg+j) = '\0';
        target[cnt] = arg;
        cnt++;
    }
    return cnt;
}