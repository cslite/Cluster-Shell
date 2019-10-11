#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<unistd.h>
#include <signal.h>
#include<sys/wait.h>

#define MAX_PENDING 3
#define MAX_BUFF_SIZE 2000
int PORT = 8083;
int connfd;
struct cmdstruc{
    char *cmd;
    char **args;
    char *inp;
};

typedef struct cmdstruc cmdstruc;

/*
Count the number of occurrences of tk in str
*/
int numTk(char *str,char tk){
    int i;
    int len = strlen(str);
    int cnt = 0;
    for(i=0;i<len;i++)
        if(str[i] == tk)
            cnt++;
    return cnt;
}

/*
Generates the cmdstruc from the remote string
*/
cmdstruc build_struct_from_str(char *cmdstr){
    cmdstruc cmd;
    char *tmpstr = (char *)calloc(strlen(cmdstr)+2,sizeof(char));
    strcpy(tmpstr,cmdstr);
    int nh = numTk(tmpstr,'#');
    if(nh == 0){
        cmd.cmd = tmpstr;
        cmd.args = NULL;
        cmd.inp = NULL;
        return cmd;
    }
    else{
        char *tk = strtok(tmpstr,"#");
        int argi = 0;
        cmd.cmd = tk;
        cmd.inp = strtok(NULL,"#");
        if(strcmp(cmd.inp,"null") == 0)
            cmd.inp = NULL;
        if(nh > 1){
            cmd.args = (char **)(calloc(nh,sizeof(char *)));
            while(tk != NULL){
                (cmd.args)[argi++] = tk = strtok(NULL,"#");
            }
        }
        else
            cmd.args = NULL;
        return cmd;
    }
}
void print_struc(cmdstruc ctest){
    printf("cmd = %s\n",ctest.cmd);
    int ci = 0;
    if(ctest.args != NULL){
        printf("args = ");
        while((ctest.args)[ci] != NULL)
            printf("%s, ",(ctest.args)[ci++]);
        printf("\n");
    }
    if(ctest.inp != NULL)
        printf("input = %s\n",ctest.inp);
}
/*
Executes the given command on local machine
and returns the output
*/
char *exec_local(cmdstruc cmd){
//    print_struc(cmd);
    if(strcmp(cmd.cmd,"cd") == 0){
        chdir((cmd.args)[1]);
        return NULL;
    }
    if(cmd.inp != NULL && strcmp(cmd.inp,"stdin") == 0){
        if(fork() == 0){
            close(0);
            dup2(connfd,0);
            close(1);
            dup2(connfd,1);
            if(cmd.args != NULL){
                execvp(cmd.cmd,cmd.args);
                perror("execvp");
            }
            else{
                execlp(cmd.cmd,cmd.cmd,NULL);
                perror("execlp");
            }
            exit(0);
        }
        int status;
        wait(&status);
        return NULL;
    }
    int pr[2];
    int pw[2];
    pipe(pr);
    pipe(pw);
    if(fork() == 0){
//         print_struc(cmd);
        close(pr[1]);
        close(pw[0]);
        if(cmd.inp != NULL){
            close(0);
            dup2(pr[0],0);
        }
        else
            close(0);
        close(1);
        dup2(pw[1],1);
        if(cmd.args != NULL){
            execvp(cmd.cmd,cmd.args);
            perror("execvp");
        }
        else{
            execlp(cmd.cmd,cmd.cmd,NULL);
            perror("execlp");
        }
        exit(0);
    }
    close(pr[0]);
    close(pw[1]);
    if(cmd.inp != NULL){
        write(pr[1],cmd.inp,strlen(cmd.inp));
        close(pr[1]);
    }
    int st;
    wait(&st);
    char *buff = (char *)(calloc(MAX_BUFF_SIZE,sizeof(char)));
    if(read(pw[0],buff,MAX_BUFF_SIZE) == 0)
        return NULL;
    else
        return buff;
}




int main(int argc, char *argv[]) {
    char *homedir = getenv("HOME");
//    printf("%s\n",homedir);
    chdir(homedir); //as all nodes need to be in the home directory
    if(argc == 2){
        //optional port argument
        sscanf(argv[1],"%d",&PORT);
    }
    int listenfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenfd < 0) {
        perror("socket");
        return -1;
    }
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));
    if (listen(listenfd, MAX_PENDING) < 0) {
        perror("listen");
    }

    for(;;){
        //iterative server
        printf("Server Running on port %d.\n",PORT);
        clilen = sizeof(cliaddr);
        connfd = accept(listenfd,(struct sockaddr *) &cliaddr,&clilen);
        if(connfd < 0){
            perror("accept");
            continue;
        }
        printf("Connected to client %s\n",inet_ntoa(cliaddr.sin_addr));
        char buff[MAX_BUFF_SIZE] = {0};
        int ret;
        while((ret = read(connfd,buff,MAX_BUFF_SIZE)) > 0){
//            printf("%d\n",ret);
//            printf("%s\n",buff);
            cmdstruc cmd = build_struct_from_str(buff);
//            print_struc(cmd);
            printf("<Executing %s command>\n",cmd.cmd);
            char *out = exec_local(cmd);

            if(out == NULL)
                out = "null";
//            printf("%s\n",out);
            write(connfd,out,strlen(out));

            memset(buff,0,sizeof(char)*MAX_BUFF_SIZE);
        }
        printf("Closing connection.\n");
    }
    return 0;
}
