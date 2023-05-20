// Множество независимых процессов взаимодействуют с использованием
// именованных POSIX семафоров. Обмен данными ведется через
// разделяемую память в стандарте POSIX.
#include "data.c"


sem_t *sem_id;
int buf_id;
Garden *garden;

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

    printf("FLOWER PROCESS. Many independent processes interact using named POSIX semaphores.\n");

    // Работа с семафором
    // ! Создаем именованнный семафор для синхронизации процессов !
    {
        // Проверка: Удалился ли семафор в прошлый раз
        if (sem_unlink(SEM_NAME) != -1) {
            perror("sem_unlink");
        }

        sem_id = sem_open(SEM_NAME, O_CREAT, 0666, 0);
        if (sem_id == 0) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }
    }

    // Работа с памятью
    {
        // Проверка: Удалилась ли память в прошлый раз
        if (shm_unlink(SHM_NAME) != -1) {
            perror("shm_unlink");
        }
        // Создаем разделяемую память
        buf_id = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
        if (buf_id == -1) {
            perror("shm_open error");
            exit(EXIT_FAILURE);
        }
        if (ftruncate(buf_id, sizeof(Garden)) == -1) {
            perror("ftruncate error");
            exit(EXIT_FAILURE);
        } else {
            printf("Memory size set = %lu\n", sizeof(Garden));
        }

        // Подключение к разделяемой памяти
        garden = mmap(NULL, sizeof(Garden), PROT_READ | PROT_WRITE, MAP_SHARED, buf_id, 0);
        if (garden == (Garden *) -1) {
            perror("mmap garden");
            exit(EXIT_FAILURE);
        }
        garden->all_days_count = false;
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
    garden->is_started = true;

    // Запускаем процесс Flowers
    printf("Flowers started\n");
    printf("W - WATERED\nF - FADED\nD - DEAD\n");

    printf("Flowers are waiting for Gardeners\n");
    sem_wait(sem_id);

    // Код процесса отслеживания состояния цветов на клумбе
    for (int day = 1; day <= garden->all_days_count + 1; ++day) {
        if (day > 1) {
            // Ждем, пока оба садовника освободят семафор
            printf("Flowers at night (after Watering):\n");
            printGarden(garden);
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
        printGarden(garden);

        // Отпускаем семафор, чтобы позволить садовникам начать полив
        sem_post(sem_id);

        // Ждем, пока не истечет время до начала увядания цветов
        sleep(FLOWERS_SLEEP);
    }

    printf("\n!ARMAGEDDON FOR FLOWERS STARTED! End of the program...\n");
    printf("Last day:\n");
    printGarden(garden);

    // Удаляем семафор и разделяемую память
    sem_unlink(SEM_NAME);
    sem_close(sem_id);
    shm_unlink(SHM_NAME);
    munmap(garden, sizeof(Garden));
    close(buf_id);

    return 0;
}