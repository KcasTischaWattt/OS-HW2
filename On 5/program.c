#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

int N = 10; // количество дикарей
int M = 5; // вместимость горшка
int curr = 5; // текущее количество мяса в горшке

sem_t* pot; // семафор для горшка
sem_t* cook; // семафор для повара

void* savage(void* arg) {
    int id = *(int*)arg;

    while(1) {
        srand(time(NULL) + id);
        int sleep_time = rand() % 5 + 1; // генерация случайного времени ожидания от 1 до 5 секунд
        sleep(sleep_time); // ждем это время, прежде чем снова проголодаться

        printf("Savage %d is hungry.\n", id);
        
        sem_wait(pot); // пытаемся получить доступ к горшку
        
        if(curr > 0) { // если в горшке еще есть мясо, едим его
            printf("Savage %d is eating.\n", id);
            curr--;
        } else { // если горшок пустой, будим повара и ждем, пока он его наполнит
            printf("Pot is empty, waking up the cook.\n");
            sem_post(cook); // будим повара
            sem_wait(pot); // ждем, пока повар наполнит горшок
            curr = M; // наполняем горшок и берем один кусок мяса из горшка
            curr--;
            printf("Savage %d got some meat from the pot.\n", id);
        }
        
        sem_post(pot); // освобождаем горшок
    }   

    return NULL;
}

void* cook_fn(void* arg) {
    while(1) {
        printf("Cook is sleeping.\n");
        sem_wait(cook); // ждём, пока повара разбудят дикари

        printf("Cook is cooking.\n");
        sleep(2); // готовим еду некоторое время

        sem_post(pot); // наполняем горшок
        curr = M;
        printf("Cook filled the pot.\n");
    }

    return NULL;
}

int main() {
    pot = sem_open("pot_sem", O_CREAT, 0666, 1); // инициализируем семафор горшка значением 1
    cook = sem_open("cook_sem", O_CREAT, 0666, 0); // инициализируем семафор повара значением 0

    pthread_t threads[N+1]; // добавляем один поток для повара

    int ids[N];
    for(int i = 0; i < N; i++) {
        ids[i] = i+1;
        pthread_create(&threads[i], NULL, savage, &ids[i]);
    }

    pthread_create(&threads[N], NULL, cook_fn, NULL); // создаём один поток повара
    for(int i = 0; i <= N; i++) {
        pthread_join(threads[i], NULL);
    }

    sem_close(pot);
    sem_close(cook);

    sem_unlink("pot_sem");
    sem_unlink("cook_sem");

    return 0;
}
