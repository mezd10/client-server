#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <iostream>
#include <list>
#include <sstream>
#include "requests.cpp"

#define SIZE_MSG 1000
#define BACKLOG 2

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

struct  thread_info {
    pthread_t threadId;
    int socket;
    int number;
} *clients;
int client_quantity = 0;


void* init(int args);
void* help(int listener, pthread_t listener_thread);
void* connections(void* args);
int readN(int socket, char* buf);
void* kick(int kickNum);
std::string listCommand();
void* client_handler(void* args);

int main(int argc, char** argv) {
    connect();
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
            printf("Clients:\n ");

            fflush(stdout);
            pthread_mutex_lock(&mutex);
            for(int i = 0; i < client_quantity; i++){
                if(clients[i].socket != -1)
                    printf(" NUMBER: %d\t\n", clients[i].number);
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


std::string listCommand() {
    std::string message;
    message += "exit - выход\n";
    message += "signUp (Ваше имя) (Ваше Фамилия) XXXX (роль) [PROGRAMMER, TESTER] - создать нового пользователя\n";
    message += "signIn (Ваше имя) (Ваше фамилия) XXXX - вход\n";
    message += "listMy - список ваших задач для выполнения\n";
    message += "listAll - список всех задач\n";
    message += "listOpen - список всех открытых задач\n";
    message += "update (ID задачи) NewState(состояние) [OPEN, CLOSE, DOING] - обновить состояние проекта\n";
    message += "insert IDResponsible(ID на кого данная задача) (название проекта) (описание) - добавить новый проекта\n";
    return message;
}

std::list<std::string> split(const std::string &s, char delimiter) {
    std::list<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}


void* client_handler(void* args) {
    pthread_mutex_lock(&mutex);
    int index = *((int*)args);
    int sock = clients[index].socket;

    pthread_mutex_unlock(&mutex);

    socklen_t len;
    struct sockaddr_storage addr{};
    char ipstr[INET6_ADDRSTRLEN];
    int port;
    len = sizeof addr;
    getpeername(sock, (struct sockaddr *) &addr, &len);
    auto *s = (struct sockaddr_in *) &addr;
    port = ntohs(s->sin_port);
    inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);
    std::string hello = "Peer IP address: ";
    hello += ipstr;
    hello += ":";
    hello += std::to_string(port);
    hello += "\n";
    hello += listCommand();
    send(sock, hello.c_str(), 1024, 0);

    std::string name;
    std::string last_name;
    std::string position;

    while (true) {
        std::string message;
        char buf[1024];
        int bytes_read;


        bytes_read = read(sock, buf, 1024);
        if (bytes_read <= 0) {
            break;
        }

        std::cout << buf << "\n";
        if (strcmp(buf, std::string("help").c_str()) == 0) {
            message = listCommand();
        } else if (strcmp(buf, std::string("exit").c_str()) == 0) {
            message = "Good bye";
        } else {
            std::list<std::string> listSplit = split(buf, ' ');
            std::string com = listSplit.front();

            if (strcmp(com.c_str(), std::string("signUp").c_str()) == 0) {
                if (listSplit.size() < 4) {
                    message = listCommand();
                } else {
                    listSplit.pop_front();
                    name = listSplit.front();
                    listSplit.pop_front();
                    last_name = listSplit.front();
                    listSplit.pop_front();
                    position = listSplit.front();
                    message = insertEmploy(name, last_name, position);
                }
            } else if (strcmp(com.c_str(), std::string("signIn").c_str()) == 0) {
                if (listSplit.size() < 3) {
                    message = listCommand();
                } else {
                    listSplit.pop_front();
                    name = listSplit.front();
                    listSplit.pop_front();
                    last_name = listSplit.front();
                    position = selectRoleByName(name, last_name);
                    if (position.length() > 0) {
                        message = "Hello ";
                        message += name;
                        message += " you are ";
                        message += position;
                    } else {
                        message = "Sorry, but user with name ";
                        message += name;
                        message += " could'n find...";
                    }
                }
            } else if (position.length() == 0) {
                message = "You are not signIn";
            } else if (strcmp(buf, std::string("listMy").c_str()) == 0) {
                if (strcmp(position.c_str(), std::string("TESTER").c_str()) == 0) {
                    int i = 1;
                    message = selectMyTasks(name, last_name,  i);
                } else {
                    int i = 0;
                    message = selectMyTasks(name, last_name, i);
                }
            } else if (strcmp(buf, std::string("listAll").c_str()) == 0) {
                message = selectAllTasks();
            } else if (strcmp(buf, std::string("listOpen").c_str()) == 0) {
                message = selectOpenTasks();
            } else if (strcmp(com.c_str(), std::string("update").c_str()) == 0) {
                if (listSplit.size() < 2) {
                    message = listCommand();
                } else {
                    listSplit.pop_front();
                    std::string id = listSplit.front();
                    listSplit.pop_front();
                    std::string status = listSplit.front();
                    if (strcmp(position.c_str(), std::string("TESTER").c_str()) == 0) {
                        message = updateTasks(id, status);
                    } else if (strcmp(position.c_str(), std::string("PROGRAMMER").c_str()) == 0) {
                        if (listSplit.front() == "CLOSE") {
                            message = "Sorry, but we can not do it, because TESTER can close only";
                        } else {
                            message = updateTasks(id, status);
                        }
                    } else {
                        message = "Sorry, but we can not do it";
                    }
                }
            } else if (strcmp(listSplit.front().c_str(), std::string("insert").c_str()) == 0) {

                    listSplit.pop_front();

                    if (listSplit.size() < 2) {
                        message = listCommand();
                    } else {
                        std::string id_res = listSplit.front();
                        listSplit.pop_front();
                        std::string name_pro = listSplit.front();
                        listSplit.pop_front();
                        std::string comment = listSplit.front();
                        message = insertTasks(name, last_name, id_res, name_pro, "OPEN", comment);

                }
            } else {
                message = listCommand();
            }
        }

        send(sock, message.c_str(), 1024, 0);

    }
    shutdown(sock, 2);
    close(sock);
    clients[index].socket = -1;
    printf("ENDED CLIENT №%d!\n", index); fflush(stdout);
}




void* connections(void* args){
    int listener = *((int*) args);

    int s;
    int index_client;
    for(;;){

        s = accept(listener, nullptr , nullptr);
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
        clients[client_quantity].number = client_quantity;
        index_client = client_quantity;
        if(pthread_create(&(clients[client_quantity].threadId), NULL, client_handler, (void *) &index_client)) {
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

