#include <stdlib.h> 
#include <iostream> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <unistd.h> 
#include <fcntl.h>
#include <stdio.h>

int main(){
    char *const parmList[] = {"/usr/bin/strace", "-c", "../../m1/pstree/pstree", NULL};
    int pipes[2];
    pipe(pipes);
    pid_t child = fork();
    if(child == 0){
        close(pipes[0]);
		int fd = open("/dev/null", O_WRONLY);
		dup2(fd, 1);
        dup2(pipes[1],2);
        execv(parmList[0], parmList);
    }
    else{
        close(pipes[1]);
		int status;
        wait(&status);

        fcntl(pipes[0], F_SETFL, O_NONBLOCK); 
        char buf[128] = {0}; 
        ssize_t bytesRead; 
        std::string stdOutBuf; 
        while(1) {
	//		std::cout << buf;
            bytesRead = read(pipes[0], buf, sizeof(buf)-1);
            if (bytesRead <= 0)
                break;
            buf[bytesRead] = 0;
            stdOutBuf += buf;
        } 
        std::cout << "<stdout>\n" << stdOutBuf << "\n</stdout>" << std::endl; 
    }

    close(pipes[0]);
    close(pipes[1]);

    return 0;
 }
