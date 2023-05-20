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

#define NUM_FLOWERS 40
#define HEIGHT 5// Длина клумбы
#define WIDTH 8 // Ширина клумбы
#define FLOWERS_SLEEP 3
#define GARDENER_SLEEP 1
#define CAN_MIN_VOLUME 5  // Минимальный объём лейки для полива (некоторые цветы точно будут умирать)
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
    bool is_started;
} Garden;


void printGarden(Garden *garden) {
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
