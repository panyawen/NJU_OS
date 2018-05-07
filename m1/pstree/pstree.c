#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#define MAX 1000
#define PROCESS_NAME_LEN 30
#define PID_LEN 10
#define MAX_COUNT_CHILD_PROCESS 20

bool print_pid = false, sort_by_pid = false, print_v = false;
char base_dir[] = "/proc";
char info_file[] = "stat";

struct process{
	char pid[PID_LEN];
	char ppid[PID_LEN];
	char name[PROCESS_NAME_LEN];
	struct process* brother; //belong to the same father process
	struct process* child;   //the child processes
	struct process* next;	 //used to travel all process when build a relation tree
};

int parse_argv(int argc, char* argv[], bool* print_pid, bool* sort_by_pid, bool* print_v){
	
	for(int i = 1; i < argc; i++){
		if(strcmp(argv[i], "-p") == 0)
			*print_pid = true;
		else if(strcmp(argv[i], "-n") == 0)
			*sort_by_pid = true;
		else if(strcmp(argv[i], "-V") == 0)
			*print_v = true;
		else
			return 1;
	}
	return 0;
}

int is_digit(char* p){
	while(*p != '\0'){
		if(*p >= '0' && *p <= '9')
			p ++;
		else
			return 0;
	}
	return 1;
}

int get_all_pid(char all_pid[][10], int* process_cnt){
	DIR *dir;
	struct dirent *ptr;
	if((dir = opendir(base_dir)) == NULL){
		printf("open directory error\n");
		return 1;
	}

	while((ptr = readdir(dir)) != NULL){	
		if(ptr->d_type == 4){  // we just need dir
			if(is_digit(ptr->d_name)){  // take pid as the name of dir
				strcpy(all_pid[(*process_cnt) ++], ptr->d_name); 
			}
		}
	}
	return 0;
}

int read_process_name_ppid(int fd, char process_name[], char ppid[]){
	char c;
	int len = 0;
	
	while(read(fd, &c, 1) == 1 && c == ' ') // read ' ' before pid
		;
	
	while(read(fd, &c, 1) == 1 && c != ' ') // locate process's name
		;
	
	read(fd, &c, 1); // read '('

	while(read(fd, &c, 1) == 1 && c != ' ')
		process_name[len ++] = c;
	process_name[len - 1] = '\0';  //remove the last ')'
	
	while(read(fd, &c, 1) == 1 && c == ' ')
		;

	while(read(fd, &c, 1) == 1 && c != ' ') //locate ppid 
		;

	len = 0;
	while(read(fd, &c, 1) == 1 && c != ' ')
		ppid[len ++] = c;
	ppid[len] = '\0';

	return 0;
}

int get_process_info(char all_pid[][10], int process_cnt, struct process** head){
	struct process* pos = NULL;
	for(int i = 0; i < process_cnt; i ++){
		char file_name[100];
		strcpy(file_name, base_dir);
		strcat(file_name, "/");
		strcat(file_name, all_pid[i]);
		strcat(file_name, "/");
		strcat(file_name, info_file);
		int fd = open(file_name, O_RDONLY);

		if(fd < 0){
			printf("Open file error!\n");
			return 1;
		}
	
		if(*head == NULL){		
			if((pos = (struct process*)malloc(sizeof(struct process))) == NULL){
				printf("malloc memory error!\n");
				return 1;
			}
			*head = pos;
		}else{
			if((pos->next = (struct process*)malloc(sizeof(struct process))) == NULL){
				printf("Malloc memory error!\n");
				return 1;
			}
			pos = pos->next;
		}
	
		strcpy(pos->pid, all_pid[i]);
		pos->brother = NULL;
		pos->child = NULL;
		pos->next = NULL;

		char process_name[PROCESS_NAME_LEN];
		char ppid[PID_LEN];

		if(read_process_name_ppid(fd, process_name, ppid)){
			printf("read process information error!\n");
			return 1;
		}

		strcpy(pos->name, process_name);
		strcpy(pos->ppid, ppid);

		close(fd);
	}
	return 0;
}

int build_process_tree(struct process* head, struct process** root){
	struct process* pos = head;
	while(pos != NULL){
		if(strcmp(pos->ppid, "0") == 0){ //root process, hasn't father process
			if(*root == NULL){
				*root = pos;
				pos->brother = NULL;
			}else{
				pos->brother = *root;
				*root = pos;
			}
		}else{  // exist father process 
			struct process* find_p = head;
			for( ; find_p; find_p = find_p->next){
				if(strcmp(find_p->pid, pos->ppid) == 0){ // find the father process
					pos->brother = find_p->child;
					find_p->child = pos;
					break;
				}
			}
			if(find_p == NULL){
				printf("can't find %s's father process %s\n", pos->pid, pos->ppid);
				return 1;
			}
		}
		pos = pos->next;
	}
	return 0;
}

int print_process_tree(struct process* root, int* offset, char offset_char[], int offset_cnt){
	if(root == NULL)
		return 0;
	
	printf("%s(%s)", root->name, root->pid);
	if(root->child){
		printf("--");
		if(root->child->brother)
			offset_char[offset_cnt] = '|';
		else
			offset_char[offset_cnt] = ' ';
		offset[offset_cnt] = offset[offset_cnt - 1] + strlen(root->name) + strlen(root->pid) + 4;
		offset_cnt ++;
	
		print_process_tree(root->child, offset, offset_char, offset_cnt);
		
		offset_cnt --;
	}
	
	if(root->brother){
		printf("\n");
		for(int i = 1; i < offset_cnt; i ++){
			for(int j = offset[i - 1]; j < offset[i] - 1; j ++)
				printf(" ");
			printf("%c", offset_char[i]);
		}
		if(strcmp(root->brother->ppid, "0") != 0)
			printf("-");
		print_process_tree(root->brother, offset, offset_char, offset_cnt);
	}

	return 0;
}

