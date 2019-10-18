#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>

#define SIZE_MSG 1000

int readN(int socket, char* buf);


int main(int argc, char** argv) {

	printf("Server\n"); fflush(stdout);

	struct sockaddr_in sockaddr;
	sockaddr.sin_family = AF_INET;

	char input_buf[SIZE_MSG];
	int sock = -1;
	int rc = -1;
	
	for(;;){
		printf("Please, write IP address:\n"); fflush(stdout);
		memset(input_buf, 0, sizeof(input_buf));
		fgets(input_buf, sizeof(input_buf), stdin);
		input_buf[strlen(input_buf) - 1] = '\0';
		sockaddr.sin_addr.s_addr = inet_addr(input_buf); 
		printf("Please, write port:\n"); fflush(stdout);
		memset(input_buf, 0, sizeof(input_buf));
		fgets(input_buf, sizeof(input_buf), stdin);
		input_buf[strlen(input_buf) - 1] = '\0';
		sockaddr.sin_port = htons(atoi(input_buf));

		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock == -1) {
			printf("ERROR: Can't create socket! Try again.\n"); fflush(stdout);
			continue;
		}
		
		int rc = connect(sock, (struct sockaddr*) &sockaddr, sizeof(sockaddr));
		if(rc == -1){
			printf("ERROR: Can't connet to server! Try again.\n"); fflush(stdout);
			continue;
		} else {
			printf("Connected.\n"); fflush(stdout);
			break;
		}
	}

	char msg[SIZE_MSG] = {0};
	printf("Input (\'-help\' to help): \n"); fflush(stdout);
	for(;;){
		fgets(input_buf, sizeof(input_buf), stdin);
		input_buf[strlen(input_buf) - 1] = '\0';

		if(!strcmp("-help", input_buf)){
			printf("HELP:\n");
			printf("\'-quit\' to shutdown;\n");
			fflush(stdout);
		} else if(!strcmp("-quit", input_buf)) {
			shutdown(sock, 2);
			break;
		} else {
			send(sock, input_buf, sizeof(input_buf), 0);
			
			if (readN(sock, msg) <= 0) {
				printf("Problems :(.\n"); fflush(stdout);
				close(sock);
				break;
			} else {
				printf("Message to SERVER: %s\n", msg); fflush(stdout);
			}

			memset(input_buf, 0, 1000);
		}
	}

	printf("ENDED CLIENT!\n"); fflush(stdout);
	
	return 0;
}

int readN(int socket, char* buf){
	int result = 0;
	int readedBytes = 0;
	int sizeMsg = SIZE_MSG;
	while(sizeMsg > 0){
		readedBytes = recv(socket, buf + result, sizeMsg, 0);
		if (readedBytes <= 0){
			return -1;
		}
		result += readedBytes;
		sizeMsg -= readedBytes;
	}
	return result;
}