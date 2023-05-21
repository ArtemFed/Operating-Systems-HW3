#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#include <signal.h>
#include <sys/types.h>
#include <stdbool.h>

#define NUM_FLOWERS 20
#define HEIGHT 4 // Длина клумбы
#define WIDTH 5 // Ширина клумбы

#define FLOWERS_SLEEP 10
#define GARDENER_SLEEP 1


// Состояние цветка
typedef enum {
    WATERED,// Политый
    FADED,  // Увядающий
    DEAD    // Мертвый
} FlowerState;

// Структура, хранящая информацию о состоянии клумбы и днях
typedef struct {
    FlowerState flowers[NUM_FLOWERS];
    int all_days_count;
    int cur_day;
    bool is_started;
} Garden;


Garden *garden;

void sigfunc(int sig) {
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }

    printf("Sig finished\n");
    exit(10);
}

char *printGarden(Garden *garden_for_print) {
    int count_of_watered = 0;
    int count_of_faded = 0;
    int count_of_dead = 0;
    char *answer = (char *) malloc(HEIGHT * WIDTH * 2 + 200); // выделяем память под строку ответа
    char *temp_answer = (char *) malloc(10); // временная строка для форматирования чисел
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (garden_for_print->flowers[i * HEIGHT + j] == WATERED) {
                count_of_watered++;
                strcat(answer, "W ");
            } else if (garden_for_print->flowers[i * HEIGHT + j] == FADED) {
                count_of_faded++;
                strcat(answer, "F ");
            } else {
                count_of_dead++;
                strcat(answer, "D ");
            }
        }
        strcat(answer, "\n");
    }
    sprintf(temp_answer, "%d", count_of_watered); // форматирование чисел в строку
    strcat(answer, "Count of Watered: ");
    strcat(answer, temp_answer);
    strcat(answer, "\n");
    sprintf(temp_answer, "%d", count_of_faded);
    strcat(answer, "Count of Faded: ");
    strcat(answer, temp_answer);
    strcat(answer, "\n");
    sprintf(temp_answer, "%d", count_of_dead);
    strcat(answer, "Count of Dead: ");
    strcat(answer, temp_answer);
    strcat(answer, "\n");
    return answer;
}


// Функция потока для обработки клиентского подключения
void *connectionHandler(void *socket_desc) {
    int new_socket = *(int *) socket_desc;
    char client_type;
    int client_id = 0;
    int index;
    int answer;

    char buffer[1024] = {0};

    // Получаем значения indexes (номер цветка) от клиента
    read(new_socket, &client_type, sizeof(client_type));

    // Получаем значения indexes (номер цветка) от клиента
    read(new_socket, &client_id, sizeof(client_id));

    if (client_type == 'g') {
        printf("Gardener №%d connected\n", client_id);

        while (garden->is_started) {
            sleep(1);

            printf("Day №%d. Gardener №%d started working.\n", garden->cur_day, client_id);

            int count_of_watered_flowers = 0;
            for (int i = 0; i < NUM_FLOWERS / 2; i++) {
                read(new_socket, &index, sizeof(int));

                answer = 0;

                // Пробуем полить цветок под index
                if (garden->flowers[index] == DEAD) {
                    // Цветок уже умер... RIP
                    printf("Gardener №%d: Flower №%d already DEAD\n", client_id, index);
                    answer = 1;
                } else if (garden->flowers[index] == WATERED) {
                    // Цветок уже был полит
                    printf("Gardener №%d: Flower №%d already WATERED\n", client_id, index);
                    answer = 2;
                } else {
                    // Поливаем цветок под index
                    garden->flowers[index] = WATERED;
                    printf("Gardener №%d: Flower №%d was WATERED\n", client_id, index);
                    count_of_watered_flowers++;
                }

                if (!garden->is_started) {
                    answer = -1;
                }
                send(new_socket, &answer, sizeof(int), 0);
            }

            sleep(1);

            send(new_socket, &count_of_watered_flowers, sizeof(int), 0);
            printf(
                    "Gardener №%d is going to bed. Today He watered: %d flowers\n",
                    client_id,
                    count_of_watered_flowers
            );
        }

        printf("Gardener №%d disconnected\n", client_id);
    } else {
        printf("Viewer №%d connected\n", client_id);

        while (garden->is_started) {
            printf("Send Info to viewer %d\n", client_id);
            sprintf((char *) &buffer, "%s", printGarden(garden));
            send(new_socket, &buffer, strlen(buffer), 0);
            sleep(5);
        }
    }

    // Закрываем сокет для клиента
    close(new_socket);

    // Освобождаем память выделенную для сокета
    free(socket_desc);
}

