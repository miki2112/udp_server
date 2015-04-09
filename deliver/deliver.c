#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> 
#include <assert.h>
#include <stdbool.h>

#define FIlESIZE 2048
#define BUFSIZE 1024
#define MAXLEN 4096 
#define DEFLEN 64
#define DATALEN 1024

int sender(FILE *fp, int sockfd, long *len, struct sockaddr *addr, int addrlen, socklen_t *len_recvfrom,char *filename); 

void error(char *msg) {
    perror(msg);
    exit(0);
}

struct packets {
    unsigned int total_frag;
    unsigned int frag_no;
    unsigned int size;
    char* filename;
    char filedata[DATALEN];
} ;

int main(int argc, char **argv) {
    int sockfd, port,cl_port ;
    struct sockaddr_in serveraddr,client;
    struct hostent *server;
    char *hostname;
    char *filename;
	long len;
	socklen_t len_recvfrom;
   	
   	assert(argc>0);
    if (argc != 5) {
       fprintf(stderr,"usage: %s <server address> <server port number> <client listen port> <file name> \n", argv[0]);
       exit(0);
    }

    hostname = argv[1];
    port = atoi(argv[2]);
    cl_port = atoi(argv[3]);
    filename = argv[4];
 
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) 
        error("ERROR opening socket");

    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

  
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, 
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(port);

    bzero((char *)&client, sizeof(client));
    client.sin_family = AF_INET; client.sin_port = htons(cl_port);
    client.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sockfd, (struct sockaddr *)&client, sizeof(client)) == -1) {
		fprintf(stderr, "Can't bind name to socket\n");
		exit(1); 
	}

	FILE *file = fopen(filename, "rb");
    if(file == NULL){
        fprintf(stderr, "Can't find the file\n");
        exit(1); 
    }

    sender(file, sockfd, &len, (struct sockaddr *)&serveraddr, sizeof(struct sockaddr_in), &len_recvfrom, filename);

	close(sockfd);
	fclose(file);
	exit(0);
}



int sender(FILE *fp, int sockfd, long *len, struct sockaddr *addr, int addrlen, socklen_t *len_recvfrom , char *filename){
	
	char *buf;
	long file_size, file_offset;
	char sends[DATALEN];
	int n, slen ,ack_rec,total_frag, frag_no;
	struct packets packets[DATALEN];
	file_offset = 0;
	bool prev_msg_acked = 1;
	int ack_number = 1;

	fseek(fp, 0, SEEK_END);
	file_size = ftell (fp);
	
	int rest =file_size%1000;
	total_frag = file_size/1000;
    if(rest !=0)
       total_frag++;
  
	buf = (char *) malloc(file_size+1);
	if (buf == NULL)
       exit (2);
	fread(buf, 1, file_size, fp);
	
	frag_no = 1;
	buf[file_size] ='\0'; 
	while(file_offset <= file_size && frag_no <=total_frag){

		if (prev_msg_acked){

			printf("Sending packets %d\n", frag_no );
			if ((file_size+1-file_offset) <= DATALEN) 
				slen = file_size+1-file_offset;
			else 
				slen = DATALEN;

			packets[frag_no].filename =filename;
        	packets[frag_no].size =slen;
			packets[frag_no].total_frag =total_frag;
        	packets[frag_no].frag_no =frag_no;
        	sprintf(sends, "%d:%d:%d:%s:",packets[frag_no].total_frag, packets[frag_no].frag_no, packets[frag_no].size ,packets[frag_no].filename);
        	int offset = strlen(sends);
        	memcpy(packets[frag_no].filedata,(buf+file_offset),slen);
			memcpy(sends+offset, (buf+file_offset), slen);
			if((n = sendto(sockfd, sends, slen+offset, 0, addr, addrlen)) == -1) {
				fprintf(stderr, "Can't send packets\n");
				exit(1);
			}
			
			if (ack_number == 1)
				ack_number = 0;
			else
				ack_number = 1;
			
			file_offset += slen;
			frag_no++;
		
		//wait for server to ack 
			char message[4];
			if ((n = recvfrom(sockfd,message, sizeof(message), 0, addr, len_recvfrom)) == -1){
				fprintf(stderr, "Can't recieve\n");
				exit(1);
			}
			ack_rec = atoi(message);
			if (ack_rec != ack_number ) {
				printf("ACK check fails\n");
				prev_msg_acked = 0;
			}
			else
				prev_msg_acked = 1;
			}
		else{	// if fail, resend that packet
			frag_no --;
			file_offset -=slen;
		}
	}

	return 1;
}

