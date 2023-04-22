#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

#define SEM_KEY_POT 1234
#define SEM_KEY_COOK 5678

int N = 10; // количество дикарей
int M = 5; // вместимость горшка
int curr = 5; // текущее количество мяса в горшке

int semid_pot, semid_cook;

void* savage(void* arg) {
    int id = *(int*)arg;

    while(1) {
        srand(time(NULL) + id);
        int sleep_time = rand() % 5 + 1; // генерация случайного времени ожидания от 1 до 5 секунд
        sleep(sleep_time); // ждем это время, прежде чем снова проголодаться

        printf("Savage %d is hungry.\n", id);
        
        struct sembuf op[] = {{0, -1, 0}}; // ждем доступа к горшку
        semop(semid_pot, op, 1);
        
        if(curr > 0) { // если в горшке еще есть мясо, едим его
            printf("Savage %d is eating.\n", id);
            curr--;
        } else { // если горшок пустой, будим повара и ждем, пока он его наполнит
            printf("Pot is empty, waking up the cook.\n");
            semctl(semid_cook, 0, SETVAL, 1); // будим повара
            op[0].sem_op = -1;
            semop(semid_pot, op, 1); // ждем, пока повар наполнит горшок
            printf("Savage %d got some meat from the pot.\n", id);
            curr--; // дикарь берет один кусок мяса из горшка
        }
        
        op[0].sem_op = 1;
        semop(semid_pot, op, 1); // освобождаем горшок
    
    }   

    return NULL;
}

void* cook_fn(void* arg) {
    while(1) {
        printf("Cook is sleeping.\n");
        struct sembuf op[] = {{0, -1, 0}}; // ждем, пока повара разбудят дикари
        semop(semid_cook, op, 1);
        
        printf("Cook is cooking.\n");
        sleep(2); // готовим еду некоторое время
        
        op[0].sem_op = 1;
        semop(semid_pot, op, 1); // наполняем горшок
        curr = M;
        printf("Cook filled the pot.\n");
    }

    return NULL;
}

int main() {
    semid_pot = semget(SEM_KEY_POT, 1, IPC_CREAT | 0666); // создаем семафор горшка
    semctl(semid_pot, 0, SETVAL, 1); // инициализируем его значением 1
    
    semid_cook = semget(SEM_KEY_COOK, 1, IPC_CREAT | 0666); // создаем семафор повара
    semctl(semid_cook, 0, SETVAL, 0); // инициализируем его значением 0, так как повар спит вначале
    pthread_t savage_threads[N], cook_thread;

    int savage_ids[N];
    for(int i = 0; i < N; i++) {
        savage_ids[i] = i;
        pthread_create(&savage_threads[i], NULL, savage, &savage_ids[i]);
    }

    pthread_create(&cook_thread, NULL, cook_fn, NULL);

    for(int i = 0; i < N; i++) {
        pthread_join(savage_threads[i], NULL);
    }

    pthread_join(cook_thread, NULL);

    return 0;
}
