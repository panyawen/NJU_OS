#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <error.h>
#include <fcntl.h>
#include <sys/types.h>

#define LINE_LEN 128
#define PATH_MAX_NUM 100

int get_path(char values_path[][LINE_LEN]){ //Get PATH by getenv()
	int i, j, len, path_cnt = 0;
	char *path;
	path = getenv("PATH"); //path free
	len = strlen(path);
	j = 0;
	for(i = 0; i < len; i ++){
		if(path[i] == ':') {
			values_path[path_cnt][j] = '\0';
			path_cnt ++;
			j = 0;
		}else{
			values_path[path_cnt][j ++] = path[i];
		}
	}
	values_path[path_cnt ++][j] = '\0';

	return path_cnt;
}

//validate file exist or executable
int validate(char values_path[][LINE_LEN], int path_cnt, char path_file[], char* file){
	int i, file_len, abs_or_rel = 0;
	file_len = strlen(file);

	for(i = 0; i < file_len; i ++)
		if(file[i] == '/')
			abs_or_rel = 1;

	if(abs_or_rel || path_cnt == 0){ //absolute or relative path
		if(access(file, F_OK) != -1 && access(file, X_OK) != -1){
			strcpy(path_file, file);
	//		printf("%s\n", path_file);
			return 0;
		}
		return -1;
	}

	//current path '.'
	if(access(file, F_OK) != -1 && access(file, X_OK) != -1){
		strcpy(path_file, "./");
		strcat(path_file, file);
//		printf("%s\n", path_file);
		return 0;
	}

	for(i = 0; i < path_cnt; i ++){
		if(values_path[0] != '\0'){
			strcpy(path_file, values_path[i]);
			strcat(path_file, "/");
			strcat(path_file, file);
	//		printf("%s\n", path_file);
			if(access(path_file, F_OK) != -1 && access(path_file, X_OK) != -1)
				return 0;	
		}
	}
	return -1;
}

int main(int argc, char *argv[]) {
	pid_t pid;
	int fd_pipe[2];
	char *para[10] = {"/usr/bin/strace", "-c"};
	char values_path[PATH_MAX_NUM][LINE_LEN], path_file[LINE_LEN];
	int para_cnt, path_cnt;

	path_cnt = get_path(values_path);

	if(validate(values_path, path_cnt, path_file, argv[1]) == -1){
		printf("File not exist or file is not executable.\n");
		exit(4);
	}

	para[2] = path_file;
	para_cnt = 3;
	for(int i = 2; i < argc; i ++)
		para[para_cnt ++] = argv[i];
	para[para_cnt] = NULL;

	if(pipe(fd_pipe) == -1){
		printf("Create pipe error!\n");
		exit(1);
	}
	
	pid = fork();
	if(pid < 0){
		printf("Fork error!\n");
		exit(2);
	}


	if(pid == 0){ //child process write info to pipe
		close(fd_pipe[0]);
		int fd_null = open("/dev/null", O_WRONLY);
		dup2(fd_null, 1);
		dup2(fd_pipe[1], 2);
		execv(para[0], para);
		
		exit(0);
	}else{ //father process
		close(fd_pipe[1]);
		
		if(wait(NULL) < 0){
			printf("Wait child error!\n");
			exit(3);
		}

		fcntl(fd_pipe[0], F_SETFL, O_NONBLOCK | O_ASYNC);
		char line[LINE_LEN];
		int len = 0;
		while((len = read(fd_pipe[0], line, LINE_LEN - 1)) > 0){
			line[len] = 0;
			printf("%s", line);
		}
		close(fd_pipe[0]);
	}

    return 0;
}
