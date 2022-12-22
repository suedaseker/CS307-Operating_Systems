#include <iostream>
#include <string>
#include <stdio.h>
#include <vector>
#include <stdlib.h>
#include <stdbool.h>
#include <pthread.h>
#include <semaphore.h>

using namespace std;

pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;

vector<pthread_t> thread_list;
pthread_barrier_t barrier;

int waitingA = 0;
int waitingB = 0;

sem_t *semSame, *semDiff, semA, semB;

void *ThreadFuncA(void *arg)
{
    pthread_mutex_lock(&lock1);
    char team_name = *(char*)arg;
    bool isItDriver = false;

    int *valSame, *valDiff;
    
    cout << "Thread ID: " << pthread_self() << ", Team: " << team_name << ", I am looking for a car\n";
    
    semSame = &semA;
    semDiff = &semB;

    valSame = &waitingA;
    valDiff = &waitingB;

    if((*valSame) < 0 && (*valDiff) < -1) {
        isItDriver = true;
        sem_post(semSame);
        sem_post(semDiff);
        sem_post(semDiff);
        
        (*valSame) += 1;
        (*valDiff) += 2;
    }
    else if((*valSame) < -2) {
        isItDriver = true;
        sem_post(semSame);
        sem_post(semSame);
        sem_post(semSame);
        
        (*valSame) += 3;
    }
    else {
        (*valSame)--;
        pthread_mutex_unlock(&lock1);
        sem_wait(semSame);
    }
    pthread_mutex_lock(&lock2);
    cout << "Thread ID: " << pthread_self() << ", Team: " << team_name << ", I have found a spot in a car\n";
    pthread_mutex_unlock(&lock2);
    pthread_barrier_wait(&barrier);
    
    if(isItDriver)
    {
        cout << "Thread ID: " << pthread_self() << ", Team: " << team_name << ",  I am the captain and driving the car\n";
        pthread_barrier_destroy(&barrier);
        pthread_barrier_init(&barrier, NULL, 4);

        pthread_mutex_unlock(&lock1);
    }
    return NULL;
}

void *ThreadFuncB(void *arg)
{
    pthread_mutex_lock(&lock1);
    char team_name = *(char*)arg;
    bool isItDriver = false;

    int *valSame, *valDiff;
    
    cout << "Thread ID: " << pthread_self() << ", Team: " << team_name << ", I am looking for a car\n";
    
    semSame = &semB;
    semDiff = &semA;

    valSame = &waitingB;
    valDiff = &waitingA;

    if((*valSame) < 0 && (*valDiff) < -1) {
        isItDriver = true;
        sem_post(semSame);
        sem_post(semDiff);
        sem_post(semDiff);

        (*valSame) += 1;
        (*valDiff) += 2;
    }
    else if((*valSame) < -2) {
        isItDriver = true;
        sem_post(semSame);
        sem_post(semSame);
        sem_post(semSame);
        
        (*valSame) += 3;
    }
    else {
        (*valSame)--;
        pthread_mutex_unlock(&lock1);
        sem_wait(semSame);
    }
    pthread_mutex_lock(&lock2);
    cout << "Thread ID: " << pthread_self() << ", Team: " << team_name << ", I have found a spot in a car\n";
    pthread_mutex_unlock(&lock2);
    pthread_barrier_wait(&barrier);
    
    if(isItDriver)
    {
        cout << "Thread ID: " << pthread_self() << ", Team: " << team_name << ",  I am the captain and driving the car\n";
        pthread_barrier_destroy(&barrier);
        pthread_barrier_init(&barrier, NULL, 4);

        pthread_mutex_unlock(&lock1);
    }
    return NULL;
}

int main(int argc, char* argv[])
{
    int numTA = atoi(argv[1]);
    int numTB = atoi(argv[2]);
    int numAll = numTA + numTB;

    if (numTA % 2 != 0 || numTB % 2 != 0 || numAll % 4 != 0){
        cout << "Wrong arguments, the main terminates\n";
        return 0;
    }

    vector<char> teamName;
    pthread_t t_id[numAll];

    for (int i = 0; i < numTA; i++) {
        teamName.push_back('A');
    }
    for (int i = numTA; i < numAll; i++){
        teamName.push_back('B');
    }

    pthread_barrier_init(&barrier, NULL, 4);

    sem_init(&semA, 0, 0);
    sem_init(&semB, 0, 0);

    for (int i = 0; i < numAll; i++){
        int ct;
        if (teamName[i] == 'A')
            ct = pthread_create(&t_id[i], NULL, ThreadFuncA, &teamName[i]);
        else
            ct = pthread_create(&t_id[i], NULL, ThreadFuncB, &teamName[i]);

        if ( ct != 0)
        {
            cout << "Failed to create thread" << endl;
            return 0;
        }
    }

    for (int i = 0; i < numAll; i++){
        int jt = pthread_join(t_id[i],NULL);
        if (jt != 0)
        {
            cout << "Failed to join thread" << endl;
            return 0;
        }
    }

    pthread_barrier_destroy(&barrier);

    cout << "The main terminates\n";
    return 0;
}
