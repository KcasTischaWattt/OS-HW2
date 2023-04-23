#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/mman.h>

int N = 10; // количество дикарей
int M = 5; // вместимость горшка
int curr = 5; // текущее количество мяса в горшке

typedef struct {
    sem_t pot; // семафор для горшка
    sem_t cook; // семафор для повара
    int curr;
} shared_data;

int main() {

    const char* shm_name = "/savage_cook_shared_mem";
    const char* pot_sem_name = "/savage_cook_pot_sem";
    const char* cook_sem_name = "/savage_cook_cook_sem";

    int fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
    ftruncate(fd, sizeof(shared_data));
    shared_data* data = mmap(NULL, sizeof(shared_data), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    sem_t* pot = sem_open(pot_sem_name, O_CREAT, 0666, 1);
    sem_t* cook = sem_open(cook_sem_name, O_CREAT, 0666, 0);

    // cook process
    while(1) {
        sem_wait(pot); // ждем, пока дикари освободят горшок

        if(data->curr == 0) { // если горшок пустой, наполняем его
            printf("Cook is filling the pot.\n");
            data->curr = M;
        }

        sem_post(cook); // будим одного из дикарей, который уже ожидает

        sem_post(pot); // освобождаем горшок

        sleep(1); // ждем некоторое время, чтобы дать дикарям время поесть
    }

    return 0;
}
