#include <stdio.h>
#include <sys/types.h> 
#include <sys/socket.h> 
#include <netinet/in.h>
#include <assert.h>
#include <sys/stat.h>
#include <stdbool.h>


#define MAXLEN 4096 
#define DEFLEN 64
#define MAX_USERNAME_LEN 64
#define MAX_HOST_LEN 64
#define MAX_FILE_NAME_SIZE 64
#define MESSAGE_SIZE 64
#define BUFSIZE 1024*1024*4
#define DATALEN 1024


void recieve(int sockfd, int *ack_number); 
int main(int argc, char *argv[])
{
	int sockfd;
	struct sockaddr_in server,client;

    //create socket
	if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) == -1) {
		printf(stderr, "Can't create a socket\n");
		exit(1);
	}
	
	int port = atoi(argv[1]);	
	bzero((char *)&server, sizeof(server)); 
	server.sin_family = AF_INET; 
	server.sin_port = htons(port); 
	server.sin_addr.s_addr = htonl(INADDR_ANY); 
	if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) == -1) {
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1); 
	}
	
	while(1){
		int ack_number = 1;
		recieve(sockfd, &ack_number);
	}

	close(sockfd);
	exit(0);
}

void recieve(int sockfd, int *ack_number)
{	
    FILE *fp;
	char buf[BUFSIZE];
	char recvs[DATALEN];
	char message[MESSAGE_SIZE];
	char file_name[MAX_FILE_NAME_SIZE];
	char filename[MAX_FILE_NAME_SIZE];
	char buf_size[16];
	char buf_tot[16];
	char buf_no[16];
	bool is_end = 0;
	int bytes = 0;
	long lseek = 0;
	int ack_send , size, frag_no, total_frag,prev_offset ;
	int prev_frag_no =0;
	struct sockaddr_in client;
	socklen_t len = sizeof(struct sockaddr_in);

	while(!is_end){

		if ((bytes = recvfrom(sockfd, &recvs, 1024, 0, (struct sockaddr *)&client, &len)) == -1){
			fprintf(stderr, "Can't receive datagram\n"); 
			exit(1);
		}

		int count = 0;
		int q = 0;
		int j = 0;
		int count_t =0, count_s =0 , count_no =0;

		// parse recvs 
		while(count!=4){
			if(count ==3&&recvs[q] != ':' ){
				file_name[j] = recvs[q];
				j++;
			}
			else if(count ==0&&recvs[q] != ':' ){
				buf_tot[count_t] = recvs[q];
				count_t++;			
			}
			else if(count ==1&&recvs[q] != ':' ){
				buf_no[count_no] = recvs[q];
				count_no++;
			}
			if(count ==2&&recvs[q] != ':' ){
				buf_size[count_s] = recvs[q];
				count_s++;
			}
			else if(recvs[q] == ':'){
				count++;
			}
			q++;
		}

		buf_size[count_s] ='\0';
		buf_no[count_no] ='\0';
		buf_tot[count_t] ='\0';
		file_name[j] ='\0';
		size =atoi(buf_size);
		frag_no = atoi(buf_no);
		total_frag = atoi(buf_tot);
		printf("handling  total %d: No.%d with size %d \n",total_frag,frag_no,size );
		
		if (*ack_number == 1)
			*ack_number = 0;
		else
			*ack_number = 1;
		
		if (recvs[bytes-1] == '\0' || frag_no == total_frag){		// end if recieve all packets or get eof signal
			is_end = 1;
			bytes--;
		}

		if(prev_frag_no == frag_no){	// error occurs
			lseek -=prev_offset;
		}
		memcpy((buf+lseek), recvs+q, (bytes-q));
		lseek += (bytes-q);
		prev_offset =(bytes-q);

		ack_send = *ack_number;
		
		if(*ack_number==1)
			sprintf(message ,"1");
		else
			sprintf(message ,"0");
		
		if ((bytes = sendto(sockfd, message, sizeof(message), 0, (struct sockaddr *)&client, len)) == -1){
			fprintf(stderr, "ACK error\n"); 
			exit(1);
		}
	}

	// create a storage directory to store files 
	struct stat st = { 0 };
	if (stat("./storage", &st) == -1) {
		mkdir("./storage", 0700);
	}
	strcpy(filename,"./storage/");
	strncat (filename, file_name, sizeof(file_name));
		
		
	if ((fp = fopen (filename, "wb")) == NULL){
		fprintf(stderr, "File doesn't exist\n"); 
		exit(0);
	}

	fwrite (buf, 1, lseek, fp); 
	fclose(fp);
	printf("Success\n");

}
