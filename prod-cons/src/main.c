#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <prod-cons.h>

int main(int argc, char** argv)
{
/* ----------------------------- init variables ----------------------------- */
  if (argc != 3)
    return 1;

  int NUM_OF_PRO = atoi(argv[1]);
  int NUM_OF_CON = atoi(argv[2]);
  int NUM_OF_WORKS = PRO_LOOP * NUM_OF_PRO;

  printf("\nNumber of producers: %d.\nNumber of consumers: %d.\n\n", NUM_OF_PRO, NUM_OF_CON);

  queue *fifo;
  pthread_t *pro = (pthread_t *)malloc(sizeof(pthread_t) * NUM_OF_PRO);
  pthread_t *con = (pthread_t *)malloc(sizeof(pthread_t) * NUM_OF_CON);
  int rc;
  long t;
  double averageWaitTime;

  fifo = queueInit(NUM_OF_PRO, NUM_OF_CON);
  
  if (fifo ==  NULL) {
    fprintf (stderr, "Main: Queue init failed.\n");
    exit (1);
  }
/* ------------------------ assign work to producers ------------------------ */
  for (t = 0; t < NUM_OF_PRO; t++) {
    rc = pthread_create(&pro[t], NULL, producer, fifo);
    if (rc) {
        printf("Error: return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
  }
/* ------------------------ assign work to consumers ------------------------ */
  for (t = 0; t < NUM_OF_CON; t++) {
    rc = pthread_create(&con[t], NULL, consumer, fifo);
    if (rc) {
        printf("Error: return code from pthread_create() is %d\n", rc);
        exit(-1);
    }
  }
/* ------------------------------ sync threads ------------------------------ */
  for (int i = 0; i < NUM_OF_PRO; i++) {
    pthread_join(pro[i], NULL);
  }

  for (int i = 0; i < NUM_OF_CON; i++) {
    pthread_join(con[i], NULL);
  }
/* -------------------------------------------------------------------------- */
  queueDelete (fifo);

  averageWaitTime = waitTimeSum / NUM_OF_WORKS;
  printf("\nAverage waiting time per work in the queue = %lf seconds.\n\n", averageWaitTime);

  return 0;
}