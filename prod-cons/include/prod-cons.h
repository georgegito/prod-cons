#define QUEUESIZE 10
#define PRO_LOOP 10
#define NUM_OF_PRO 8
#define NUM_OF_CON 8

int _arg = 0;
int worksFinished = 0;
int proFinished = 0;
int conFinished = 0;
double addTimeSum = 0;
double delTimeSum = 0;
double tempTime1, tempTime2;
double averageWaitTime;
struct timeval t1, t2;

void *producer (void *args);
void *consumer (void *args);

struct workFunction {
  void * (*work)(void *);
  void * arg;
};

typedef struct {
  struct workFunction buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
} queue;

void * print(int arg)
{
  printf("Work number %d is executed, arg = %d.\n", ++worksFinished, arg);
}

queue *queueInit(void);
void queueDelete (queue *q);
void queueAdd (queue *q, struct workFunction *in);
void queueDel (queue *q, struct workFunction out);

void *producer(void *q)
{
  queue *fifo;
  fifo = (queue *)q;
  struct workFunction func;
  func.arg = &_arg;
  func.work = (void *)print;

  for (int i = 0; i < PRO_LOOP; i++) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->full) {
      // printf ("Producer: queue FULL.\n");
      pthread_cond_wait (fifo->notFull, fifo->mut);
    }

    gettimeofday(&t1, NULL);

    queueAdd (fifo, &func);

    // record the time of adding
    tempTime1 = t1.tv_sec * 1e6; 
    tempTime1 = (tempTime1 + t1.tv_usec) * 1e-6; 
    addTimeSum += tempTime1;
    
    // printf("A producer added a work to queue.\n");
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notEmpty);
  }

  // mutex to avoid data race
  pthread_mutex_lock (fifo->mut);
  proFinished++;
  pthread_mutex_unlock (fifo->mut);

  if (proFinished == NUM_OF_PRO) {
    // printf("I am the last producer.\n");
    while (conFinished != NUM_OF_CON) {
      // printf("Last producer sends signal to unblock the blocked consumers.\n");
      pthread_cond_broadcast(fifo->notEmpty);
    }
  }

  return (NULL);
}

void *consumer(void *q)
{
  queue *fifo;
  fifo = (queue *)q;
  struct workFunction d_func;

  while(1) {
    pthread_mutex_lock (fifo->mut);
    while (fifo->empty) {
      // printf ("Consumer: queue EMPTY.\n");
      if (fifo->empty && proFinished == NUM_OF_PRO) {
        conFinished++;
        pthread_mutex_unlock (fifo->mut);
        // printf("A consumer finished all of its works (%d consumers total).\n", conFinished);
        return (NULL);
      }
      pthread_cond_wait (fifo->notEmpty, fifo->mut);
    }

    gettimeofday(&t2, NULL);

    queueDel (fifo, d_func);

    // record the time of deleting
    tempTime2 = t2.tv_sec * 1e6; 
    tempTime2 = (tempTime2 + t2.tv_usec) * 1e-6; 
    delTimeSum += tempTime2;

    // printf("A consumer removed a work from queue.\n");
    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
  }
}

queue *queueInit(void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));
  pthread_mutex_init (q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init (q->notEmpty, NULL);
	
  return (q);
}

void queueDelete(queue *q)
{
  pthread_mutex_destroy (q->mut);
  free (q->mut);	
  pthread_cond_destroy (q->notFull);
  free (q->notFull);
  pthread_cond_destroy (q->notEmpty);
  free (q->notEmpty);
  free (q);
}

void queueAdd(queue *q,  struct workFunction *in)
{
  q->buf[q->tail] = *in;
  q->tail++;

  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel(queue *q, struct workFunction out)
{
  out = q->buf[q->head];
  ((void(*)())out.work)(*(int *)out.arg);

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}