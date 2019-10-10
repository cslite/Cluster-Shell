//https://stackoverflow.com/questions/7383803/writing-to-stdin-and-reading-from-stdout-unix-linux-c-programming
//https://unix.stackexchange.com/questions/505828/how-to-pass-a-string-to-a-command-that-expects-a-file

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<ctype.h>
#include<unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<sys/wait.h>


#define MAX_LINE_LEN 100
#define MAX_OUTPUT_LEN 2000

struct cmdstruc{
	char *cmd;
	char **args;
	char *inp;
};

typedef struct cmdstruc cmdstruc;

char **nodes = NULL;
int *port = NULL;
int *socket_fd;

/*
read the passed config file and store the mapping to an array
*/
void readMapping(char *filePath){
	FILE *fp = fopen(filePath,"r");
	int i,numc;
	if(fscanf(fp,"%d",&numc) == EOF)
		return;
	nodes = (char **)(calloc(numc+1,sizeof(char *)));
	port = (int *)(calloc(numc+1,sizeof(int)));
	for(i=0;i<=numc;i++){
		nodes[i] = (char *)(calloc(20,sizeof(char)));
	}
	for(i=1;i<=numc;i++){
		fscanf(fp,"%s%d",nodes[i],&port[i]);
	}
	sprintf(nodes[0],"%d",numc);
}


/*
Print the stored mapping to the stdout
*/
void printMapping(){
	if(nodes == NULL)
		return;
	int i,numc;
	sscanf(nodes[0],"%d",&numc);
	for(i=1;i<=numc;i++){
	    if(nodes[i] != NULL)
		    printf("n%d -> %s:%d\n",i,nodes[i],port[i]);
	}
}

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
Convert a raw command with arguments into a
cmdstruc type
*/
cmdstruc build_struct_from_rawstr(char *cmdstr, char *inp){
	cmdstruc cmd;
	if(inp != NULL){
		cmd.inp = (char *)calloc(strlen(inp)+2,sizeof(char));
		strcpy(cmd.inp,inp);
	}
	else
	    cmd.inp = NULL;
	char *tmpstr = (char *)calloc(strlen(cmdstr)+2,sizeof(char));
	strcpy(tmpstr,cmdstr);
	int ns; //number of spaces
	if((ns=numTk(tmpstr,' ')) == 0){
		//no arguments
		cmd.args = NULL;
		cmd.cmd = tmpstr;
		return cmd;
	}
	else{
		char *saveptr;
		cmd.args = (char **)(calloc(ns+2,sizeof(char *)));
		char *tk = strtok_r(tmpstr," ",&saveptr);
		int argi = 0;
		cmd.cmd = tk;
		while(tk != NULL){
			(cmd.args)[argi++] = tk;
			tk = strtok_r(NULL," ",&saveptr);
		}
		(cmd.args)[argi] = NULL;
		return cmd;
	}
}

/*
Gives approx maximum length for remote encoded
string.
*/
int get_str_len(cmdstruc cmd){
	//max str length
	int res = 1;
	res += strlen(cmd.cmd);
	if(cmd.inp != NULL)
		res += (1 + strlen(cmd.inp));
	else
		res += 1;
	if(cmd.args != NULL){
		int i = 0;
		while((cmd.args)[i] != NULL){
			res += (1 + strlen((cmd.args)[i++]));
		}
	}
	return (res + 5);
}

