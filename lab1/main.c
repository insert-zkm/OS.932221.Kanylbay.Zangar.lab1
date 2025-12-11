#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>

#define EVENT_NUM 10

typedef struct {
    int id;
    char info[64]; // serrialization data
} EventData;

pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

bool has_event = false;
EventData* event_ptr = NULL;

void* producer(void*);
void* consumer(void*);

int main(void) {
    pthread_t prod, cons;

    if (pthread_create(&prod, NULL, producer, NULL) != 0) {
        perror("Producer thread creation error");
        exit(1);
    }
    if (pthread_create(&cons, NULL, consumer, NULL) != 0) {
        perror("Consumer thread creation error");
        exit(1);
    }

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    return 0;
}

void* consumer(void* arg)  {
    while (true) {
        pthread_mutex_lock(&mtx);
        while (!has_event) {
            pthread_cond_wait(&cond, &mtx);
        }

        EventData* ev = event_ptr;
        event_ptr = NULL;
        has_event = 0;
    

        printf("Consumer: received event %d\n", ev->id);
        printf("Consumer: processing info at %p\n",
            (void*)ev->info);
        fflush(stdout);

        free(ev);

        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mtx);
    }
}

void* producer(void* arg)  {
    int i = 1;
    while(true) { // gen events
        sleep(1);

        EventData* ev = malloc(sizeof(EventData));
        ev->id = i++;
        snprintf(ev->info, sizeof(ev->info),
                "info for event %d", ev->id);

        pthread_mutex_lock(&mtx);

        while (has_event) {
            pthread_cond_wait(&cond, &mtx);
        }
        event_ptr = ev;
        has_event = true;
        printf("Producer: event %d sent\n", ev->id);
        fflush(stdout);

        pthread_cond_broadcast(&cond);
        pthread_mutex_unlock(&mtx);
    }
}
