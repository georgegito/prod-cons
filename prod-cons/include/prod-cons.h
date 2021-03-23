#define QUEUESIZE 10
#define PRO_LOOP 50

int workIndex = 0;
int proFinished = 0;
int conFinished = 0;
double waitTimeSum = 0;
struct timeval t1, t2;

void *producer (void *args);
void *consumer (void *args);

struct workFunction {
  void * (*work)(void *);
  void * arg;
  double addTime;
};

typedef struct {
  struct workFunction *buf[QUEUESIZE];
  long head, tail;
  int full, empty;
  pthread_mutex_t *mut;
  pthread_cond_t *notFull, *notEmpty;
  int NUM_OF_PRO, NUM_OF_CON;
} queue;

void * print(int arg)
{
  printf("Work %d is executed.\n", arg);
}

queue *queueInit(int NUM_OF_PRO, int NUM_OF_CON);
void queueDelete(queue *q);
void queueAdd(queue *q, struct workFunction *in);
void queueDel(queue *q, struct workFunction *out);

void *producer(void *q)
{
  queue *fifo;
  fifo = (queue *)q;

  for (int i = 0; i < PRO_LOOP; i++) {
    struct workFunction * _func = (struct workFunction *)malloc(sizeof(struct workFunction));
    _func->work = (void *)print;
    _func->arg = malloc(sizeof(int));
    pthread_mutex_lock(fifo->mut);
    while (fifo->full) {
      // printf ("Producer: queue FULL.\n");
      pthread_cond_wait(fifo->notFull, fifo->mut);
    }

    *((int*)_func->arg) = ++workIndex;
    queueAdd(fifo, _func);
    
    pthread_mutex_unlock(fifo->mut);
    pthread_cond_signal(fifo->notEmpty);
  }

  // mutex to avoid data race
  pthread_mutex_lock(fifo->mut);
  proFinished++;
  pthread_mutex_unlock(fifo->mut);

  // the last producer sends signals to unblock the consumers when they finish theirs works
  if (proFinished == fifo->NUM_OF_PRO) {
    // printf("I am the last producer.\n");
    while (conFinished != fifo->NUM_OF_CON) {
      // printf("The last producer sends signal to unblock the blocked consumers.\n");
      pthread_cond_broadcast(fifo->notEmpty);
    }
  }

  return (NULL);
}

void *consumer(void *q)
{
  queue *fifo;
  fifo = (queue *)q;

  while(1) {
    struct workFunction * d_func;
    pthread_mutex_lock(fifo->mut);
    while (fifo->empty) {
      // printf ("Consumer: queue EMPTY.\n");
      if (fifo->empty && proFinished == fifo->NUM_OF_PRO) {
        conFinished++;
        pthread_mutex_unlock (fifo->mut);
        // printf("A consumer finished all of its works (%d consumers total).\n", conFinished);
        return (NULL);
      }
      pthread_cond_wait(fifo->notEmpty, fifo->mut);
    }

    queueDel(fifo, d_func);

    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);
  }
}

queue *queueInit(int NUM_OF_PRO, int NUM_OF_CON)
{
  queue *q;

  q = (queue *)malloc(sizeof (queue));
  if (q == NULL) return (NULL);

  q->empty = 1;
  q->full = 0;
  q->head = 0;
  q->tail = 0;
  q->mut = (pthread_mutex_t *)malloc(sizeof (pthread_mutex_t));
  pthread_mutex_init(q->mut, NULL);
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init(q->notFull, NULL);
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));
  pthread_cond_init(q->notEmpty, NULL);
  q->NUM_OF_PRO = NUM_OF_PRO;
  q->NUM_OF_CON = NUM_OF_CON;

  return (q);
}

void queueDelete(queue *q)
{
  pthread_mutex_destroy(q->mut);
  free(q->mut);	
  pthread_cond_destroy(q->notFull);
  free(q->notFull);
  pthread_cond_destroy(q->notEmpty);
  free(q->notEmpty);
  free(q);
}

void queueAdd(queue *q,  struct workFunction *in)
{
/* ---------------------------------- timer --------------------------------- */
  gettimeofday(&t1, NULL);
  in->addTime = t1.tv_sec * 1e6; 
  in->addTime = (in->addTime + t1.tv_usec) * 1e-6;
/* -------------------------------------------------------------------------- */

  q->buf[q->tail] = in;
  q->tail++;

  // printf("A producer added work %d to the queue.\n", *((int*)in->arg));

  if (q->tail == QUEUESIZE)
    q->tail = 0;
  if (q->tail == q->head)
    q->full = 1;
  q->empty = 0;

  return;
}

void queueDel(queue *q, struct workFunction *out)
{
  out = q->buf[q->head];

/* ---------------------------------- timer --------------------------------- */
  gettimeofday(&t2, NULL);
  double delTime = t2.tv_sec * 1e6;
  delTime = (delTime + t2.tv_usec) * 1e-6;
  double waitTime = delTime - out->addTime;
  // printf("Waiting time = %lf seconds\n", waitTime);
  waitTimeSum += waitTime; // already mutex locked
/* -------------------------------------------------------------------------- */

  ((void(*)())out->work)(*(int *)out->arg);
  // printf("A consumer removed work %d from the queue.\n", *((int*)out->arg));
  free(out->arg);
  free(out);

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}