#include <fcntl.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/mman.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>


#define NUM_FLOWERS 40
#define GARDENER_POWER 20
#define GARDENER_SLEEP 9

#define PORT 8080


void sigfunc(int sig) {
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }

    printf("Sig finished\n");
    exit(10);
}

int main(int argc, char const *argv[]) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    int server_answer_1 = 0;
    int server_answer_2 = 0;

    srand(time(NULL));

    // Получаем значения m, n, h с помощью аргументов командной строки
    //    if (argc != 4) {
    //        printf("Usage: %s <m> <n> <h>\n", argv[0]);
    //        return -1;
    //    }

    //    m = atoi(argv[1]);
    //    h = atoi(argv[3]);

    // Создаем TCP сокет
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n Socket creation error \n");
        return -1;
    }

    memset(&serv_addr, '0', sizeof(serv_addr));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Устанавливаем адрес сервера
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Подключаемся к серверу
    if (connect(sock, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed \n");
        return -1;
    }

    int gardener_id = rand() % 99 + 1;
    send(sock, &gardener_id, sizeof(int), 0);

    bool server_is_connected = true;

    while (server_is_connected) {
        printf("Gardener №%d woke up.\n", gardener_id);

        for (int i = 0; i < GARDENER_POWER; ++i) {
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

        sleep(GARDENER_SLEEP);
    }

    return 0;
}