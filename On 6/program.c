#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/sem.h>

int N = 10; // количество дикарей
int M = 5; // вместимость горшка
int curr = 5; // текущее количество мяса в горшке

int pot; // идентификатор семафора для горшка
int cook; // идентификатор семафора для повара

void* savage(void* arg) {
    int id = *(int*)arg;

    while(1) {
        srand(time(NULL) + id);
        int sleep_time = rand() % 5 + 1; // генерация случайного времени ожидания от 1 до 5 секунд
        sleep(sleep_time); // ждем это время, прежде чем снова проголодаться

        printf("Savage %d is hungry.\n", id);
        
        struct sembuf pot_lock = {0, -1, 0}; // ждем, пока горшок не освободится
        semop(pot, &pot_lock, 1);
        
        if(curr > 0) { // если в горшке еще есть мясо, едим его
            printf("Savage %d is eating.\n", id);
            curr--;
        } else { // если горшок пустой, будим повара и ждем, пока он его наполнит
            printf("Pot is empty, waking up the cook.\n");
            
            struct sembuf cook_unlock = {0, 1, 0}; // разблокируем семафор для повара, чтобы он мог начать готовить
            semop(cook, &cook_unlock, 1);
            
            struct sembuf pot_lock_again = {0, -1, 0}; // ждем, пока горшок не будет заполнен
            semop(pot, &pot_lock_again, 1);
            
            printf("Savage %d got some meat from the pot.\n", id);
            curr--; // дикарь берет один кусок мяса из горшка
        }
        
        struct sembuf pot_unlock = {0, 1, 0}; // освобождаем горшок
        semop(pot, &pot_unlock, 1);
    }   

    return NULL;
}

void* cook_fn(void* arg) {
    while(1) {
        printf("Cook is sleeping.\n");
        
        struct sembuf cook_lock = {0, -1, 0}; // ждем, пока повара разбудят дикари
        semop(cook, &cook_lock, 1);
        
        printf("Cook is cooking.\n");
        sleep(2); // готовим еду некоторое время
        
        struct sembuf pot_lock = {0, -1, 0}; // заполняем горшок
        semop(pot, &pot_lock, 1);
        
        curr = M;
        printf("Cook filled the pot.\n");
        
        struct sembuf pot_unlock = {0, 1, 0}; // освобождаем горшок
	semop(pot, &pot_unlock, 1);
     }
     return NULL;
}

int main() {
	pthread_t savages[N];
	pthread_t cook_thread;
	int savage_ids[N];
	for(int i = 0; i < N; i++) {
	    savage_ids[i] = i;
	    pthread_create(&savages[i], NULL, savage, &savage_ids[i]);
	}

	pot = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT); // создаем семафор для горшка
	cook = semget(IPC_PRIVATE, 1, 0666 | IPC_CREAT); // создаем семафор для повара

	// инициализируем семафоры
	union semun {
	    int val;
	    struct semid_ds *buf;
	    unsigned short *array;
	} arg;

	arg.val = 1;
	semctl(pot, 0, SETVAL, arg); // устанавливаем начальное значение семафора для горшка в 1
	arg.val = 0;
	semctl(cook, 0, SETVAL, arg); // устанавливаем начальное значение семафора для повара в 0

	pthread_create(&cook_thread, NULL, cook_fn, NULL); // создаем поток для повара

	for(int i = 0; i < N; i++) {
	    pthread_join(savages[i], NULL); // ожидаем завершения потоков дикарей
	}

	pthread_cancel(cook_thread); // отменяем поток повара

	semctl(pot, 0, IPC_RMID, 0); // удаляем семафор для горшка
	semctl(cook, 0, IPC_RMID, 0); // удаляем семафор для повара

	return 0;
}
