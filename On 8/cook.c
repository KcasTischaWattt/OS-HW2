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

void *cooking() {
    while(1) {
        sem_wait(cook); // ждем, пока дикари не съедят всё мясо
        printf("Cook is adding meat to the pot.\n");
        *shared_mem = M; // повар добавляет максимальное количество мяса в горшок
        sem_post(pot); // оповещаем дикарей о том, что мясо добавлено в горшок
    }
}

int main() {
    // Создание семафоров
    pot = sem_open("/pot_sem", IPC_CREAT, 0666, 1);
    cook = sem_open("/cook_sem", IPC_CREAT, 0666, 0);

    // Создание разделяемой памяти
    key_t key = ftok("shared_mem_key", 1);
    int shmid = shmget(key, SHM_SIZE, IPC_CREAT | 0666);
    shared_mem = (int*)shmat(shmid, NULL, 0);

    // Инициализация горшка
    *shared_mem = curr;

    // Создание потока для повара
    pthread_t cook_thread;
    pthread_create(&cook_thread, NULL, cooking, NULL);

    // Ожидание завершения потока
    pthread_join(cook_thread, NULL);

    // Удаление семафоров и разделяемой памяти
    sem_unlink("/pot_sem");
    sem_unlink("/cook_sem");
    shmctl(shmid, IPC_RMID, NULL);

    return 0;
}
