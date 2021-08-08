// myshell.c
// 谢廷浩 3180101944
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <pwd.h>
#include <time.h>
#include <wait.h>
#include <dirent.h>

#define BUF_SIZE 256 //默认的缓冲区大小
//命令字符串宏定义
#define CMD_EXIT "exit"
#define CMD_QUIT "quit"
#define CMD_CD "cd"
#define CMD_PWD "pwd"
#define CMD_TIME "time"
#define CMD_CLR "clr"
#define CMD_LS "ls"
#define CMD_DIR "dir"
#define CMD_UMASK "umask"
#define CMD_SET "set"
#define CMD_UNSET "unset"
#define CMD_ENVIRON "environ"
#define CMD_SHIFT "shift"
#define CMD_ECHO "echo"
#define CMD_HELP "help"
#define CMD_JOBS "jobs"
#define CMD_EXEC "exec"


int splitCmd(char* cmd_input);
//将输入行中，$后直到[空格/下一个$]的内容被视为变量名
//替换所有变量为变量值，否则替换为""
//然后按空格分割用户输入的命令字符串
//返回分割后字符串个数
//将分割后的字符串写进全局变量

char* getVarValue(char* var_name_in);
//根据自定义变量/环境变量名获取变量值，返回指向变量值字符串的指针

int setVarValue(char* var_name_in, char* var_value);
//设置变量值，设定成功返回1，否则返回0

void initializeEnviron();
//初始化环境变量

int myshellExit();
//exit命令

int myshellCd();
//cd命令

int myshellPwd();
//pwd命令

int myshellTime();
//time命令

int myshellClr();
//clr命令

int myshellLs();
//ls命令

int myshellUmask();
//umask命令

int myshellAssign();
//等号赋值

int myshellSet();
//set命令

int myshellUnset();
//unset命令

int myshellShift();
//shift命令

int myshellEcho(char* cmd_input);
//echo命令

int myshellHelp();
//help命令

int myshellExec(char* cmd_input);
//exec命令

int myshellExtCmd(int cmd_num);
//外部命令，调用程序作为shell的子进程

int myshellJobs();
//jobs命令

int myshellPipeLine();
//管道


struct var //变量结构
{
    char name[BUF_SIZE];
    char value[BUF_SIZE];
    struct var* next;
};
struct var* vars; //用户自定义变量
struct var* environ; //环境变量

struct job //后台任务结构
{
    char name[BUF_SIZE];
    int pid;
};
struct job jobs[BUF_SIZE]; //记录所有后台挂起的进程
int jobs_occupied[BUF_SIZE] = {0}; //jobs_occupied[i]为1则表示jobs[i]的挂起任务

char username[BUF_SIZE]; //用户名
char hostname[BUF_SIZE]; //主机名
char cwd[BUF_SIZE]; //当前工作目录，同$pwd
char shell_wd[BUF_SIZE]; //myshell所在目录
char cmds[BUF_SIZE][BUF_SIZE]; //保存分割后的命令字符串
int cmd_num = 0; //分割后的命令字符串的个数
char arg[20][BUF_SIZE] = {0}; //$1 ~ $20
char mask[5]="0002"; //掩码

//shell命令函数返回状态值
enum {
	SUCCESS_RETURN,
	ERROR_WRONG_ARGUMENT,
	ERROR_MISS_ARGUMENT,
	ERROR_TOO_MANY_ARGUMENT,
    ERROR_FORK,
	ERROR_REDIR, //重定向错误
	ERROR_TOO_MANY_PIPE, //管道过多错误
    ERROR_PIPE_ERROR
};