struct process* swap(struct process *p1, struct process *p2, struct process *root){
	struct process *pre_p1, *pre_p2, *pos;
	pos = root;
	pre_p1 = root;
	while(pos && pos != p1){  //find p1's pre
		pre_p1 = pos;
		pos = pos->brother;
	}
	
	pre_p2 = pos;
	while(pos && pos != p2){  //find p2's pre
		pre_p2 = pos;
		pos = pos->brother;
	}
	
	if(pos == NULL){
		printf("can't find process that needed swap!\n");
		return NULL;
	}

	if(pre_p1 == p1){
		if(pre_p2 == p1){
			root = p2;
			p1->brother = p2->brother;
			p2->brother = p1;
		}
		
		else{
			struct process *tmp = p1->brother;
			root = p2;
			p1->brother = p2->brother;
			pre_p2->brother = p1;
			p2->brother = tmp;
		}
	}
	if(pre_p1 != p1){
		if(pre_p2 == p1){
			p1->brother = p2->brother;
			p2->brother = p1;
			pre_p1->brother = p2;
		}else{
			struct process *tmp = p1->brother;
			p1->brother = p2->brother;
			pre_p2->brother = p1;
			p2->brother = tmp;
			pre_p1->brother = p2;
		}
	}

	return root;
}

int compare(char c1[], char c2[]){
	if(strlen(c1) != strlen(c2))
		return strlen(c1) - strlen(c2);
	while((*c1) != '\0' && (*c2) != '\0'){
		if((*c1) != (*c2))
			return (*c1) - (*c2);
		c1 ++;
		c2 ++;
	}
	return 0;
}

struct process* sort(struct process* root){
	struct process *cur_i, *cur_j, *cur_k;
	cur_i = root;

	struct process *next_cmp;

	while(cur_i){
		cur_k = cur_i;
		cur_j = cur_i->brother;

		while(cur_j){
		//	printf("%s %s %d\n", cur_j->pid, cur_k->pid, compare(cur_j->pid, cur_k->pid));
			if(sort_by_pid){
				if(compare(cur_j->pid, cur_k->pid) < 0)
					cur_k = cur_j;
			}else{ // sort by name
				if(strcmp(cur_j->name, cur_k->name) < 0)
					cur_k = cur_j;
			}
			cur_j = cur_j->brother;
		}
		
		next_cmp = cur_i->brother;
		if(cur_i != cur_k){
			root = swap(cur_i, cur_k, root);

			if(root == NULL){
				printf("swap error!\n");
				return 1;
			}
		}
		cur_i = next_cmp;
	}


	return root;
}

int sort_process(struct process **father, int deep){
	if(deep == 1 && (*father) == NULL)
		return NULL;
	else if(deep != 1 && (*father)->child == NULL)
		return NULL;

	struct process *new_root;

//	printf("%d  %s %s\n", deep, (*father)->pid, (*father)->child->pid);

	if(deep == 1)
		new_root = sort(*father); // sort root and it's brother
	else
		new_root = sort((*father)->child);

	if(new_root == NULL){
		printf("Sort error!\n");
		return 1;
	}

	if(deep == 1)
		*father = new_root;
	else
		(*father)->child = new_root;

	while(new_root){
		sort_process(&new_root, deep + 1); 
		new_root = new_root->brother;
	}
	return 0;
}

int free_tree(struct process *root){
	if(root == NULL)
		return 0;
	if(root->child){
		if(free_tree(root->child)){
			return 1;
		}
	}
	struct process *brother = root->brother;
	free(root);
	if(brother){
		if(free_tree(brother)){
			return 1;
		}
	}
	return 0;
}

int main(int argc, char *argv[]) {
    int i, process_cnt = 0, offset[MAX_COUNT_CHILD_PROCESS] = {0};
    char all_pid[MAX][10], offset_char[MAX_COUNT_CHILD_PROCESS]; 
	struct process* root = NULL, *head = NULL;
    for (i = 0; i < argc; i++) {
      assert(argv[i]); // specification
      printf("argv[%d] = %s\n", i, argv[i]);
    }
    assert(!argv[argc]); // specification


    if(argc > 1 && parse_argv(argc, argv, &print_pid, &sort_by_pid, &print_v)){ // parse error
		printf("Can't parse parameter!\n");
		exit(1);
	}

	if(get_all_pid(all_pid, &process_cnt)){ //get pids
		printf("Get pids error!\n");
		exit(2);
	}

	printf("process:%d\n", process_cnt);
	
	if(get_process_info(all_pid, process_cnt, &head)){ //get ppid, name;
		printf("get process information error!\n");
		exit(3);
	}

/*	struct process *tmp = head;
	while(tmp){
		printf("%s %s %s\n", tmp->pid, tmp->ppid, tmp->name);
		tmp = tmp->next;
	}
*/
	if(build_process_tree(head, &root)){ //root represent tree's root
		printf("build process tree error!\n");
		exit(4);
	}

	if(root == NULL){
		printf("No process!\n");
		return 0;
	}

	if((sort_process(&root, 1))){
		printf("sort process tree error!\n");
		exit(6);
	}

	if(print_process_tree(root, offset, offset_char, 1)){  //print
		printf("print process tree error!\n");
		exit(6);
	}

	if(free_tree(root) == 1){
		printf("free tree error!\n");
		exit(7);
	}
	
	printf("\n");

    return 0;
}
