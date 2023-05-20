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

    printf("GARDENER PROCESS. Many processes interact using semaphores in the UNIX SYSTEM V standard.\n");

    // Операция для захвата семафора
    struct sembuf acquire = {0, -1, SEM_UNDO};

    // Операция для освобождения семафора
    struct sembuf release = {0, 1, SEM_UNDO};

    // Создаем семафор
    if ((sem_id = semget(KEY, 0, 0666)) == -1) {
        perror("Ошибка при создании семафора");
        exit(1);
    }

    // Создаю разделяемую память
    if ((buf_id = shmget(KEY, sizeof(Garden),  0666)) == -1) {
        perror("Ошибка при создании разделяемой памяти");
        exit(1);
    } else {
        printf("Object is open: id = 0x%x\n", buf_id);
    }

    if ((garden = shmat(buf_id, 0, 0)) == (Garden *) -1) {
        perror("Ошибка при присоединении к разделяемой памяти");
        exit(1);
    }

    printf("Gardeners are waiting...\n");
//    sem_post(sem_id);
    semop(sem_id, &release, 1);


    while (!garden->is_started) {
        garden = mmap(NULL, sizeof(Garden), PROT_READ | PROT_WRITE, MAP_SHARED, buf_id, 0);
        if (garden == (Garden *) -1) {
            perror("mmap garden");
            exit(EXIT_FAILURE);
        }
        // Пока у садовников нет сада (его строят), они ждут...
        sleep(1);
    }

    printf("Gardeners are ready to start\n");

    // Создаем процессы Gardeners
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
            for (int checks = 0; water_volume > 0 && checks < NUM_FLOWERS - CAN_RANDOM_VOLUME; checks++) {
                int ind = rand() % NUM_FLOWERS;

                // Садовник подходит к цветку, бликирует другого садовника и поливает, если необходимо.
                semop(sem_id, &acquire, 1);
//                sem_wait(sem_id);
                if (garden->flowers[ind] == FADED) {
                    printf("Gardener=%d watered the flower=%d\n", gardener_num, ind);
                    garden->flowers[ind] = WATERED;
                    water_volume--;
                }
                // Отпускаем семафор
                semop(sem_id, &release, 1);
//                sem_post(sem_id);
            }
        }
        printf("Gardener=%d finished\n", gardener_num);

        int status;
        waitpid(pid_gardeners, &status, 0);

    } else {
        int gardener_num = 2;
        printf("Gardener=%d started\n", gardener_num);

        // Код садовника 2
        while (garden->cur_day <= garden->all_days_count) {
            // Садовник 2 спит ночью (цветы вянут)
            sleep(GARDENER_SLEEP);
            // Садовник 2 начинает полив
            int water_volume = CAN_MIN_VOLUME + rand() % CAN_RANDOM_VOLUME;
            for (int checks = 0; water_volume > 0 && checks < NUM_FLOWERS - CAN_RANDOM_VOLUME; checks++) {
                int ind = rand() % NUM_FLOWERS;

                // Садовник подходит к цветку, бликирует другого садовника и поливает, если необходимо.
                semop(sem_id, &acquire, 1);
//                sem_wait(sem_id);
                if (garden->flowers[ind] == FADED) {
                    printf("Gardener=%d watered the flower=%d\n", gardener_num, ind);
                    garden->flowers[ind] = WATERED;
                    water_volume--;
                }
                // Отпускаем семафор
                semop(sem_id, &release, 1);
//                sem_post(sem_id);
            }
        }
        printf("Gardener=%d finished\n", gardener_num);
        exit(0);
    }

    printf("\n!ARMAGEDDON FOR GARDENERS STARTED!\n");


    return 0;
}