int main(int argc, char** argv) 
{
    char cmd_input[BUF_SIZE];
    struct passwd* pwd = getpwuid(getuid());
	
    //初始化shell_wd
    getcwd(shell_wd, BUF_SIZE);

    //初始化username、hostname、cwd
    strcpy(username, pwd->pw_name);
    gethostname(hostname, BUF_SIZE);
    getcwd(cwd, BUF_SIZE);

    //初始化自定义变量链表
    vars = (struct var*)malloc(sizeof(struct var));
    strcpy(vars->name, "head");
    strcpy(vars->value, "HEAD_NODE");
    vars->next = NULL;

    //初始化环境变量
    initializeEnviron();

    //判断参数个数；大于1个则认为在执行myshell脚本文件；否则从命令行界面循环获得输入
    if(argc > 1)
    {
        int fd = open(argv[1] ,O_RDONLY); //打开脚本文件
        if(fd == -1)
        {
            fprintf(stderr, "Error: Script file '%s' not available!\n", argv[1]);
            exit(-1);
        }
        freopen(argv[1], "r", stdin); //重定向标准输入为myshell脚本文件
        //将后面的所有字段视为参数$1 ~ $20，存入arg[0]~arg[19]中；多余的参数忽略
        int i;
        for(i = 2; i < ((argc < 22)?argc:22); i++)
        {
            strcpy(arg[i-2], argv[i]);
        }
        for(i = 0; i < 20; i++)
        {
            printf("$%d=%s\n", i + 1, arg[i]);
        }
    }

    while(1)
    {
        if(argc == 1) printf("[%s@%s %s]$ ", username, hostname, cwd);
        fgets(cmd_input, BUF_SIZE, stdin); //从标准输入流获取一行命令
        if(feof(stdin)) break; //读到文件结束符，结束循环

        int cmd_input_len = strlen(cmd_input); //获得用户输入的字符串长度
        //if(cmd_input_len != BUF_SIZE)
            cmd_input[cmd_input_len - 1] = '\0'; //将换行符替换为'\0'
        
        splitCmd(cmd_input); //分割字符串，记录分割个数
        // int i;
        // printf("cmd_num: %d\n",cmd_num);
        // for(i = 0; i < cmd_num; i++)
        //     printf("> %d\n", cmds[i][0]);
        if(cmd_num == 0) continue; //如果没有有效输入则continue





        //命令处理模块
        int cmd_return;
        if(strcmp(cmds[0], CMD_EXIT) == 0 || strcmp(cmds[0], CMD_QUIT) == 0) //exit命令
        {
            cmd_return = myshellExit();
        }
        else if(strstr(cmd_input, "|"))
        {
            myshellPipeLine();
        }
        else if(strstr(cmd_input, "="))
        {
            cmd_return = myshellAssign();
            if(cmd_return == ERROR_TOO_MANY_ARGUMENT)
            {
                fprintf(stderr, "Error: Wrong format while assigning a variable!\n");
            }
            else if(cmd_return == ERROR_WRONG_ARGUMENT)
            {
                fprintf(stderr, "Error: Unable to alter system variables!\n");
            }
        }
        else if(strcmp(cmds[0], CMD_CD) == 0) //cd命令
        {
            cmd_return = myshellCd();
            if(cmd_return == SUCCESS_RETURN) //成功返回
            {
                getcwd(cwd, BUF_SIZE);
            }
            else if(cmd_return == ERROR_TOO_MANY_ARGUMENT) //参数太多
            {
                fprintf(stderr, "Error: Too many arguments while using command '%s'.\n", CMD_CD);
            }
            else if(cmd_return == ERROR_WRONG_ARGUMENT) //错误路径传入
            {
                fprintf(stderr, "Error: No such path '%s'!\n", cmds[1]);
            }
        }
        else if(strcmp(cmds[0], CMD_PWD) == 0) //pwd命令
        {
            cmd_return = myshellPwd();
            if(cmd_return == ERROR_TOO_MANY_ARGUMENT) //参数太多
            {
                fprintf(stderr, "Error: Too many arguments while using command '%s'.\n", CMD_PWD);
            }
        }
        else if(strcmp(cmds[0], CMD_TIME) == 0) //time命令
        {
            cmd_return = myshellTime();
            if(cmd_return == ERROR_TOO_MANY_ARGUMENT) //参数太多
            {
                fprintf(stderr, "Error: Too many arguments while using command '%s'.\n", CMD_TIME);
            }
        }
        else if(strcmp(cmds[0], CMD_CLR) == 0) //clr命令
        {
            cmd_return = myshellClr();
            if(cmd_return == ERROR_TOO_MANY_ARGUMENT) //参数太多
            {
                fprintf(stderr, "Error: Too many arguments while using command '%s'.\n", CMD_CLR);
            }
        }
        else if(strcmp(cmds[0], CMD_LS) == 0) //ls命令
        {
            cmd_return = myshellLs();
            if(cmd_return == ERROR_TOO_MANY_ARGUMENT) //参数太多
            {
                fprintf(stderr, "Error: Too many arguments while using command '%s'.\n", CMD_LS);
            }
            else if(cmd_return == ERROR_WRONG_ARGUMENT)
            {
                fprintf(stderr, "Error: Cannot open directory '%s'!\n", cmds[1]);
            }
        }
        else if(strcmp(cmds[0], CMD_DIR) == 0) //dir命令
        {
            cmd_return = myshellLs();
            if(cmd_return == ERROR_TOO_MANY_ARGUMENT) //参数太多
            {
                fprintf(stderr, "Error: Too many arguments while using command '%s'.\n", CMD_DIR);
            }
            else if(cmd_return == ERROR_WRONG_ARGUMENT)
            {
                fprintf(stderr, "Error: Cannot open directory '%s'!\n", cmds[1]);
            }
        }
        else if(strcmp(cmds[0], CMD_UMASK) == 0) //umask命令
        {
            cmd_return = myshellUmask();
            if(cmd_return == ERROR_TOO_MANY_ARGUMENT) //参数太多
            {
                fprintf(stderr, "Error: Too many arguments while using command '%s'.\n", CMD_UMASK);
            }
            else if(cmd_return == ERROR_WRONG_ARGUMENT)
            {
                fprintf(stderr, "Error: Wrong mask value '%s'!\n", cmds[1]);
            }
        }
        else if(strcmp(cmds[0], CMD_SET) == 0) //set命令
        {
            cmd_return = myshellSet();
            if(cmd_return == ERROR_TOO_MANY_ARGUMENT) //参数太多
            {
                fprintf(stderr, "Error: Too many arguments while using command '%s'.\n", CMD_SET);
            }
        }
        else if(strcmp(cmds[0], CMD_ENVIRON) == 0) //environ命令
        {
            cmd_return = myshellSet();
            if(cmd_return == ERROR_TOO_MANY_ARGUMENT) //参数太多
            {
                fprintf(stderr, "Error: Too many arguments while using command '%s'.\n", CMD_ENVIRON);
            }
        }
        else if(strcmp(cmds[0], CMD_UNSET) == 0) //unset命令
        {
            cmd_return = myshellUnset();
            if(cmd_return == ERROR_MISS_ARGUMENT) //参数太少
            {
                fprintf(stderr, "Error: Arguments missing while using command '%s'.\n", CMD_UNSET);
            }
            else if(cmd_return == ERROR_TOO_MANY_ARGUMENT) //参数太多
            {
                fprintf(stderr, "Error: Too many arguments while using command '%s'.\n", CMD_UNSET);
            }
            else if(cmd_return == ERROR_WRONG_ARGUMENT) //错误参数
            {
                fprintf(stderr, "Error: Invalid operand '%s'!\n", cmds[1]);
            }
        }
        else if(strcmp(cmds[0], CMD_SHIFT) == 0) //shift命令
        {
            cmd_return = myshellShift();
            if(cmd_return == ERROR_TOO_MANY_ARGUMENT) //参数太多
            {
                fprintf(stderr, "Error: Too many arguments while using command '%s'.\n", CMD_SHIFT);
            }
            else if(cmd_return == ERROR_WRONG_ARGUMENT) //错误参数
            {
                fprintf(stderr, "Error: Invalid argument '%s'. Numeric argument required!\n", cmds[1]);
            }
        }
        else if(strcmp(cmds[0], CMD_ECHO) == 0) //echo命令
        {
            cmd_return = myshellEcho(cmd_input);
        }
        else if(strcmp(cmds[0], CMD_HELP) == 0) //help命令
        {
            cmd_return = myshellHelp();
        }
        else if(strcmp(cmds[0], CMD_EXEC) == 0) //exec命令
        {
            cmd_return = myshellExec(cmd_input);
            if(cmd_return == ERROR_WRONG_ARGUMENT)
            {
                fprintf(stderr, "Error: Command '%s' not found!\n", cmds[1]);
            }
            else if(cmd_return == ERROR_MISS_ARGUMENT)
            {
                fprintf(stderr, "Error: Arguments missing while using command '%s'.\n", CMD_EXEC);
            }
        }
        else if(strcmp(cmds[0], CMD_JOBS) == 0) //jobs命令
        {
            cmd_return = myshellJobs();
            if(cmd_return == ERROR_TOO_MANY_ARGUMENT)
            {
                fprintf(stderr, "Error: Too many arguments while using command '%s'.\n", CMD_JOBS);
            }
        }
        else //否则执行该外部程序
        {
            myshellExtCmd(cmd_num);
        }

    }
    return 0;
}

