// Множество процессов взаимодействуют с использованием семафоров в стандарте UNIX SYSTEM V.
// Обмен данными ведется через разделяемую память в стандарте UNIX SYSTEM V
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

#define NUM_FLOWERS 40
#define HEIGHT 5// Длина клумбы
#define WIDTH 8 // Ширина клумбы
#define FLOWERS_SLEEP 3
#define GARDENER_SLEEP 1
#define CAN_MIN_VOLUME 10  // Минимальный объём лейки для полива (некоторые цветы точно будут умирать)
#define CAN_RANDOM_VOLUME 5// Random max

#define SEM_NAME "/garden_semaphore"// Имя семафора
#define SHM_NAME "/garden"          // Имя памяти

#define KEY 1234// Ключ для доступа к ресурсам

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
} Garden;

int sem_id;
int buf_id;
Garden *garden;

void printGarden() {
    int count_of_watered = 0;
    int count_of_faded = 0;
    int count_of_dead = 0;
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (garden->flowers[i * HEIGHT + j] == WATERED) {
                count_of_watered++;
                printf("W ");
            } else if (garden->flowers[i * HEIGHT + j] == FADED) {
                count_of_faded++;
                printf("F ");
            } else {
                count_of_dead++;
                printf("D ");
            }
        }
        printf("\n");
    }
    printf("Count of Watered: %d\nCount of Faded: %d\nCount of Dead: %d\n",
           count_of_watered, count_of_faded, count_of_dead);
}

void sigfunc(int sig) {
    if (sig != SIGINT && sig != SIGTERM) {
        return;
    }

    // Закрывает свой семафор
    semctl(sem_id, 1, IPC_RMID);
    shmctl(buf_id, IPC_RMID, NULL);

    printf("Sig finished\n");
    exit(10);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, sigfunc);
    signal(SIGTERM, sigfunc);

    printf("Many processes interact using semaphores in the UNIX SYSTEM V standard. "
           "Data is exchanged via shared memory in the UNIX SYSTEM V standard.\n");

    // Операция для захвата семафора
    struct sembuf acquire = {0, -1, SEM_UNDO};

    // Операция для освобождения семафора
    struct sembuf release = {0, 1, SEM_UNDO};

    pid_t pid;
    pid_t pid_gardeners;

    // Создаем семафор
    if ((sem_id = semget(KEY, 1, IPC_CREAT | 0666)) == -1) {
        perror("Ошибка при создании семафора");
        exit(1);
    }

    // Инициализируем семафор значением 1
    if (semctl(sem_id, 0, SETVAL, 1) == -1) {
        perror("Ошибка при инициализации семафора");
        exit(1);
    }

    // Создаю разделяемую память
    if ((buf_id = shmget(KEY, sizeof(Garden), IPC_CREAT | 0666)) == -1) {
        perror("Ошибка при создании разделяемой памяти");
        exit(1);
    } else {
        printf("Object is open: id = 0x%x\n", buf_id);
    }

    if ((garden = shmat(buf_id, 0, 0)) == (Garden *) -1) {
        perror("Ошибка при присоединении к разделяемой памяти");
        exit(1);
    }

    // Инициализируем состояние клумбы
    for (int i = 0; i < NUM_FLOWERS; i++) {
        garden->flowers[i] = WATERED;
    }

    // Чтение входных данных
    garden->all_days_count = 3;
    srand(42);
    if (argc >= 2) {
        // Сколько дней отобразить
        garden->all_days_count = atoi(argv[1]);
    }
    if (argc >= 3) {
        // Рандомизированный ввод
        srand(atoi(argv[2]));
    }

    // Создаем все процессы
    pid = fork();
    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid != 0) {
        printf("Flowers started\n");

        printf("W - WATERED\nF - FADED\nD - DEAD\n");
        // Код процесса отслеживания состояния цветов на клумбе
        for (int day = 1; day <= garden->all_days_count + 1; ++day) {
            if (day > 1) {
                // Ждем, пока оба садовника освободят семафор
                printf("Flowers at night (after Watering):\n");
                printGarden();
                semop(sem_id, &acquire, 1);
            }

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
                semop(sem_id, &release, 1);
                break;
            }

            printf("--------------------------------------------------"
                   "\nDay: %d started\n",
                   day);

            printf("Flowers at day (before Watering):\n");
            printGarden();

            // Отпускаем семафор, чтобы позволить садовникам начать полив
            semop(sem_id, &release, 1);

            // Ждем, пока не истечет время до начала увядания цветов
            sleep(FLOWERS_SLEEP);
        }

        int status;
        waitpid(pid, &status, 0);

    } else {
        // Создаем процессы садовников
        pid_gardeners = fork();
        if (pid_gardeners == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid_gardeners != 0) {
            int gardener_num = 1;
            printf("Gardener=%d started\n", gardener_num);

            // Код садовника 1
            while (garden->cur_day <= garden->all_days_count) {
                // Садовник 1 спит ночью (цветы вянут)
                sleep(GARDENER_SLEEP);
                // Садовник 1 начинает полив
                int water_volume = CAN_MIN_VOLUME + rand() % CAN_RANDOM_VOLUME;
                for (int checks = 0; water_volume > 0 && checks < NUM_FLOWERS; checks++) {
                    int ind = rand() % NUM_FLOWERS;

                    // Садовник подходит к цветку, бликирует другого садовника и поливает, если необходимо.
                    semop(sem_id, &acquire, 1);
                    if (garden->flowers[ind] == FADED) {
                        garden->flowers[ind] = WATERED;
                        water_volume--;
                    }
                    // Отпускаем семафор
                    semop(sem_id, &release, 1);
                }
            }
            printf("Gardener=%d finished\n", gardener_num);
            exit(0);
        } else {
            int gardener_num = 2;
            printf("Gardener=%d started\n", gardener_num);

            // Код садовника 2
            while (garden->cur_day <= garden->all_days_count) {
                // Садовник 2 спит ночью (цветы вянут)
                sleep(GARDENER_SLEEP);
                // Садовник 2 начинает полив
                int water_volume = CAN_MIN_VOLUME + rand() % CAN_RANDOM_VOLUME;
                for (int checks = 0; water_volume > 0 && checks < NUM_FLOWERS; checks++) {
                    int ind = rand() % NUM_FLOWERS;

                    // Садовник подходит к цветку, бликирует другого садовника и поливает, если необходимо.
                    semop(sem_id, &acquire, 1);
                    if (garden->flowers[ind] == FADED) {
                        garden->flowers[ind] = WATERED;
                        water_volume--;
                    }
                    // Отпускаем семафор
                    semop(sem_id, &release, 1);
                }
            }
            printf("Gardener=%d finished\n", gardener_num);
            exit(0);
        }
    }

    printf("\n!ARMAGEDDON STARTED! End of the program...\n");
    printf("Last day:\n");
    printGarden();

    // Удаляем семафор и разделяемую память
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("semctl sem_id");
        exit(1);
    }

    if (shmctl(buf_id, IPC_RMID, NULL) == -1) {
        perror("shmctl buf_id");
        exit(1);
    }

    return 0;
}