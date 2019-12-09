#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <cstring>

std::string message;
char buf[1024];

void Event() {
    char input_buf[1000];
    int sock;
    struct sockaddr_in addr{};

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        exit(1);
    }

    printf("Please, write IP address:\n"); fflush(stdout);
    memset(input_buf, 0, sizeof(input_buf));
    fgets(input_buf, sizeof(input_buf), stdin);
    input_buf[strlen(input_buf) - 1] = '\0';

    addr.sin_family = AF_INET;
    addr.sin_port = htons(atoi(input_buf)); // или любой другой порт...
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (connect(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("connect");
        exit(2);
    }
    read(sock, buf, 1024);
    std::cout << buf << "\n";
    int bytes_read;
    bool isWork = true;
    while (isWork) {
        std::getline(std::cin, message);
        send(sock, message.c_str(), 1024, 0);

        bytes_read = read(sock, buf, 1024);
        std::cout << buf << "\n";
        if (strcmp(message.c_str(), std::string("exit").c_str()) == 0) {
            isWork = false;
        }
    }
    shutdown(sock, 2);
    close(sock);
}

int main(int argc, char *argv[]) {
    Event();
    return 0;
}