int splitCmd(char* cmd_input)
{
    int cmd_input_len = strlen(cmd_input);
    int num = 0, tmp = 0;
    int i;

    //替换$开头的变量
    if(strstr(cmd_input, "$"))
    {
        for(i = 0; i < cmd_input_len; i++)
        {
            if(cmd_input[i] == '$') //读到了$
            {
                int start = i, end;
                i++;
                char var_name[BUF_SIZE] = {0};
                int cnt = 0;
                while(cmd_input[i] != ' ' && cmd_input[i] != '$' && i < cmd_input_len) //直到读到空格或者下一个$
                {
                    var_name[cnt++] = cmd_input[i++];
                }
                                
                char var_value[BUF_SIZE] = {0}; 
                strcpy(var_value, getVarValue(var_name));
                if(var_value[0] == '\0') //如果变量为空
                {
                    end = start + cnt;
                    strcpy(cmd_input + start, cmd_input + end + 1); //将后面的部分拼接回来
                    cmd_input_len = strlen(cmd_input);
                    i = start;
                    continue;
                }
                //否则变量不为空
                end = start + cnt;
                char tmp_buffer[BUF_SIZE];
                strcpy(tmp_buffer, cmd_input + end + 1); //将后面的部分暂存到tmp_buffer中
                strcpy(cmd_input + start, var_value); //将这个$开头的变量替换为变量值
                strcat(cmd_input, tmp_buffer); //将后面的部分拼接回来
                cmd_input_len = strlen(cmd_input);
                i = start;
            }
        }
    }

    cmd_input_len = strlen(cmd_input);
    //printf("[debug] After variable substitution\n> %s\n", cmd_input);
    
    //读取输入字符串，保存到cmds全局变量中
    for(i = 0; i < cmd_input_len; i++)
    {
        if(cmd_input[i] != ' ')
        {
            cmds[num][tmp++] = cmd_input[i];
            //如果最后一个字符不是空格
            if(i == cmd_input_len - 1)
            {
                cmds[num][tmp] = '\0';
                num++;
            }
        }
        else
        {
            //否则读到了空格
            //如果第一个字符是空格
            if(i == 0)
            {
                while(cmd_input[++i] == ' ');
                i--;
                continue;
            }
            //如果最后一个字符是空格
            if(i == cmd_input_len - 1)
            {
                num++;
                break;
            }
            if(cmd_input[i + 1] == ' ') continue; //如果下一个还是空格就continue

            //读到连续空格段的最后一个空格，为刚才写的cmd[]添加结束符，更新tmp、num
            cmds[num][tmp] = '\0';
            tmp = 0;
            num++;
            
        }
    }
    cmd_num = num; //修改全局变量
    return num;
}

int myshellExit()
{
    pid_t pid = getpid();
    if(kill(pid, SIGTERM) == -1)
        exit(-1); //没有成功kill则执行exit(-1)
    return SUCCESS_RETURN; //正常kill进程，显示Terminated
}

int myshellCd()
{
    if(cmd_num == 1)
    {
        if(username == "root")
        {
            int cd_success_or_not = chdir("/root"); //更改工作目录
            if(cd_success_or_not)
                return ERROR_WRONG_ARGUMENT;
        }
        else
        {
            char tmp[BUF_SIZE];
            sprintf(tmp, "/home/%s", username);
            int cd_success_or_not = chdir(tmp); //更改工作目录
            if(cd_success_or_not)
                return ERROR_WRONG_ARGUMENT;
        }
    }
	else if(cmd_num > 2)
		return ERROR_TOO_MANY_ARGUMENT;
    else
    {
		int cd_success_or_not = chdir(cmds[1]); //更改工作目录
		if(cd_success_or_not)
            return ERROR_WRONG_ARGUMENT;
    }
	return SUCCESS_RETURN;
}

int myshellPwd()
{
    if(cmd_num == 1)
    {
        printf("%s\n", cwd);
    }
	else
		return ERROR_TOO_MANY_ARGUMENT;
	return SUCCESS_RETURN;
}

int myshellTime()
{
    time_t time_ = time(NULL); //当前时间
    struct tm *time_stuct = localtime(&time_);
    if(cmd_num > 1) return ERROR_TOO_MANY_ARGUMENT;
    switch(time_stuct->tm_wday) //输出星期几
    {
        case 0:
            printf("Sun ");
            break;
        case 1:
            printf("Mon ");
            break;
        case 2:
            printf("Tue ");
            break;
        case 3:
            printf("Wed ");
            break;
        case 4:
            printf("Thu ");
            break;
        case 5:
            printf("Fri ");
            break;
        case 6:
            printf("Sat ");
            break;
    }

    switch(time_stuct->tm_mon) //输出月份
    {
        case 0: 
            printf("Jan ");
            break;
        case 1: 
            printf("Feb ");
            break;
        case 2: 
            printf("Mar ");
            break;
        case 3: 
            printf("Apr ");
            break;
        case 4: 
            printf("May ");
            break;
        case 5: 
            printf("Jun ");
            break;
        case 6: 
            printf("Jul ");
            break;
        case 7: 
            printf("Aug ");
            break;
        case 8: 
            printf("Sep ");
            break;
        case 9: 
            printf("Oct ");
            break;
        case 10: 
            printf("Nov ");
            break;
        case 11: 
            printf("Dec ");
            break;
    }
    
    printf("%02d %02d:%02d:%02d %s %d\n", time_stuct->tm_mday, time_stuct->tm_hour,\
     time_stuct->tm_min, time_stuct->tm_sec, time_stuct->tm_zone, time_stuct->tm_year + 1900); 
     //按照date命令格式输出时间
    return SUCCESS_RETURN;
}

