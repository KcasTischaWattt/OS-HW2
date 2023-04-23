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

    for(int i = 0; i < N; i++) {
        int pid = fork();
        if(pid == -1) {
            perror("fork");
            exit(1);
        } else if(pid == 0) { // child process
            srand(time(NULL) + i);
            int id = i + 1;

            while(1) {
                int sleep_time = rand() % 5 + 1; // генерация случайного времени ожидания от 1 до 5 секунд
                sleep(sleep_time); // ждем это время, прежде чем снова проголодаться
		printf("Savage %d is hungry.\n", id);
                
                sem_wait(pot); // пытаемся получить доступ к горшку
                
                if(data->curr > 0) { // если в горшке еще есть мясо, едим его
                    printf("Savage %d is eating.\n", id);
                    data->curr--;
                } else { // если горшок пустой, будим повара и ждем, пока он его наполнит
                    printf("Pot is empty, waking up the cook.\n");
                    sem_post(cook); // будим повара
                    sem_wait(pot); // ждем, пока повар наполнит горшок
                    printf("Savage %d got some meat from the pot.\n", id);
                    data->curr--; // дикарь берет один кусок мяса из горшка
                }
                
                sem_post(pot); // освобождаем горшок
            }
            exit(0);
        }
    }

    // parent process (cook)
    while(1) {
        printf("Cook is sleeping.\n");
        sem_wait(cook); // ждём пока дикарь не разбудит нас, когда горшок пустой
    printf("Cook is preparing the meat.\n");
    data->curr = M; // добавляем M кусков мяса в горшок

    for(int i = 0; i < N - 1; i++) { // будим N-1 дикарей, которые ожидают еду
        sem_post(pot);
    }

    printf("Cook filled the pot with %d pieces of meat.\n", M);

    sleep(5); // готовим мясо

	}

	return 0;
}
