// Множество независимых процессов взаимодействуют с использованием семафоров в стандарте UNIX SYSTEM V. Обмен
// данными ведется через разделяемую память в стандарте UNIX SYSTEM V.
#include "data.c"


int sem_id;
int buf_id;
Garden *garden;

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

    printf("FLOWER PROCESS. Many processes interact using semaphores in the UNIX SYSTEM V standard.\n");

    // Операция для захвата семафора
    struct sembuf acquire = {0, -1, SEM_UNDO};

    // Операция для освобождения семафора
    struct sembuf release = {0, 1, SEM_UNDO};

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
    printf("You have 5 second to start Gardeners\n");
    sleep(5);

    semop(sem_id, &acquire, 1);

    // Код процесса отслеживания состояния цветов на клумбе
    for (int day = 1; day <= garden->all_days_count + 1; ++day) {
        if (day > 1) {
            // Ждем, пока оба садовника освободят семафор
            printf("Flowers at night (after Watering):\n");
            printGarden(garden);
            semop(sem_id, &acquire, 1);
//            sem_wait(sem_id);
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
//            sem_post(sem_id);
            break;
        }

        printf("--------------------------------------------------"
               "\nDay: %d started\n",
               day);

        printf("Flowers at day (before Watering):\n");
        printGarden(garden);

        // Отпускаем семафор, чтобы позволить садовникам начать полив
        semop(sem_id, &release, 1);
//        sem_post(sem_id);

        // Ждем, пока не истечет время до начала увядания цветов
        sleep(FLOWERS_SLEEP);
    }

    printf("\n!ARMAGEDDON FOR FLOWERS STARTED! End of the program...\n");
    printf("Last day:\n");
    printGarden(garden);

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