int myshellClr()
{
    if(cmd_num > 1) return ERROR_TOO_MANY_ARGUMENT;
    int pid = fork();
    if(pid < 0)
    {
        fprintf(stderr, "Error: myshellClr()::fork() failed!\n");
        return ERROR_FORK;
    }
    else if(pid == 0)
        execl("/usr/bin/clear", "clear", NULL); //子进程调用/usr/bin/clear程序
    else
        wait(NULL); //父进程等待子进程
    return SUCCESS_RETURN;
}

int myshellLs()
{
    DIR *dp;
    struct dirent *entry;
    //struct stat statbuf;
    char dir[BUF_SIZE]; //目录路径字符串

    int redir_flag_in = 0, redir_flag_out = 0;
    char in[BUF_SIZE], out[BUF_SIZE]; //重定向的文件名
    int cat_flag = 0; //覆盖还是添加
    int i;
    int num_in = 0, num_out = 0; //计数器
    int new_cmd_num = BUF_SIZE + 1; //除去重定向部分，新的命令字段数目
    
    if(cmd_num > 2) //多于2个参数
    {
        //判定是否包含重定向
        for(i = 0; i < cmd_num; i++)
        {
            if(num_in == 1 && num_out == 1)
            {
                break; //读到一个重定向
            }
            if(strcmp(cmds[i], "<") == 0)
            {
                if(new_cmd_num > i) new_cmd_num = i; //更新有效的命令数目
                num_in++;
                if(i == cmd_num - 1 || num_in > 1) //不允许的情形
                {
                    fprintf(stderr, "Error: Syntax error using redirection.\n");
                    return ERROR_REDIR;
                }
                strcpy(in, cmds[++i]);
            }
            if(strcmp(cmds[i], ">") == 0 || strcmp(cmds[i], ">>") == 0)
            {
                if(new_cmd_num > i) new_cmd_num = i; //更新有效的命令数目
                if(strcmp(cmds[i], ">>") == 0) cat_flag = 1; //添加到文件尾，而非覆盖
                num_out++;
                if(i == cmd_num - 1 || num_out > 1) //不允许的情形
                {
                    fprintf(stderr, "Error: Syntax error using redirection.\n");
                    return ERROR_REDIR;
                }
                strcpy(out, cmds[++i]);
            }
        }

        redir_flag_in = num_in;
        redir_flag_out = num_out;
        if((redir_flag_in == 0 && redir_flag_out == 0) || new_cmd_num > 2) //没有重定向，或者仍有多余字段，则返回
            return ERROR_TOO_MANY_ARGUMENT;
        if(new_cmd_num <= BUF_SIZE) cmd_num = new_cmd_num; //更新cmd字段数目
    }
    //如果需要重定向
    if(redir_flag_in)
    {
        freopen(in, "r", stdin); //重定向标准输入
    }
    if(redir_flag_out)
    {
        if(cat_flag) freopen(out, "a", stdout); //重定向标准输出-追加
        else freopen(out, "w", stdout); //重定向标准输出-覆盖
    }


    
    if(cmd_num == 1) //没有路径参数默认为当前工作路径
        strcpy(dir, cwd);
    else if(strcmp(cmds[1], "~") == 0) //~符号替换为用户主目录
        sprintf(dir, "/home/%s", username);
    else
        strcpy(dir, cmds[1]);
    
    if((dp = opendir(dir)) == NULL) //目录无法打开
        return ERROR_WRONG_ARGUMENT;

    chdir(dir); //变换工作目录
    if((entry = readdir(dp)) != NULL) //获得目录下的第一个文件
    {
        //lstat(entry->d_name, &statbuf); //获得文件的stat结构
        printf("%s", entry->d_name);//打印文件名
    }
    while((entry = readdir(dp)) != NULL) //遍历目录下的其他文件
    {
        //lstat(entry->d_name, &statbuf); //获得文件的stat结构
        printf(" %s", entry->d_name); //打印文件名
    }
    printf("\n");
    chdir(cwd); //回到原来的工作目录
    closedir(dp); //关闭目录

    //恢复重定向
    if(redir_flag_in)
    {
        freopen("/dev/tty", "r", stdin);
    }
    if(redir_flag_out)
    {
        freopen("/dev/tty", "w", stdout);
    }

    return SUCCESS_RETURN;
}

int myshellUmask()
{
    if(cmd_num > 2) return ERROR_TOO_MANY_ARGUMENT;
    else if(cmd_num == 1)
    {
        printf("%s\n", mask);
    }
    else if(strlen(cmds[1]) == 3)
    {
        if(cmds[1][0] >= '0' && cmds[1][0] <= '7' && cmds[1][1] >= '0' && cmds[1][1] <= '7' && cmds[1][2] >= '0' && cmds[1][2] <= '7')
            strcpy(mask, cmds[1]);
        else
            return ERROR_WRONG_ARGUMENT;
    }
    else if(strlen(cmds[1]) == 4)
    {
        if(cmds[1][0] >= '0' && cmds[1][0] <= '7' && cmds[1][1] >= '0' && cmds[1][1] <= '7' && cmds[1][2] >= '0' && cmds[1][2] <= '7' && cmds[1][3] >= '0' && cmds[1][3] <= '7')
            strcpy(mask, cmds[1]);
        else
            return ERROR_WRONG_ARGUMENT;
    }
    return SUCCESS_RETURN;
}

int myshellAssign()
{
    if(cmd_num > 1) return ERROR_TOO_MANY_ARGUMENT;
    char* equal_pos = strstr(cmds[0], "=");
    int var_name_len = equal_pos - cmds[0];
    char var_name[BUF_SIZE] = {0};
    char var_value[BUF_SIZE] = {0};

    strncpy(var_name, cmds[0], var_name_len);
    strcpy(var_value, equal_pos + 1);

    //printf("Assigning value '%s' -> variable $%s...\n", var_value, var_name);
    if(!setVarValue(var_name, var_value))
    {
        return ERROR_WRONG_ARGUMENT;
    }
    //printf("variable $%s = %s\n", var_name, getVarValue(var_name));

    return SUCCESS_RETURN;
}