/* 
Generates the remote string, to be sent
over the network
*/
char *build_str_from_cmdstruc(cmdstruc cmd){
	//convert the struct data to a string to be sent over tcp
	char *remote_cmd = (char *)(calloc(get_str_len(cmd),sizeof(char)));
	strcpy(remote_cmd,cmd.cmd);
	if(cmd.args == NULL && cmd.inp == NULL)
		return remote_cmd;
	strcat(remote_cmd,"#");
	if(cmd.inp != NULL)
		strcat(remote_cmd,cmd.inp);
	else
	    strcat(remote_cmd,"null");
	if(cmd.args != NULL){
		int i = 0;
		while((cmd.args)[i] != NULL){
			strcat(remote_cmd,"#");
			strcat(remote_cmd,(cmd.args)[i++]);
		}
	}
	return remote_cmd;
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

//void exec_remote_i(int node_id, cmdstruc cmd){
//    cmd.inp = "intrac";
//    char *remote_cmd = build_str_from_cmdstruc(cmd);
//    if(nodes[node_id] != NULL){
//        write(socket_fd[node_id],remote_cmd,strlen(remote_cmd));
//        char *buff = (char *)(calloc(MAX_OUTPUT_LEN,sizeof(char)));
//        int ret;
//        if((ret=fork()) == 0){
//            while(1){
//                fgets(buff,MAX_OUTPUT_LEN,stdin);
////                scanf("%s",buff);
//                write(socket_fd[node_id],buff,strlen(buff));
//            }
//        }
//        while(1){
//            printf("hello!test_msg\n");
//            memset(buff,0,MAX_OUTPUT_LEN);
//            if(read(socket_fd[node_id],buff,MAX_OUTPUT_LEN) == 0)
//                break;
//            else if(strcmp(buff,"null") == 0)
//                break;
//            else{
//                printf("%s",buff);
//                fflush(stdout);
//            }
//
//        }
//        kill(ret,SIGKILL);
//    }
//}

char *exec_remote(int node_id, cmdstruc cmd){
	//get the cmd executed on remote machine(s) and return output
	char *remote_cmd = build_str_from_cmdstruc(cmd);
//	printf("%s\n\n",remote_cmd);
	if(nodes[node_id] != NULL){
	    //that means that we are connected to this socket
	    write(socket_fd[node_id],remote_cmd,strlen(remote_cmd));
	    char *buff = (char *)(calloc(MAX_OUTPUT_LEN,sizeof(char)));
	    if(read(socket_fd[node_id],buff,MAX_OUTPUT_LEN) == 0)
	        return NULL;
	    else if(strcmp(buff,"null")==0)
	        return NULL;
	    else
	        return buff;
	}
	return NULL;
}

/*
Executes the given command on local machine
and returns the output
*/
char *exec_local(cmdstruc cmd){
	if(strcmp(cmd.cmd,"cd") == 0){
		if(chdir((cmd.args)[1]) < 0)
		    perror("cd");
		return NULL;
	}
	int pr[2];
	int pw[2];
	pipe(pr);
	pipe(pw);
	if(fork() == 0){
		// print_struc(cmd);
		close(pr[1]);
		close(pw[0]);
		if(cmd.inp != NULL){
			close(0);
			dup2(pr[0],0);
		}
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
	char *buff = (char *)(calloc(MAX_OUTPUT_LEN,sizeof(char)));
	if(read(pw[0],buff,MAX_OUTPUT_LEN) == 0)
		return NULL;
	else
		return buff;
}

void unitTest(){
	/*
		//Unit Test: numTk
		printf("%d\n",numTk("Hello#is#tus##",'#'));
		printf("%d\n",numTk("Hello#is#tus##",'*'));
	*/
    /*
		//Unit Test: build_struct_from_rawstr

		// cmdstruc ctest = build_struct_from_rawstr("./hello pehla dusra2","yeh hai input");
		// cmdstruc ctest = build_struct_from_rawstr("./hello pehla dusra2",NULL);
		cmdstruc ctest = build_struct_from_rawstr("cat config.txt",NULL);
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
    */
	/*
		//Unit Test: build_str_from_cmdstruc

		cmdstruc ct;
		ct.cmd = "./hello";
		char *args[] = {"./hello","pehla",NULL};
		ct.args = args;
		// ct.args = NULL;
		ct.inp = "this is my input";
		// ct.inp = NULL;
		printf("%s\n",build_str_from_cmdstruc(ct));
	*/

		//Unit Test: build_struct_from_str
    /*
//		char str[] = "./hello#this is my input#./hello#pehla";
		char str[] = "cat#null#cat#config.txt";
		cmdstruc ctest = build_struct_from_str(str);
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
    */
	/*

		//Unit Test: exec_local
		cmdstruc ct;

		ct.cmd = "cat";
		char *args[] = {"cat","config",NULL};
		ct.args = NULL;
		ct.inp = NULL;

		char *tout = exec_local(ct);
		if(tout != NULL)
			printf("%s\n",tout);
	*/
}

void createConnection(int numc){
    struct sockaddr_in serveraddr[numc+1];
    socket_fd = (int *)(calloc(numc+1,sizeof(int)));
    char errmsg[20];
    for(int i=1;i<=numc;i++){
        memset(&serveraddr[i], 0, sizeof(serveraddr[i]));
        serveraddr[i].sin_family = AF_INET;
        serveraddr[i].sin_addr.s_addr = inet_addr(nodes[i]);
        serveraddr[i].sin_port = htons(port[i]);

        socket_fd[i] = socket(PF_INET,SOCK_STREAM,IPPROTO_TCP);
        int result = connect(socket_fd[i],(struct sockaddr *) &serveraddr[i],sizeof(serveraddr[i]));
        if(result == -1){
            sprintf(errmsg,"connect n%d",i);
            perror(errmsg);
//            printf("%s:%d",nodes[i],port[i]);
            nodes[i] = NULL;
        }
    }
}

int main(int argc, char *argv[]){

	 unitTest();

	if(argc != 2){
		printf("This shell accepts one argument, the path of the config file.\n");
		return -1;
	}
    readMapping(argv[1]);
    char *homedir = getenv("HOME");
    chdir(homedir); //as all nodes need to be in the home directory
    int numc,i;
    sscanf(nodes[0],"%d",&numc);
    createConnection(numc);
    printf("Cluster Shell Client Started. Current Working Dir: %s\n",homedir);
	while(1){
	    printf("$ ");
        char cmd[MAX_LINE_LEN] = {0};
		fgets(cmd,MAX_LINE_LEN,stdin);

		int cmdlen = strlen(cmd);
		if(cmdlen > 0)
			cmd[cmdlen-1] = '\0';   //remove the newline char

		if(strcmp(cmd,"exit") == 0){
            break;
		}
		else if(strcmp(cmd,"nodes") == 0)
			printMapping(nodes);
		else{
			char *tinp=NULL;
			char *tout = NULL;
			char *saveptr;
//			int pipecnt = numTk(cmd,'|');
			char *tk = strtok_r(cmd,"|",&saveptr);
//			int firstcmd = 1;
			while(tk != NULL){
				if(tk[0] == 'n' && tk[1] == '*'){
				//execute on all nodes
					int nop = 1;    //indicates there was no output

					char *rawstr;
					int si;
					sscanf(tk,"n*.%n",&si);
					rawstr = tk + si;
					cmdstruc cmds = build_struct_from_rawstr(rawstr,tinp);
						// print_struc(cmds);
					int i;
					char *iout = NULL;
					tout = (char *)(calloc(MAX_OUTPUT_LEN*numc,sizeof(char)));
					iout = exec_local(cmds);
					if(iout != NULL){
						nop = 0;
						strcpy(tout,iout);
					}
					else
						tout[0] = '\0';
					for(i=1;i<=numc;i++){
						
						iout = exec_remote(i,cmds);
						if(iout != NULL){
							nop = 0;
							strcat(tout,"\n");
							strcat(tout,iout);
						}
					}
					if(nop == 1)
						tout = NULL;
				}
				else if(tk[0] == 'n' && isdigit(tk[1])){
					//execute on a remote node
					int remote_id,si;
					char *rawstr;
					sscanf(tk,"n%d.%n",&remote_id,&si);
					rawstr = tk + si;
					cmdstruc cmds = build_struct_from_rawstr(rawstr,tinp);
//						 print_struc(cmds);
//                    if(pipecnt == 0){
//                        exec_remote_i(remote_id,cmds);
//                        tout = NULL;
//                    }
//                    else
                    tout = exec_remote(remote_id,cmds);
				}
				else{
					//local command
					cmdstruc cmds = build_struct_from_rawstr(tk,tinp);
						// print_struc(cmds);
					tout = exec_local(cmds);
				}
				// printf("tk : %s\n",tk);
				tk = strtok_r(NULL,"|",&saveptr);
				// printf("tk2 : %s\n",tk);
				tinp = tout;
				tout = NULL;
			}
			if(tinp != NULL)
				printf("%s\n",tinp);
		}

	}
	for(i=1;i<=numc;i++)
	    if(nodes[i] != NULL)
	        close(socket_fd[i]);
	return 0;
}



