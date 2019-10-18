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
#define BACKLOG 2

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct  thread_info {
    pthread_t threadId;
    int socket;
    char* address;
    int port;
    int number;
} *clients;
int client_quantity = 0;

void* init(int args);
void* help(int listener, pthread_t listener_thread);
void* connections(void* args);
int readN(int socket, char* buf);
void* kick(int kickNum);

int main(int argc, char** argv) {
    int listener = socket(AF_INET, SOCK_STREAM, 0 ); 
    init(listener);
	
    pthread_t listener_thread;
    if (pthread_create(&listener_thread, NULL, connections, (void*) &listener)){
        printf("ERROR: Can't create listener thread!"); fflush(stdout);
        exit(1);
    }

	help(listener,  listener_thread);

    return 0;
}

void* init(int args) {
	char input_buf[SIZE_MSG];
    int listener = args;

    if ( listener < 0 ) {
		perror( "Can't create socket to listen: ");
		exit(1);
	}
	printf("Socket create.\n");
	fflush(stdout);

	struct sockaddr_in listener_info;
	listener_info.sin_family = AF_INET;
	

	printf("Please, write IP address:\n"); fflush(stdout);
	memset(input_buf, 0, sizeof(input_buf));
	fgets(input_buf, sizeof(input_buf), stdin);
	input_buf[strlen(input_buf) - 1] = '\0';
	listener_info.sin_addr.s_addr = inet_addr(input_buf);
	printf("Please, write port:\n"); fflush(stdout);
	memset(input_buf, 0, sizeof(input_buf));
	fgets(input_buf, sizeof(input_buf), stdin);
	input_buf[strlen(input_buf) - 1] = '\0';
	listener_info.sin_port = htons(atoi(input_buf));

    
    int resBind = bind(listener, (struct sockaddr *)&listener_info, sizeof(listener_info));
	if (resBind < 0 ) {
		perror( "Can't bind socket" );
		exit(1);
	}
	printf("Socket bind.\n");
	fflush(stdout);
		
	if ( listen(listener, BACKLOG) ) { 
		perror("Error while listening: ");
		exit(1);
	}
	printf("Listen input connection...\n");
	fflush(stdout);
}

void* help(int listener, pthread_t listener_thread) {

	

	printf("Input (-help to help): \n"); fflush(stdout);
    char buf[1000];
	for(;;) {
		bzero(buf, 1000);
		fgets(buf, 1000, stdin);
		buf[strlen(buf) - 1] = '\0';
		
		if(!strcmp("-help", buf)){
			printf("HELP:\n");
			printf("\'-ls_clients\' to list users online;\n");
			printf("\'-kick [number client]\' to kick client from server;\n");
			printf("\'-close\' to shutdown;\n");
			fflush(stdout);
		} else if(!strcmp("-ls_clients", buf)){
				printf("Clients:\n NUMBER  \tADDRESS  \tPORT\n");
				
				fflush(stdout);
				pthread_mutex_lock(&mutex);
				for(int i = 0; i < client_quantity; i++){
					if(clients[i].socket != -1)
						printf("  %d\t\t%s\t%d\n", clients[i].number, clients[i].address, clients[i].port);
				}
				pthread_mutex_unlock(&mutex);

				fflush(stdout);
		} else if(!strcmp("-close", buf)) {
				shutdown(listener, 2);
				close(listener);
				pthread_join(listener_thread, NULL);
				break;
		} else {
			char *sep = " ";
			char *str = strtok(buf, sep);
			if(str == NULL) {
				printf("Illegal format!\n"); fflush(stdout);
				continue;
			}
			if(!strcmp("-kick", str)){
				str = strtok(NULL, sep);
				int kickNum = atoi(str);
				if(str[0] != '0' && kickNum == 0){
					printf("Illegal format!\n"); fflush(stdout);
					continue;
				}
				kick(kickNum);
			}else
			{
				printf("Illegal format! Please, write \"-help\" for help\n"); fflush(stdout);

			}
			
		}
	}

	printf("ENDED SERVER!\n"); fflush(stdout);

}

void* client_handler(void* args){
	
	pthread_mutex_lock(&mutex);
	int index = *((int*)args);
	int sock = clients[index].socket;
	pthread_mutex_unlock(&mutex);

	char msg[SIZE_MSG] = {0};
	for(;;) {
		if (readN(sock, msg) <= 0) {
			printf("Client №%d disconnect\n", index); fflush(stdout);
			clients[index].socket = -1;
			break;
		} else {
			printf("Get message from client №%d: %s\n", index, msg); fflush(stdout);
			send(sock, msg, sizeof(msg), 0);
		}
		memset(msg, 0, sizeof(msg));
	}
	shutdown(sock, 2);
	close(sock);
	printf("ENDED CLIENT №%d!\n", index); fflush(stdout);
}

void* connections(void* args){
    int listener = *((int*) args);
    
    int s;
    int index_client;
    struct sockaddr_in addr;
	int len = sizeof(addr);
    for(;;){

        s = accept(listener, &addr, &len);
        if (s <= 0){
        	printf("STOPING SERVER\n"); fflush(stdout);
			
			pthread_mutex_lock(&mutex);
    		for(int i = 0; i < client_quantity; i++){
        		shutdown(clients[i].socket, 2);
        		close(clients[i].socket);
        		clients[i].socket = -1;
    		}
    		for (int i = 0; i < client_quantity; i++){
    			pthread_join(clients[i].threadId, NULL);
    		}
    		pthread_mutex_unlock(&mutex);

      		break;
        }

        pthread_mutex_lock(&mutex);
        clients = (struct  thread_info*) realloc(clients, sizeof(struct  thread_info) * (client_quantity + 1));
        clients[client_quantity].socket = s;
        clients[client_quantity].address = inet_ntoa(addr.sin_addr);
        clients[client_quantity].port = addr.sin_port;
        clients[client_quantity].number = client_quantity;
        index_client = client_quantity;
        if(pthread_create(&(clients[client_quantity].threadId), NULL, client_handler, (void*) &index_client)) {
            printf("ERROR: Can't create thread for client!\n"); fflush(stdout);
            continue;
        }
        pthread_mutex_unlock(&mutex);
        
        client_quantity++;
    }

    printf("ENDED LISTENER CONNECTIONS!\n"); fflush(stdout);
}

int readN(int socket, char* buf){
	int result = 0;
	int readed_bytes = 0;
	int size_msg = SIZE_MSG;
	while(size_msg > 0){
		readed_bytes = recv(socket, buf + result, size_msg, 0);
		if (readed_bytes <= 0){
			return -1;
		}
		result += readed_bytes;
		size_msg -= readed_bytes;
	}
	return result;
}

void* kick(int kickNum){
	pthread_mutex_lock(&mutex);
	for(int i = 0; i < client_quantity; i++){
		if(clients[i].number == kickNum) {
			shutdown(clients[i].socket, 2);
			close(clients[i].socket);
			clients[i].socket = -1;
			break;
		}
	}
	pthread_mutex_unlock(&mutex);
}