int currentCountOfClients = 0;

void *daysProcess() {
    // Код процесса отслеживания состояния цветов на клумбе
    for (int day = 1; day <= garden->all_days_count + 1; ++day) {
        garden->cur_day = day;

        /* Устанавливаем цветам состояния:
         * Политым - явядающие
         * Увядающим - Мёртвые */
        for (int i = 0; i < NUM_FLOWERS; i++) {
            if (garden->flowers[i] == WATERED) {
                garden->flowers[i] = FADED;
            } else if (garden->flowers[i] == FADED) {
                garden->flowers[i] = DEAD;
            }
        }

        if (garden->cur_day > garden->all_days_count) {
            printf("Flowers finished\n");
            break;
        }

        printf(
                "--------------------------------------------------"
                "\nDay: %d / %d started\n",
                day,
                garden->all_days_count
        );

        printf("%s", printGarden(garden));

        // Ждем, пока не истечет время до начала увядания цветов
        sleep(FLOWERS_SLEEP);

        printf("Flowers at night (after Watering):\n");
        printf("%s", printGarden(garden));
    }
    garden->is_started = false;

    printf("\n!ARMAGEDDON ARE STARTING!\n");

    sleep(2);
    printf("Last day:\n");
    printf("%s", printGarden(garden));

    exit(0);
}

int main(int argc, char const *argv[]) {
//    signal(SIGINT, sigfunc);
//    signal(SIGTERM, sigfunc);

    unsigned short server_port;
    int all_days_count = 0;
    // Чтение входных данных
    {
        server_port = 8080;
        all_days_count = 3;
        srand(42);
        if (argc >= 2) {
            // Порт сервера
            server_port = atoi(argv[1]);
        }
        if (argc >= 3) {
            // Сколько дней отобразить
            all_days_count = atoi(argv[2]);
        }
        if (argc >= 4) {
            // Рандомизированный ввод
            srand(atoi(argv[3]));
        }
    }

    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    pthread_t thread_id;

    printf("FLOWER SERVER\n");

    // Создаем TCP сокет
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Устанавливаем опции для сокета
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    memset(&address, 0, sizeof(address));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(server_port);

    // Привязываем сокет к адресу и порту
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Слушаем входящие подключения
    if (listen(server_fd, 2) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    garden = malloc(sizeof(Garden));

    garden->all_days_count = all_days_count;

    // Инициализируем состояние клумбы
    for (int i = 0; i < NUM_FLOWERS; i++) {
        garden->flowers[i] = WATERED;
    }
    garden->is_started = true;

    // Запускаем процесс Flowers
    printf("Flowers started\n");
    printf("W - WATERED\nF - FADED\nD - DEAD\n");

    printf("Flowers are waiting for Gardeners...\n");

    if (pthread_create(&thread_id, NULL, daysProcess, &garden->all_days_count) > 0) {
        perror("could not create thread");
        return 1;
    }

    // Обрабатываем каждое новое подключение в отдельном потоке
    while ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen))) {
        currentCountOfClients++;

        // Создаем новый сокет для клиента
        int *socket_desc = malloc(1);
        *socket_desc = new_socket;

        // Создаем поток для обработки клиентского подключения
        if (pthread_create(&thread_id, NULL, connectionHandler, (void *) socket_desc) > 0) {
            perror("could not create thread");
            return 1;
        }
    }

    return 0;
} 
