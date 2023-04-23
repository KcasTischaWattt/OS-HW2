#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define SHM_SIZE 1024

int N = 10; // количество дикарей
int M = 5; // вместимость горшка
int curr = 5; // текущее количество мяса в горшке

sem_t* pot; // семафор для горшка
sem_t* cook; // семафор для повара
int* shared_mem; // разделяемая память для обмена информацией о состоянии горшка

void savage() {
    int id = getpid();

    while(1) {
        srand(time(NULL) + id);
        int sleep_time = rand() % 5 + 1; // генерация случайного времени ожидания от 1 до 5 секунд
        sleep(sleep_time); // ждем это время, прежде чем снова проголодаться

        printf("Savage %d is hungry.\n", id);
        
        sem_wait(pot); // пытаемся получить доступ к горшку
        
        if(*shared_mem > 0) { // если в горшке еще есть мясо, едим его
            printf("Savage %d is eating.\n", id);
            (*shared_mem)--;
        } else { // если горшок пустой, будим повара и ждем, пока он его наполнит
            printf("Pot is empty, waking up the cook.\n");
            sem_post(cook); // будим повара
            sem_wait(pot); // ждем, пока повар наполнит горшок
            printf("Savage %d got some meat from the pot.\n", id);
            (*shared_mem)--; // дикарь берет один кусок мяса из горшка
        }
        
        sem_post(pot); // освобождаем горшок
    
    }   
}

int main() {
    // Создание семафоров
    pot = sem_open("/pot_sem", O_CREAT, 0666, 1);
    cook = sem_open("/cook_sem", O_CREAT, 0666, 0);

    // Создание разделяемой памяти
    key_t key = ftok("shared_mem_key", 1);
    int shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
    shared_mem = (int*)shmat(shmid, NULL, 0);

    // Инициализация горшка
    *shared_mem = curr;

    // Создание дикарей
    for(int i = 0; i < N; i++) {
    pid_t pid = fork();
        if(pid == -1) { // Если создание процесса дикаря не удалось
        printf("Error: could not create savage %d.\n", i);
        exit(1);
    } else if(pid == 0) { // Если процесс создан успешно
        savage(); // запускаем функцию дикаря
        exit(0); // завершаем процесс дикаря
    }
}

// Создание повара
pid_t pid = fork();

if(pid == -1) { // Если создание процесса повара не удалось
    printf("Error: could not create cook.\n");
    exit(1);
} else if(pid == 0) { // Если процесс повара создан успешно
    while(1) {
        sem_wait(cook); // ждем, пока кто-то не разбудит повара
        printf("Cook is filling the pot.\n");
        *shared_mem = M; // повар наполняет горшок
        printf("Pot is full again.\n");
        sem_post(pot); // разбудим ожидающих доступа к горшку
    }
    exit(0); // завершаем процесс повара
}

// Ожидание завершения всех процессов
int status;
for(int i = 0; i < N + 1; i++) { // ожидаем завершения всех дикарей и повара
    wait(&status);
}

// Удаление семафоров
sem_unlink("/pot_sem");
sem_unlink("/cook_sem");

// Удаление разделяемой памяти
    shmdt(shared_mem);
    hmctl(shmid, IPC_RMID, NULL);

    return 0;
}
