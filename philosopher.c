#include <stdlib.h>
#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <unistd.h> 

int main(int argc, char* argv[]){
	
	if (argc !=2){
		printf("Incorrect number of parameters, received %d but expected 2. \nTemplate %s <(int)id>\n", argc, argv[0]);
		exit(1);
	}
	
	int id = atoi(argv[1]);
	char *str1 = "/tmp/ph_rqst";
	char *str2 = "/tmp/ph_rls";
	char fifo_rqst[20];
	char fifo_rls[20];
	int fd_rqst, fd_rls;
	char bin;
	int ret;
	unsigned long int step=0;
	
	printf("Philosopher %d spawned, PID %d\n", id, getpid());
	// fifo opening
	strcpy(fifo_rqst, "");
	strcpy(fifo_rls, "");
	sprintf(fifo_rqst, "%s%d", str1, id);
	sprintf(fifo_rls, "%s%d", str2, id);

	if((fd_rls  = open(fifo_rls,  O_WRONLY))<0){ perror("Release pipe opening"); exit(fd_rls);  }
	if((fd_rqst = open(fifo_rqst, O_WRONLY))<0){ perror("Request pipe opening"); exit(fd_rqst); }

	printf("Philosopher %d sat to the table\n", id); fflush(stdout);
	
	while(1){
		
		bin = 'c';
		// think for a random amount of time, max 2 sec (actually 1999ms)
		printf("%d thinking...\n", id);
		usleep(rand()%2000 * 1000);
		
		// request chopsticks!
		printf("%d asking for chopsticks\n", id);
		if(write(fd_rqst, &bin, sizeof(char)) < 0) perror("PH write");
		printf("%d waiting for chopsticks\n", id); fflush(stdout);
		close(fd_rqst);
		// WAIT until confirmation
		if((fd_rqst = open(fifo_rqst, O_RDONLY))<0){ perror("Request pipe opening readonly"); exit(fd_rqst); }
		if((ret=read(fd_rqst, &bin, sizeof(char))) < 0){ perror("PH write"); exit(ret); }
		close(fd_rqst);
		if((fd_rqst = open(fifo_rqst, O_WRONLY))<0){ perror("Request pipe opening"); exit(fd_rqst); }

		// write itself is not blocking (unless I write more char than fifo size, which seems overdoing it
		// filling the pipe would require the assumptions that no one but the waiter is reading from it.
		
		// If the process managed to arrive here means that it received the chopsticks
		// eat for max 3s
		printf("%d eating, %lu!\n", id, step++); fflush(stdout);
		usleep(rand()%3000 * 1000);
		printf("%d releasing chopsticks!\n", id);
		if((ret=write(fd_rls, &bin, sizeof(char))) < 0){ perror("PH write chopstick release"); exit(ret); }
		// release chopsticks
		
	}
	
	return 0;
}