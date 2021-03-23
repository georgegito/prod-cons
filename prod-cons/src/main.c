#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <prod-cons.h>

int main()
{
  int NUM_OF_WORKS = PRO_LOOP * NUM_OF_PRO;

  queue *fifo;
  pthread_t pro[NUM_OF_PRO];
  pthread_t con[NUM_OF_CON];
  int rc;
  long t;

  fifo = queueInit();
  
  if (fifo ==  NULL) {
    fprintf (stderr, "Main: Queue Init failed.\n");
    exit (1);
  }

  for (t = 0; t < NUM_OF_PRO; t++) {
    rc = pthread_create(&pro[t], NULL, producer, fifo);
    if (rc) {
        printf("Error: return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
  }

  for (t = 0; t < NUM_OF_CON; t++) {
    rc = pthread_create(&con[t], NULL, consumer, fifo);
    if (rc) {
        printf("Error: return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
  }

  for (int i = 0; i < NUM_OF_PRO; i++) {
    pthread_join(pro[i], NULL);
  }

  for (int i = 0; i < NUM_OF_CON; i++) {
    pthread_join(con[i], NULL);
  }

  queueDelete (fifo);

  averageWaitTime = (delTimeSum - addTimeSum) / NUM_OF_WORKS;
  printf("Average waiting time in queue per work = %lf seconds.\n", averageWaitTime);

  return 0;
}