#include <stdlib.h>
#include <stdio.h> 
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <sys/time.h>
#include <unistd.h> 


int main(int argc, char* argv[]){

	if (argc!=2){
		printf("Wrong number of input argument, %d inserted - 2 expected\nexecutable should be called using sintax:\t%s <number of philosophers>\n", argc, argv[0]); fflush(stdout);
		exit(1);
	}
	
	int PH_NUM = atoi(argv[1]);
	
	char *str1 = "/tmp/ph_rqst";		// suppose their name is already known by philosophers
	char *str2 = "/tmp/ph_rls";
	char fifo_rqst[PH_NUM][20];
	char fifo_rls[PH_NUM][20];
	int n=0;
	fd_set rqst, rls;
	struct timeval tv;
	int fd_rqst[PH_NUM], fd_rls[PH_NUM], readable[PH_NUM], chop[PH_NUM];	// I like the chopsticks version more, sorry. Also, I'm sure I'd end up confusiong fork[] with fork()
	int nfds_rqst, nfds_rls;
	int acc_rqst, n_read;
	char bin;
	int tmp=0;
	int ret;

	
	for(int i=0; i<PH_NUM; i++){
		chop[i]=1;
		strcpy(fifo_rqst[i], "");
		strcpy(fifo_rls[i], "");
		sprintf(fifo_rqst[i], "%s%d", str1, i);
		sprintf(fifo_rls[i], "%s%d", str2, i);

		if((fd_rls[i]  = open(fifo_rls[i], O_RDONLY))<0)	{ perror("Release pipe opening"); exit(fd_rls[i]);  }
		if((fd_rqst[i] = open(fifo_rqst[i], O_RDONLY))<0)	{ perror("Request pipe opening"); exit(fd_rqst[i]); }
		// Order of the open is important! Open on a named pipe hangs until both ends are open.
		
		// what would happen if we tried to open first the "release" and then "request" here for a certain philosopher
		// but inside it reversed the order?
	}

	tv.tv_sec = 1;
	tv.tv_usec = 0;
	// maximum time to listen to the pipes during the selects
	
	nfds_rls = fd_rls[PH_NUM-1]+1; // select needs to get the highest filedescriptor (+1) of a set it's evaluating.
	
	while(1){
		
		FD_ZERO(&rls);
		FD_ZERO(&rqst);
		
		for(int i=0; i<PH_NUM; i++){
			FD_SET(fd_rls[i], &rls);
			readable[i] = -1; // just clearing the array to be safe
		}
		
		// -------------- CHOPSTICKS RELEASES ------------------------
		printf("Waiter selecting the released chopsticks\n"); fflush(stdout);
		if((ret=select(nfds_rls, &rls, NULL, NULL, &tv)) < 0){	perror("Select on release pipes"); exit(ret); }
		for(int i=0; i<PH_NUM; i++){
			if(FD_ISSET(fd_rls[i], &rls)){
				read(fd_rls[i], &bin, sizeof(char));	// I read just one byte, it's not meaningful, in order to empty the pipe for next possible read
														// 	since I read from all there's no priviliged process
				chop[i] = 1;							// free up again the chopsticks the i-th philosopher took.
				chop[(i+1)%PH_NUM] = 1;
			}
		}
		
		printf("PRE ASSIGNMENT\n");						// Print chopsticks status
		for(int i=0; i<PH_NUM; i++){
			printf("Chop %d: %d\n", i, chop[i]);
		}
		fflush(stdout);
		// I can allow a release from multiple sources, I'm sure data on those FIFO won't be consumed by other processes, so I can trust they'll be there
		// even if I attempt multiple subsequent read in the same cycle
		
		// -------------- CHOPSTICKS REQUESTS -------------------------
		
		for(int i=0; i<PH_NUM; i++){
			if (chop[i] && chop[(i+1)%PH_NUM]) { FD_SET(fd_rqst[i], &rqst); nfds_rqst = fd_rqst[i]+1; }
		}
		// NOTE: nfds_rqst needs to be updated, depending on which pairs of chopsticks are available, which in turn define which pipes we are going to listen to
		
		if((ret=select(nfds_rqst, &rqst, NULL, NULL, &tv)) < 0){ perror("Select on request pipes"); exit(ret); }
		// now randomness is mandatory, I can select just one philosphers!
		
		n_read=0;
		
		for(int i=0; i<PH_NUM; i++){
			if(FD_ISSET(fd_rqst[i], &rqst)){
				readable[n_read++] = i;	// readable contains the indexes of the readable request FIFO inside their array in its first n_read positions, -1 elsewhere
			}
		}
		
		if (n_read > 0){				// if no philospher requested chopsticks, go to next iteration
		
			// choose randomly one of the readable FIFOs
			tmp = rand()%n_read;		
			acc_rqst = readable[tmp]; 	// FIFO from which the waiter is gonna read
			// it's not completely random, first because rand() in not crypto-safe, than 'cause probably max value it could return is not a multiple of n_read
			// but we'll make do with what we have easily accessible
			if((ret=read(fd_rqst[acc_rqst], &bin, sizeof(char)))<0){ perror("Read from readable pipe gone wrong. Probably caused by an interrupt"); exit(ret); }
			// FIFO should be readable by now, if errors occour it's highly plausible some signal interrupted the communication.
			// again we read not to obtain info, but just to empty the FIFO
			
			chop[acc_rqst] = 0;					// chopsticks are now taken by the process
			chop[(acc_rqst+1)%PH_NUM] = 0;
			
			// The next open-close sequence is explained on the long comment down below!
			close(fd_rqst[acc_rqst]);
			if((fd_rqst[acc_rqst] = open(fifo_rqst[acc_rqst], O_WRONLY))<0){	perror("Confirming accepted request"); close(fd_rqst[acc_rqst]); }
			bin = 'c';
			if((ret=write(fd_rqst[acc_rqst], &bin, sizeof(char)))<0){ perror("Read from readable pipe gone wrong. Probably caused by an interrupt"); exit(ret); }
			printf("Accepted philosopher %d\n", acc_rqst); fflush(stdout);
			
			printf("POST ASSIGNMENT\n");
			for(int i=0; i<PH_NUM; i++){
				printf("Chop %d: %d\n", i, chop[i]);
			}
			
			close(fd_rqst[acc_rqst]);
			if((fd_rqst[acc_rqst] = open(fifo_rqst[acc_rqst], O_RDONLY))<0){ perror("Reopening accepted request pipe in readonly mode"); exit(fd_rqst[acc_rqst]);}
		
			printf("------------------\n");
			
		}
		sleep(1); // one second delay between FIFO sampling, to avoid continuos check, which basically would result in a busy waiting
		
		// how to block/unblock ph:
		/*
			ph opens pipe in wronly
			ph writes that it wants to eat 
			ph closes pipe from his end
			ph opens pipe in read_only, which will hang
			
			waiter opens all pipes_rqst in rd_only
			waiter uses select to read who wants chops
			waiter closes selected pipe_rqst
			waiter opens selected pipe_rqst in wr_only
			waiter sends over the pipe, so that the ph will be able to read and continue with the rest
			waiter closes selected pipe_rqst and reopens it in readonly mode
		*/
	}
	
	// note that it does not wait for children since they won't terminate spontaneously

	return 0;
}