int myshellSet()
{
    int redir_flag_in = 0, redir_flag_out = 0;
    char in[BUF_SIZE], out[BUF_SIZE]; //重定向的文件名
    int cat_flag = 0; //覆盖还是添加
    int i;
    int num_in = 0, num_out = 0; //计数器
    int new_cmd_num = BUF_SIZE + 1; //除去重定向部分，新的命令字段数目

    for(i = 0; i < cmd_num; i++)
    {
        if(num_in == 1 && num_out == 1)
        {
            break; //读到一个重定向
        }
        if(strcmp(cmds[i], "<") == 0)
        {
            if(new_cmd_num > i) new_cmd_num = i; //更新有效的命令数目
            num_in++;
            if(i == cmd_num - 1 || num_in > 1) //不允许的情形
            {
                fprintf(stderr, "Error: Syntax error using redirection.\n");
                return ERROR_REDIR;
            }
            strcpy(in, cmds[++i]);
        }
        if(strcmp(cmds[i], ">") == 0 || strcmp(cmds[i], ">>") == 0)
        {
            if(new_cmd_num > i) new_cmd_num = i; //更新有效的命令数目
            if(strcmp(cmds[i], ">>") == 0) cat_flag = 1; //添加到文件尾，而非覆盖
            num_out++;
            if(i == cmd_num - 1 || num_out > 1) //不允许的情形
            {
                fprintf(stderr, "Error: Syntax error using redirection.\n");
                return ERROR_REDIR;
            }
            strcpy(out, cmds[++i]);
        }

        redir_flag_in = num_in;
        redir_flag_out = num_out;
        if(new_cmd_num <= BUF_SIZE) cmd_num = new_cmd_num; //更新cmd字段数目
    }
    //如果需要重定向
    if(redir_flag_in)
    {
        freopen(in, "r", stdin); //重定向标准输入
    }
    if(redir_flag_out)
    {
        if(cat_flag) freopen(out, "a", stdout); //重定向标准输出-追加
        else freopen(out, "w", stdout); //重定向标准输出-覆盖
    }



    if(cmd_num > 1) return ERROR_TOO_MANY_ARGUMENT;

    //遍历链表，打印所有环境变量
    struct var* tmp = environ;
    while(tmp != NULL)
    {
        printf("%s=%s\n", tmp->name, tmp->value);
        tmp = tmp->next;
    }

    //恢复重定向
    if(redir_flag_in)
    {
        freopen("/dev/tty", "r", stdin);
    }
    if(redir_flag_out)
    {
        freopen("/dev/tty", "w", stdout);
    }

    return SUCCESS_RETURN;
}

int myshellUnset()
{
    if(cmd_num > 2) return ERROR_TOO_MANY_ARGUMENT;
    else if(cmd_num <= 1) return ERROR_MISS_ARGUMENT;
    
    char var_name[BUF_SIZE];
    if(cmds[1][0]=='$') return ERROR_WRONG_ARGUMENT;
    strcpy(var_name, cmds[1]);

    //判断是否为$0~$20（判定是否为数字，即不允许使用纯数字作为变量名）
    if(var_name[0] == '0' && var_name[1] == 0) // $0
    {
        return ERROR_WRONG_ARGUMENT;
    }
    else if(atoi(var_name) != 0)
    {
        return ERROR_WRONG_ARGUMENT;
    }

    struct var* tmp = environ;
    //如果是环境变量
    while(tmp != NULL && strcmp(tmp->name, var_name) != 0) tmp = tmp->next; //找到该环境变量节点
    if(tmp != NULL) //确实存在该节点
    {
        //删除节点
        if(strcmp(var_name, "shell") == 0)
        {
            printf("Unable to unset system environment variable 'shell'!\n");
            return SUCCESS_RETURN;
        }
        //找到被删除节点的前一个节点
        tmp = environ;
        while(tmp->next != NULL && strcmp(tmp->next->name, var_name) != 0) tmp = tmp->next;
        //移除该节点并释放空间
        struct var* del_node = tmp->next;
        tmp->next = tmp->next->next;
        free(del_node);
        return SUCCESS_RETURN;
    }
    //如果是自定义变量
    tmp = vars;
    while(tmp != NULL && strcmp(tmp->name, var_name) != 0) tmp = tmp->next; //找到该自定义变量节点
    if(tmp != NULL) //确实存在该节点
    {
        //删除节点
        if(strcmp(var_name, "head") == 0)
        {
            printf("Unable to unset system variable 'head'!\n");
            return SUCCESS_RETURN;
        }
        //找到被删除节点的前一个节点
        tmp = vars;
        while(tmp->next != NULL && strcmp(tmp->next->name, var_name) != 0) tmp = tmp->next;
        //移除该节点并释放空间
        struct var* del_node = tmp->next;
        tmp->next = tmp->next->next;
        free(del_node);
        return SUCCESS_RETURN;
    }
}

int myshellShift()
{
    if(cmd_num > 2) return ERROR_TOO_MANY_ARGUMENT;
    int shift_num = 1; //默认移动一位
    if(cmd_num == 2)
    {
        if(cmds[1][0] == '0' && cmds[1] == 0) // 0
        {
            shift_num = 0;
        }
        else if(atoi(cmds[1]) > 0) //数字
        {
            int num = atoi(cmds[1]);
            shift_num = (num >= 20)?20:num; //大于20位则只移20位
        }
        else //不是数字
        {
            return ERROR_WRONG_ARGUMENT;
        }
    }
    
    //移动arg[]
    int i;
    for(i = 0; i + shift_num < 20; i++)
    {
        strcpy(arg[i], arg[i + shift_num]);
    }
    for(; i < 20; i++)
    {
        arg[i][0] = '\0'; //移动过的参数全部设为空字符串
    }
    
    for(i = 0; i < 20; i++)
    {
        printf("$%d=%s\n", i + 1, arg[i]);
    }

    return SUCCESS_RETURN;
}

