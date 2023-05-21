#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>

int main(int argc, char const *argv[]) {
    unsigned short server_port;
    const char *server_ip;

    if (argc >= 3) {
        server_port = atoi(argv[1]);
        server_ip = argv[2];
    } else {
        printf("Invalid args");
        exit(1);
    }

    int sockt = 0;
    struct sockaddr_in server_addr;
    char buffer[1024] = {0};

    if ((sockt = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);

    if (connect(sockt, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    char type = 'v';
    send(sockt, &type, sizeof(char), 0);

    int viewer_id = rand() % 90 + 10;
    send(sockt, &viewer_id, sizeof(int), 0);

    printf("Viewer №%d started\n", viewer_id);

    int messages_count = 0;

    while (strcmp(buffer, "finish") != 0) {
        long status = read(sockt, buffer, 1024);
        if (status == 0) {
            break;
        }
        printf("----------------------------------\nMessage №%d\n", ++messages_count);
        printf("Server response: \n%s\n", buffer);
    }

    return 0;
}