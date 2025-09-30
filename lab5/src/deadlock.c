#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

pthread_mutex_t firstLock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t secondLock = PTHREAD_MUTEX_INITIALIZER;

void *FirstThreadWork();
void *SecondThreadWork();

void *FirstThreadWork()
{
  pthread_mutex_lock(&firstLock);
  printf("Поток 1 захватил Мютекс 1");

  sleep(1);

  pthread_mutex_lock(&secondLock);
  printf("Поток 1 захватил Мютекс 2");

  pthread_mutex_unlock(&secondLock);
  pthread_mutex_unlock(&firstLock);
  return NULL;
}

void *SecondThreadWork()
{
  pthread_mutex_lock(&secondLock);
  printf("Поток 2 захватил Мютекс 2");

  sleep(1);

  pthread_mutex_lock(&firstLock);
  printf("Поток 2 захватил Мютекс 1");

  pthread_mutex_unlock(&firstLock);
  pthread_mutex_unlock(&secondLock);
  return NULL;
}

int main()
{
  pthread_t firstThread;
  pthread_t secondThread;

  pthread_create(&firstThread, NULL, FirstThreadWork, NULL);
  pthread_create(&secondThread, NULL, SecondThreadWork, NULL);

  pthread_join(firstThread, NULL);
  pthread_join(secondThread, NULL);

  return 0;
}