int myshellEcho(char* cmd_input)
{
    int redir_flag_in = 0, redir_flag_out = 0;
    char in[BUF_SIZE], out[BUF_SIZE]; //重定向的文件名
    int cat_flag = 0; //覆盖还是添加
    int i;
    int num_in = 0, num_out = 0; //计数器
    int new_cmd_num = BUF_SIZE + 1; //除去重定向部分，新的命令字段数目

    for(i = 0; i < cmd_num; i++)
    {
        if(num_in == 1 && num_out == 1)
        {
            break; //读到一个重定向
        }
        if(strcmp(cmds[i], "<") == 0)
        {
            if(new_cmd_num > i) new_cmd_num = i; //更新有效的命令数目
            num_in++;
            if(i == cmd_num - 1 || num_in > 1) //不允许的情形
            {
                fprintf(stderr, "Error: Syntax error using redirection.\n");
                return ERROR_REDIR;
            }
            strcpy(in, cmds[++i]);
        }
        if(strcmp(cmds[i], ">") == 0 || strcmp(cmds[i], ">>") == 0)
        {
            if(new_cmd_num > i) new_cmd_num = i; //更新有效的命令数目
            if(strcmp(cmds[i], ">>") == 0) cat_flag = 1; //添加到文件尾，而非覆盖
            num_out++;
            if(i == cmd_num - 1 || num_out > 1) //不允许的情形
            {
                fprintf(stderr, "Error: Syntax error using redirection.\n");
                return ERROR_REDIR;
            }
            strcpy(out, cmds[++i]);
        }

        redir_flag_in = num_in;
        redir_flag_out = num_out;
        if(new_cmd_num <= BUF_SIZE) //更新不包含重定向部分的输入命令字符串
        {
            char* pos = strstr(cmd_input, cmds[new_cmd_num]);
            pos[0] = '\0'; //添加结束符
        }
    }

    //如果需要重定向
    if(redir_flag_in)
    {
        freopen(in, "r", stdin); //重定向标准输入
    }
    if(redir_flag_out)
    {
        if(cat_flag) freopen(out, "a", stdout); //重定向标准输出-追加
        else freopen(out, "w", stdout); //重定向标准输出-覆盖
    }

    if(cmd_num == 1)
    {
        printf("\n");
        return SUCCESS_RETURN;
    }
    char* pos = strstr(cmd_input, cmds[1]); //找到第一个参数的起始位置
    
    printf("%s\n", pos); //从echo后第一个非空格位置输出所有字符串（已经替换了变量）

    //恢复重定向
    if(redir_flag_in)
    {
        freopen("/dev/tty", "r", stdin);
    }
    if(redir_flag_out)
    {
        freopen("/dev/tty", "w", stdout);
    }

    return SUCCESS_RETURN;
}

int myshellHelp()
{
    int redir_flag_in = 0, redir_flag_out = 0;
    char in[BUF_SIZE], out[BUF_SIZE]; //重定向的文件名
    int cat_flag = 0; //覆盖还是添加
    int i;
    int num_in = 0, num_out = 0; //计数器
    int new_cmd_num = BUF_SIZE + 1; //除去重定向部分，新的命令字段数目

    for(i = 0; i < cmd_num; i++)
    {
        if(num_in == 1 && num_out == 1)
        {
            break; //读到一个重定向
        }
        if(strcmp(cmds[i], "<") == 0)
        {
            if(new_cmd_num > i) new_cmd_num = i; //更新有效的命令数目
            num_in++;
            if(i == cmd_num - 1 || num_in > 1) //不允许的情形
            {
                fprintf(stderr, "Error: Syntax error using redirection.\n");
                return ERROR_REDIR;
            }
            strcpy(in, cmds[++i]);
        }
        if(strcmp(cmds[i], ">") == 0 || strcmp(cmds[i], ">>") == 0)
        {
            if(new_cmd_num > i) new_cmd_num = i; //更新有效的命令数目
            if(strcmp(cmds[i], ">>") == 0) cat_flag = 1; //添加到文件尾，而非覆盖
            num_out++;
            if(i == cmd_num - 1 || num_out > 1) //不允许的情形
            {
                fprintf(stderr, "Error: Syntax error using redirection.\n");
                return ERROR_REDIR;
            }
            strcpy(out, cmds[++i]);
        }

        redir_flag_in = num_in;
        redir_flag_out = num_out;
        if(new_cmd_num <= BUF_SIZE) cmd_num = new_cmd_num; //更新cmd字段数目
    }
    //如果需要重定向
    if(redir_flag_in)
    {
        freopen(in, "r", stdin); //重定向标准输入
    }
    if(redir_flag_out)
    {
        if(cat_flag) freopen(out, "a", stdout); //重定向标准输出-追加
        else freopen(out, "w", stdout); //重定向标准输出-覆盖
    }

    if(cmd_num == 1 || cmd_num > 2)
    {
        if(cmd_num > 2) fprintf(stderr, "Error: Too many arguments while using command 'help'.\n");
        FILE *fp = fopen("./help/help.txt", "r");
        char ch;
        while((ch = fgetc(fp)) != EOF) putchar(ch);
        printf("\n");

        //恢复重定向
        if(redir_flag_in)
        {
            freopen("/dev/tty", "r", stdin);
        }
        if(redir_flag_out)
        {
            freopen("/dev/tty", "w", stdout);
        }
        if(cmd_num > 2)
            return ERROR_TOO_MANY_ARGUMENT;
        else
            return SUCCESS_RETURN;
    }
    
    char filepath[BUF_SIZE] = {0};
    strcpy(filepath, "./help/help_");
    strcat(filepath, cmds[1]);
    strcat(filepath, ".txt");
    FILE *fp = fopen(filepath, "r");
    if(fp == NULL)
    {
        fprintf(stderr, "Error: no help topics match '%s'.\n", cmds[1]);
        FILE *fp = fopen("./help/help.txt", "r");
        char ch;
        while((ch = fgetc(fp)) != EOF) putchar(ch);
        printf("\n");
        //恢复重定向
        if(redir_flag_in)
        {
            freopen("/dev/tty", "r", stdin);
        }
        if(redir_flag_out)
        {
            freopen("/dev/tty", "w", stdout);
        }
        return ERROR_WRONG_ARGUMENT;
    }
    char ch;
    while((ch = fgetc(fp)) != EOF) putchar(ch);
    printf("\n");

    //恢复重定向
    if(redir_flag_in)
    {
        freopen("/dev/tty", "r", stdin);
    }
    if(redir_flag_out)
    {
        freopen("/dev/tty", "w", stdout);
    }
    
    return SUCCESS_RETURN;
}

