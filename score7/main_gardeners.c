// Множество процессов взаимодействуют с использованием
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

    printf("GARDENER PROCESS. Many independent processes interact using named POSIX semaphores.\n");

    // Работа с семафором
    // ! Создаем именованнный семафор для синхронизации процессов !
    {
        sem_id = sem_open(SEM_NAME, O_CREAT, 0666, 0);
        if (sem_id == 0) {
            perror("sem_open");
            exit(EXIT_FAILURE);
        }
    }

    // Работа с памятью
    {
        // Создаем разделяемую память
        buf_id = shm_open(SHM_NAME, O_RDWR, 0666);
        if (buf_id == -1) {
            perror("shm_open error");
            exit(EXIT_FAILURE);
        }

        // Подключение к разделяемой памяти
        garden = mmap(NULL, sizeof(Garden), PROT_READ | PROT_WRITE, MAP_SHARED, buf_id, 0);
        if (garden == (Garden *) -1) {
            perror("mmap garden");
            exit(EXIT_FAILURE);
        }
    }

    printf("Gardeners are waiting...\n");
    sem_post(sem_id);

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
                sem_wait(sem_id);
                if (garden->flowers[ind] == FADED) {
                    printf("Gardener=%d watered the flower=%d\n", gardener_num, ind);
                    garden->flowers[ind] = WATERED;
                    water_volume--;
                }
                // Отпускаем семафор
                sem_post(sem_id);
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
                sem_wait(sem_id);
                if (garden->flowers[ind] == FADED) {
                    printf("Gardener=%d watered the flower=%d\n", gardener_num, ind);
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

    printf("\n!ARMAGEDDON FOR GARDENERS STARTED!\n");


    return 0;
}