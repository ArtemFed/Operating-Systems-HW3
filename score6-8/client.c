#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/time.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define NUM_FLOWERS 20
#define GARDENER_SLEEP 1


int main(int argc, char const *argv[]) {
    unsigned short server_port;
    const char *server_ip;

    int sock = 0;
    struct sockaddr_in server_addr;
    int server_answer_1 = 0;
    int server_answer_2 = 0;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    int milliseconds = tv.tv_usec / 1000;
    srand(milliseconds);

    // Получаем значения port и server IP с помощью аргументов командной строки
    if (argc != 3) {
        printf("Args: <port> <SERVER_IP>\n");
        return -1;
    }

    server_port = atoi(argv[1]);
    server_ip = argv[2];

    // Создаем TCP сокет
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);

    // Устанавливаем адрес сервера
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Подключаемся к серверу
    if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    char type = 'g';
    send(sock, &type, sizeof(char), 0);

    int gardener_id = rand() % 90 + 10;
    send(sock, &gardener_id, sizeof(int), 0);

    bool server_is_connected = true;

    while (server_is_connected) {
        server_answer_1 = 0;
        server_answer_2 = 0;
        printf("Gardener №%d woke up.\n", gardener_id);

        for (int i = 0; i < NUM_FLOWERS / 2; ++i) {
            if (i % 3 == 0) {
                sleep(GARDENER_SLEEP);
            }
            // Отправляем значения index на сервер
            int index = rand() % NUM_FLOWERS;
            printf("Gardener №%d: Choose flower №%d\n", gardener_id, index);

            send(sock, &index, sizeof(int), 0);

            read(sock, &server_answer_1, sizeof(server_answer_1));
            if (server_answer_1 == 0) {
                printf("Gardener №%d: WATERED flower №%d\n", gardener_id, index);
            } else if (server_answer_1 == 1) {
                printf("Gardener №%d: already DEAD flower №%d\n", gardener_id, index);
            } else if (server_answer_1 == 2) {
                printf("Gardener №%d: already WATERED flower №%d\n", gardener_id, index);
            } else {
                server_is_connected = false;
                break;
            }
        }

        // Получаем ответ от сервера
        read(sock, &server_answer_2, sizeof(server_answer_2));
        printf("Gardener №%d has ended his work. Gardener watered %d flowers\n", gardener_id, server_answer_2);

        sleep(GARDENER_SLEEP * 4);
    }

    return 0;
}