int myshellExec(char* cmd_input)
{
    if(cmd_num < 2) return ERROR_MISS_ARGUMENT;
    char* pos = strstr(cmd_input, cmds[1]); //找到第一个参数的起始位置

    //将命令行参数存入argv
    int i;
    char* argv[BUF_SIZE];
    for(i = 1; i < cmd_num; i++)
    {
        argv[i - 1] = cmds[i];
    }
    argv[i - 1] = NULL;

    //设定环境变量
    char parent[BUF_SIZE];
    sprintf(parent, "%s/myshell", shell_wd);
    char* envp[] = {parent, NULL};

    //通过execve执行程序
    if(execve(cmds[1], argv, envp) == -1)
    {
        chdir("/bin/");
        if(execve(cmds[1], argv, envp) == -1)
            return ERROR_WRONG_ARGUMENT;
    }
    return SUCCESS_RETURN;
}

int myshellExtCmd(int cmd_num)
{
    int silent = 0;
    //判定命令结尾是否为&（最后一个字段为&）
    if(cmds[cmd_num - 1][0] == '&' && cmds[cmd_num - 1][1] == '\0')
        silent = 1;
    if(silent && cmd_num == 1) //不允许命令仅包含一个&
    {
        fprintf(stderr, "Error: Syntax error near '&'.\n");
        return ERROR_WRONG_ARGUMENT;
    }

    //判定命令是否包含< > >>3种重定向标志
    //重定向标志必须由空格分隔单独成一个字段
    int redir_flag_in = 0, redir_flag_out = 0;
    char in[BUF_SIZE], out[BUF_SIZE]; //重定向的文件名
    int cat_flag = 0; //覆盖还是添加
    if(!silent) //不允许在后台执行时重定向
    {
        int i;
        int num_in = 0, num_out = 0; //计数器
        int new_cmd_num = BUF_SIZE + 1; //除去重定向部分，新的命令字段数目

        for(i = 0; i < cmd_num; i++)
        {
            if(num_in == 1 && num_out == 1)
            {
                break; //读到一个重定向
            }
            if(strcmp(cmds[i], "<") == 0)
            {
                if(new_cmd_num > i) new_cmd_num = i; //更新有效的命令数目
                num_in++;
                if(i == cmd_num - 1 || num_in > 1) //不允许的情形
                {
                    fprintf(stderr, "Error: Syntax error using redirection.\n");
                    return ERROR_REDIR;
                }
                strcpy(in, cmds[++i]);
            }
            if(strcmp(cmds[i], ">") == 0 || strcmp(cmds[i], ">>") == 0)
            {
                if(new_cmd_num > i) new_cmd_num = i; //更新有效的命令数目
                if(strcmp(cmds[i], ">>") == 0) cat_flag = 1; //添加到文件尾，而非覆盖
                num_out++;
                if(i == cmd_num - 1 || num_out > 1) //不允许的情形
                {
                    fprintf(stderr, "Error: Syntax error using redirection.\n");
                    return ERROR_REDIR;
                }
                strcpy(out, cmds[++i]);
            }
        }

        redir_flag_in = num_in;
        redir_flag_out = num_out;
        if(new_cmd_num <= BUF_SIZE) cmd_num = new_cmd_num; //更新cmd字段数目
    }
    

    int pid = fork();
    if(pid < 0)
    {
        fprintf(stderr, "Error: myshellExtCmd()::fork() failed!\n");
        return ERROR_FORK;
    }
    else if(pid > 0) //父进程
    {
        if(!silent) //正常运行
        {
            wait(NULL); //父进程等待子进程
            return SUCCESS_RETURN;
        }    
        else //后台运行
        {
            printf("PID %d\n", pid); //输出PID

            
            //记录到后台任务列表中，以便通过jobs命令查看
            int job_index, i;
            i = BUF_SIZE - 1;
            while(i >= 0 && jobs_occupied[i] == 0) i--; //找到最大的后台任务index
            job_index = i + 1; //可以放置任务的空闲位置
            if(job_index >= BUF_SIZE)
            {
                fprintf(stderr, "Error: Too many background process!\n");
                return ERROR_WRONG_ARGUMENT;
            }

            //记录任务
            jobs_occupied[job_index] = 1;
            memset(jobs[job_index].name, 0, BUF_SIZE);
            //记录任务命令
            for(i = 0; i < cmd_num - 1; i++)
            {
                strcat(jobs[job_index].name, cmds[i]);
                strcat(jobs[job_index].name, " ");
            }
            jobs[job_index].pid = pid;

            return SUCCESS_RETURN;
        }
    }
    else //子进程
    {
        int i;
        char* argv[BUF_SIZE];
        if(silent) //后台运行
        {
            freopen("/dev/null", "w", stdout); //重定向标准输出
            freopen("/dev/null", "w", stderr); //重定向错误输出

            //将命令行参数存入argv
            for(i = 0; i < cmd_num - 1; i++)
            {
                argv[i] = cmds[i];
            }
            argv[i] = NULL;
        }
        else //正常执行
        {
            //如果需要重定向
            if(redir_flag_in)
            {
                freopen(in, "r", stdin); //重定向标准输入
            }
            if(redir_flag_out)
            {
                if(cat_flag) freopen(out, "a", stdout); //重定向标准输出-追加
                else freopen(out, "w", stdout); //重定向标准输出-覆盖
            }
            //将命令行参数存入argv
            for(i = 0; i < cmd_num; i++)
            {
                argv[i] = cmds[i];
            }
            argv[i] = NULL;
        }
        
        //设定环境变量
        char parent[BUF_SIZE];
        sprintf(parent, "%s/myshell", shell_wd);
        char* envp[] = {parent, NULL};

        //通过execve执行程序
        if(execve(cmds[0], argv, envp) == -1)
        {
            chdir("/bin/");
            if(execve(cmds[0], argv, envp) == -1)
                fprintf(stderr, "Error: Command '%s' not found!\n", cmds[0]);
            exit(-1);
        }
        exit(0);
    }
    return SUCCESS_RETURN;
}

int myshellJobs()
{
    if(cmd_num > 1) return ERROR_TOO_MANY_ARGUMENT;
    int i;
    printf("NUMBER\t\tPID\t\tCOMMAND\n");
    for(i = 0; i < BUF_SIZE; i++)
    {
        if(jobs_occupied[i])
        {
            printf("[%d]\t\t%d\t\t%s\n", i, jobs[i].pid, jobs[i].name);
        }
    }
    return SUCCESS_RETURN;
}

