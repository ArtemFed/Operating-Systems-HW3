// Множество процессов взаимодействуют с использованием
// именованных POSIX семафоров. Обмен данными ведется через
// разделяемую память в стандарте POSIX.
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>


#define NUM_FLOWERS 40
#define HEIGHT 5// Длина клумбы
#define WIDTH 8 // Ширина клумбы
#define FLOWERS_SLEEP 2
#define GARDENER_SLEEP 1
#define CAN_MIN_VOLUME 10  // Минимальный объём лейки для полива (некоторые цветы точно будут умирать)
#define CAN_RANDOM_VOLUME 5// Random max

#define SEM_NAME "/garden_semaphore"// Имя семафора
#define SHM_NAME "/garden"          // Имя памяти

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

sem_t *sem_id;
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

    // Закрываем всё
    sem_unlink(SEM_NAME);
    sem_close(sem_id);
    shm_unlink(SHM_NAME);
    munmap(garden, sizeof(Garden));
    close(buf_id);

    printf("Sig finished\n");
    exit(10);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, sigfunc);
    signal(SIGTERM, sigfunc);

    printf("Many processes interact using NAMED POSIX semaphores located in shared memory.\n");

    // Работа с памятью
    {
        // Проверка: Удалилась ли память в прошлый раз
        if (shm_unlink(SHM_NAME) != -1) {
            perror("shm_unlink");
        }
        // Создаем разделяемую память
        buf_id = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (buf_id == -1) {
            perror("shm_open");
            exit(EXIT_FAILURE);
        }
        if (ftruncate(buf_id, sizeof(Garden)) == -1) {
            perror("ftruncate");
            exit(EXIT_FAILURE);
        }

        // Подключение к разделяемой памяти
        garden = mmap(NULL, sizeof(Garden), PROT_READ | PROT_WRITE, MAP_SHARED, buf_id, 0);
        if (garden == MAP_FAILED) {
            perror("mmap");
            exit(EXIT_FAILURE);
        }
    }

    // Работа с семафором
    // ! Создаем именованнный семафор для синхронизации процессов !
    {
        // Проверка: Удалился ли семафор в прошлый раз
        if (sem_unlink(SEM_NAME) != -1) {
            perror("sem_unlink");
        }

        sem_id = sem_open(SEM_NAME, O_CREAT | O_EXCL, 0666, 0);
        if (sem_id == SEM_FAILED) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }
    }

    // Чтение входных данных
    {
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
    }

    // Инициализируем состояние клумбы
    for (int i = 0; i < NUM_FLOWERS; i++) {
        garden->flowers[i] = WATERED;
    }

    // Создаем все процессы
    pid_t pid = fork();
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
                sem_wait(sem_id);
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
                sem_post(sem_id);
                break;
            }

            printf("--------------------------------------------------"
                   "\nDay: %d started\n",
                   day);

            printf("Flowers at day (before Watering):\n");
            printGarden();

            // Отпускаем семафор, чтобы позволить садовникам начать полив
            sem_post(sem_id);

            // Ждем, пока не истечет время до начала увядания цветов
            sleep(FLOWERS_SLEEP);
        }

        int status;
        waitpid(pid, &status, 0);

    } else {
        // Создаем процессы садовников
        pid_t pid_gardeners = fork();
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
                for (int checks = 0; water_volume > 0 && checks < NUM_FLOWERS + CAN_RANDOM_VOLUME; checks++) {
                    int ind = rand() % NUM_FLOWERS;

                    // Садовник подходит к цветку, бликирует другого садовника и поливает, если необходимо.
                    sem_wait(sem_id);
                    if (garden->flowers[ind] == FADED) {
                        garden->flowers[ind] = WATERED;
                        water_volume--;
                    }
                    // Отпускаем семафор
                    sem_post(sem_id);
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
                for (int checks = 0; water_volume > 0 && checks < NUM_FLOWERS + CAN_RANDOM_VOLUME; checks++) {
                    int ind = rand() % NUM_FLOWERS;

                    // Садовник подходит к цветку, бликирует другого садовника и поливает, если необходимо.
                    sem_wait(sem_id);
                    if (garden->flowers[ind] == FADED) {
                        garden->flowers[ind] = WATERED;
                        water_volume--;
                    }
                    // Отпускаем семафор
                    sem_post(sem_id);
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
    sem_unlink(SEM_NAME);
    sem_close(sem_id);
    shm_unlink(SHM_NAME);
    munmap(garden, sizeof(Garden));
    close(buf_id);

    return 0;
}