int myshellPipeLine()
{
    int i, pipe_num = 0;
    int pipe_pos;
    for(i = 0; i < cmd_num; i++)
    {
        if(strcmp(cmds[i], "|") == 0)
        {
            pipe_pos = i; //管道位置
            pipe_num++; //管道数目增加
            if(pipe_num > 1 || i == cmd_num - 1) //有多于1个管道 / 管道后没有内容
                return ERROR_TOO_MANY_PIPE;
        }
    }
    //printf("pipe_pos = %d\n", pipe_pos);
    int fd[2];
    if(pipe(fd) == -1)
    {
        return ERROR_PIPE_ERROR;
    }
    int pid1 = fork();
    if(pid1 < 0)
    {
        fprintf(stderr, "Error: myshellPipeLine()::fork() failed!\n");
        return ERROR_FORK;
    }
    if(pid1 == 0) //子进程
    {        
        int i;
        char* argv[BUF_SIZE];
        //将管道前半段的命令行参数存入argv
        for(i = 0; i < pipe_pos; i++)
        {
            argv[i] = cmds[i];
            //printf("Son:argv[%d]=%s\n", i, argv[i]);
        }
        argv[i] = NULL;
        //设定环境变量
        char parent[BUF_SIZE];
        sprintf(parent, "%s/myshell", shell_wd);
        char* envp[] = {parent, NULL};

        close(1);
        dup2(fd[1], 1); //将stdout挂至管道写入端
        close(fd[0]);
        //通过execve执行程序
        if(execve(cmds[0], argv, envp) == -1)
        {
            chdir("/bin/");
                if(execve(cmds[0], argv, envp) == -1)
            fprintf(stderr, "Error: Command '%s' not found!\n", cmds[0]);
            exit(-1);
        }
    }
    //父进程
    //printf("I am the father\n");
    //signal(SIGCHLD, SIG_IGN);
    int pid2 = fork();
    if(pid2 < 0)
    {
        fprintf(stderr, "Error: myshellPipeLine()::fork() failed!\n");
        return ERROR_FORK;
    }
    if(pid2 > 0)
    {
        //printf("pid2: %d\n", pid2);
        //int status;
        //waitpid(pid, &status, 0);
        wait(NULL);
        return SUCCESS_RETURN;
    }
    else
    {
        close(0);
        close(fd[1]);
        dup2(fd[0], 0); //将标准输入流挂至读入端

        int i;
        char* argv[BUF_SIZE];
        //将管道后半段的命令行参数存入argv
        for(i = pipe_pos + 1; i < cmd_num; i++)
        {
            argv[i - pipe_pos - 1] = cmds[i];
            //printf("Father:argv[%d]=%s\n", i - pipe_pos - 1, argv[i - pipe_pos - 1]);
        }
        argv[i - pipe_pos - 1] = NULL;
        //设定环境变量
        char parent[BUF_SIZE];
        sprintf(parent, "%s/myshell", shell_wd);
        char* envp[] = {parent, NULL};
        
        //通过execve执行程序
        if(execve(cmds[pipe_pos + 1], argv, NULL) == -1)
        {
            chdir("/bin/");
            if(execve(cmds[pipe_pos + 1], argv, envp) == -1)
                fprintf(stderr, "Error: Command '%s' not found!\n", cmds[pipe_pos + 1]);
            exit(-1);
        }
    }
    return SUCCESS_RETURN;
}




char* getVarValue(char* var_name_in)
{
    char var_name[BUF_SIZE];
    if(var_name_in[0] == '$')
    {
        strcpy(var_name, var_name_in + 1);
    }
    else
    {
        strcpy(var_name, var_name_in);
    }

    //判断是否为$0~$20
    if(var_name[0] == '0' && var_name[1] == 0) // $0
    {
        return "-myshell";
    }
    else if(atoi(var_name) > 0 && atoi(var_name) <= 20)
    {
        return arg[atoi(var_name)];
    }

    //如果是环境变量
    struct var* tmp = environ;
    while(tmp != NULL && strcmp(tmp->name, var_name) != 0) tmp = tmp->next; //找到符合要求的环境变量节点
    if(tmp != NULL) //找到
        return tmp->value;

    //否则是自定义变量
    tmp = vars;
    while(tmp != NULL && strcmp(tmp->name, var_name) != 0) tmp = tmp->next; //找到符合要求的自定义变量节点
    if(tmp != NULL) //找到
        return tmp->value;

    return "";
}

int setVarValue(char* var_name_in, char* var_value)
{
    char var_name[BUF_SIZE];
    if(var_name_in[0] == '$')
    {
        strcpy(var_name, var_name_in + 1);
    }
    else
    {
        strcpy(var_name, var_name_in);
    }

    //判断是否为$0~$20（判定是否为数字，即不允许使用纯数字作为变量名）
    if(var_name[0] == '0' && var_name[1] == 0) // $0
    {
        return 0;
    }
    else if(atoi(var_name) != 0)
    {
        return 0;
    }

    struct var* tmp = environ;
    //如果是环境变量
    while(tmp != NULL && strcmp(tmp->name, var_name) != 0) tmp = tmp->next; //找到该环境变量节点
    if(tmp != NULL) //确实存在该节点
    {
        strcpy(tmp->value, var_value);
        return 1;
    }
    //如果是自定义变量
    tmp = vars;
    while(tmp != NULL && strcmp(tmp->name, var_name) != 0) tmp = tmp->next; //找到该自定义变量节点
    if(tmp != NULL) //确实存在该节点
    {
        strcpy(tmp->value, var_value);
    }
    //否则不存在这个变量节点，新建一个自定义变量
    struct var* node = (struct var*)malloc(sizeof(struct var));
    strcpy(node->name, var_name);
    strcpy(node->value, var_value);
    //将新节点node插入到vars链表尾部
    tmp = vars;
    while(tmp->next != NULL) tmp = tmp->next;
    tmp->next = node;
    node->next = NULL;
    //printf("New variable added: $%s=%s\n", node->name, node->value);
    return 1;
}

void initializeEnviron()
{
    environ = (struct var*)malloc(sizeof(struct var));
    strcpy(environ->name, "shell");
    sprintf(environ->value, "%s/myshell", shell_wd);
    environ->next = NULL;
}