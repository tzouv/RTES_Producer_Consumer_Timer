/*
 *
 *	Short	: A timer in C/C++ by using the producer and consumer problem
 *
 *	Long 	:
 *
 *	Author	: Tzouvaras Evangelos
 *
 *	Date	: 22 September 2023
 *
 *	Revised	:
 */

/* User includes */
#include <pthread.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>       // include the library that contains the gettimeofday() function
#include <signal.h>         // To handle the signal
#include <math.h>
#include <time.h>           // To use the time_t type


/* Define the queue size and the loop number */
#define QUEUESIZE 10

/* Define the number of producers an consumers */
//#define PRODUCER 1        ONLY ONE PRODUCER
#define CONSUMER 1

/* Define the timer parameters*/
#define PERIOD 10            // Period of timer in milliseconds (10 or 100 or 1000)
#define START_DELAY 0         // The delay at the start of the timer in seconds
#define TIME_TO_EXECUTE 100  // The time to execute in seconds (total execution time an hour)

/* Producer and consumer functions */
void *producer (void *args);
void *consumer (void *args);

/* Struct timeval to measure the timer periods (array with the maximum number of periods)*/
struct timeval prod_times[TIME_TO_EXECUTE*100];
long long int periods[TIME_TO_EXECUTE*100];

/* Struct timeval to measure the execution time of each TimerFcn */
struct timeval con_times[TIME_TO_EXECUTE*200];
long long int execution_time[TIME_TO_EXECUTE*100];
int executed_tasks = 0;
int count = 0;

int drift_error[TIME_TO_EXECUTE*100];

/* Take the initial time */
struct timeval initialTime;

/* Take the current time by using the sys/time library, to use it at startat function*/
struct timeval currentTime;


/* The new struct for FIFO contains the pointer to the data and the function that will be executed at each period time */
typedef struct {
  void *(*work)(void *);
  void *arg;
}workFunction;

/* Define the queue as a struct */
typedef struct {
  workFunction buffer[QUEUESIZE];           // An array with type workFunction

  long head, tail;                          // head and tail: Indexes for the array
  int full, empty;                          // full, empty: Use as booleans: TRUE, FALSE
  pthread_mutex_t *mut;                     // A mutex variable
  pthread_cond_t *notFull, *notEmpty;       // Two condition variables, notFull, notEmpty
} queue;

/* Declare a struct Timer with all the timer parameters and functions (Timer specifications)*/
typedef struct{
  int period;                                 // The time between two executions of the TimerFcn (in milliseconds)
  int TasksToExecute;                         // The number of task executions
  int  StartDelay;                            // The delay time (in seconds), before the start of the timer

  // Declare a queue inside the struct
  queue *fifo;

  // Declare the producers
  pthread_t pro;

  // A workfunction struct that contains the userData and the TimerFcn
  workFunction Timer_Work;

  // Declare the struct functions (pointers in functions)
  void *(*StartFcn)(void *);
  void *(*StopFcn)(void *);
  void *(*ErrorFcn)(void *);
}Timer;

void *start_Fcn(void *arg);                    // A function for the initialization of the timer variables
void *stop_Fcn(void *arg);                     // A function that runs after the final execution of the TimerFcn
void *error_Fcn(void *arg);                    // A function that runs when the queue is full

/* Construction function for a Timer Struct object */
Timer *Timer_Init (queue *q);
void Timer_Delete(Timer *t);

/* Functions for the timer management*/
void start(Timer *t);                                                   // Start the timer t
void startat(Timer *t, int y, int m, int d, int h, int min, int sec);   // Start the timer after a specific time

/* Functions for the queue management */
queue *queueInit (void);
void queueDelete (queue *q);
void queueAdd (queue *q, workFunction in);      // The queue will have workFunction types
void queueDel (queue *q, workFunction *out);    // *out workFunction type

// work function: Argument a pointer
// Input: an array with angles, Output: An array with the calculated sin of angles
void *work(void *work_array){
  float *work_return;
  work_return = (float *)malloc(10*sizeof(float));
  int i = 0;
  int * work_int = (int *)(work_array);
  for(i = 0; i < 10; i++){
    work_return[i] = (float)sin(work_int[i]);
  }
  printf("\nFunction Run\n");
  return (NULL);
}


int main ()
{
  queue *fifo;                              // Create a queue fifo

  // Create an array of consumers
  pthread_t con[CONSUMER];

 /* Create the files to save the results */
 FILE *fp1 = fopen("Periods_measurement.txt","wb");
 FILE *fp2 = fopen("Execution_times.txt", "wb");
 FILE *fp3 = fopen("Drift_error.txt", "wb");

  fifo = queueInit ();                      // Run the function to initialize the fifo
  if (fifo ==  NULL) {                      // If the initialization did not return an array print an error message
    fprintf (stderr, "main: Queue Init failed.\n");
    exit (1);
  }

  /* Create a Timer struct object */
  Timer *timer = Timer_Init(fifo);

  printf(" \n\n TASKS TO EXECUTE: %d\n\n", timer->TasksToExecute);
  /* Start the timer by using the start() function (Also can be started by using the startart() function)*/
  start(timer);

  /*Create multiple consumers. Also join for multiple producers and consumers */
  int j = 0;
  for(j = 0; j < CONSUMER; j++)
    pthread_create (&con[j], NULL, consumer, fifo);          // Create the pthread con, with start function the consumer and argument fifo

  pthread_join (timer->pro, NULL);                             // Consumer, wait the producer thread

  int l = 0;
  for(l = 0; l < CONSUMER; l++)
    pthread_join (con[l], NULL);                             // Consumer, wait the producer thread

  printf("\nFINISHED\n");

  /* Calculate the periods and save them into the file */
  int h = 0;
  for (h = 0; h < (timer->TasksToExecute - 1); h++){
    periods[h] = (1000000*prod_times[h+1].tv_sec - 1000000*prod_times[h].tv_sec + prod_times[h+1].tv_usec - prod_times[h].tv_usec);
    char str[32];
    sprintf(str, "%lld", periods[h]);               // Convert the time into string to save it on the file
    fputs(str, fp1);                             // Place the string(time) into the file
    fputc('\n',fp1);
  }

  /* Calculate the Execution Times and save them into the file */
  int x = 0;
  for (x = 0; x < timer->TasksToExecute; x++){
    execution_time[x] = (1000000*con_times[x].tv_sec - 1000000*prod_times[x].tv_sec + con_times[x].tv_usec - prod_times[x].tv_usec);
    char str2[32];
    sprintf(str2, "%lld", execution_time[x]);               // Convert the time into string to save it on the file
    fputs(str2, fp2);                             // Place the string(time) into the file
    fputc('\n',fp2);
  }

  /* Calculate the Drift errors and save them into the file */
  int y = 0;
  for (y = 0; y < timer->TasksToExecute; y++){
    drift_error[y] = (int)periods[y] - (timer->period)*1000;
    char str3[32];
    sprintf(str3, "%d", drift_error[y]);               // Convert the time into string to save it on the file
    fputs(str3, fp3);                             // Place the string(time) into the file
    fputc('\n',fp3);
  }

  /* Delete the fifo queue struct pointer */
  queueDelete (fifo);

  /* Delete the timer */
  Timer_Delete(timer);

  /* Close the files */
  fclose(fp1);
  fclose(fp2);
  fclose(fp3);

  return 0;
}

void *producer (void *t)
{
  // Create a timer and copy the *t pointer after typecasting
  Timer *timer;
  timer = (Timer *)t;

  // Delay time for each iteration
  int delay_time;

  int i;
  for (i = 0; i < (timer->TasksToExecute); i++) {                  // Loop to run the code multiple times
    pthread_mutex_lock ((timer->fifo)->mut);                 // Mutex lock
    while ((timer->fifo)->full) {                            // While fifo is full
      printf ("producer: queue FULL.\n");           // Print fifo is full
      pthread_cond_wait ((timer->fifo)->notFull, (timer->fifo)->mut); // producer thread wait until the fifo is notFull, then unlock the mutex
    }
    printf("\nTask to Execute: %d\n", i);

    // Measure each time that the producer place an element to the queue
    gettimeofday(&prod_times[i], NULL);

    // The producer will place inside the queue a workfunction type structs
    queueAdd ((timer->fifo), (timer->Timer_Work));

    pthread_mutex_unlock ((timer->fifo)->mut);               // Unlock the mutex
    pthread_cond_signal ((timer->fifo)->notEmpty);           // The consumer thread waits for the notEmpty signal

    delay_time =  1000000*initialTime.tv_sec + initialTime.tv_usec + (timer->period)*1000*(i+1) - 1000000*prod_times[i].tv_sec - prod_times[i].tv_usec ;

    usleep (delay_time);
  }
  return (NULL);
}

void *consumer (void *q)
{
  queue *fifo;
  int i, d;

  /* A workFunction type */
  workFunction out;
  fifo = (queue *)q;

  while(1){                                             // Consumer will always run
    pthread_mutex_lock (fifo->mut);                     // Lock the mutex
    while (fifo->empty) {                               // While fifo is empty
      printf ("consumer: queue EMPTY.\n");              // Print the queue is empty
      pthread_cond_wait (fifo->notEmpty, fifo->mut);    // Mutex will remain lock until the fifo is empty. If FIFO not empty unlock the mutex
    }

    /* The consumer will delete each fifo element and will run the function */
    queueDel (fifo, &out);                              // The out workFunction will take the final workFunction queue elemen
    out.work(out.arg);                                  // Here the consumer calls the work function
    free(out.arg);

    /* Get the time that the consumer run the function */
    gettimeofday(&con_times[executed_tasks], NULL);

    pthread_mutex_unlock (fifo->mut);
    pthread_cond_signal (fifo->notFull);                // The producer thread waits for the notFull signal
    printf ("consumer: recieved %d.\n", d);

    /* Stop the consumer */
    executed_tasks = executed_tasks + 1;
    if(executed_tasks == ((TIME_TO_EXECUTE*1000)/PERIOD))
        break;
  }

  return (NULL);
}

// Queue init: Take a queue, malloc an array for the queue and return the pointer that shows on the first index of the array
queue *queueInit (void)
{
  queue *q;

  q = (queue *)malloc (sizeof (queue));                 // Malloc a size of queue
  if (q == NULL) return (NULL);                         // If q = NULL, cannot create the queue

// Initialize the parameters of the queue
  q->empty = 1;                                         // q empty at the begin -> TRUE
  q->full = 0;                                          // q full at the begin -> FALSE
  q->head = 0;                                          // q head and tail at the first index of the array (0)
  q->tail = 0;
  q->mut = (pthread_mutex_t *) malloc (sizeof (pthread_mutex_t));       // Allocate space for the mutex
  pthread_mutex_init (q->mut, NULL);                                    // Initialize the mutex
  q->notFull = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));     // Allocate space for the condition variable notFull
  pthread_cond_init (q->notFull, NULL);                                 // Initialize notFull condition variable
  q->notEmpty = (pthread_cond_t *) malloc (sizeof (pthread_cond_t));    // Allocate space for the condition variable notEmpty
  pthread_cond_init (q->notEmpty, NULL);                                // Initialize notEmpty condition variable

  return (q);
}

// Function to delete the queue
void queueDelete (queue *q)
{
  pthread_mutex_destroy (q->mut);               // Destroy the mutex
  free (q->mut);                                // Free the mutex's space
  pthread_cond_destroy (q->notFull);            // Destroy  the condition variable notFull
  free (q->notFull);                            // Free the notFull's space
  pthread_cond_destroy (q->notEmpty);           // Destroy the condition variable notEmpty
  free (q->notEmpty);                           // Free the notEmpty's space
  free (q);                                     // Free the q array space
}

// Add queue
// The queue has types workFunction (not int), so in the queue will be added types of workFunction
void queueAdd (queue *q, workFunction in)
{
  // Now the workFunction types will be added to the buffer which is an array of workFunction types
  q->buffer[q->tail] = in;             // Go to the tail index element of the buffer and place the in

  q->tail++;                        // Increase the tail by one
  if (q->tail == QUEUESIZE)         // If the tail went at the final element of the queue, place the tail at the begin
    q->tail = 0;
  if (q->tail == q->head)           // If the tail is equal with head, then the queue is full
    q->full = 1;
  q->empty = 0;

  return;
}

// queueDel will delete types of workFunction
void queueDel (queue *q, workFunction *out)
{
  *out = q->buffer[q->head];                // here buffer

  q->head++;
  if (q->head == QUEUESIZE)
    q->head = 0;
  if (q->head == q->tail)
    q->empty = 1;
  q->full = 0;

  return;
}

/* Implementation of the timer functions */

// Initialize a timer
Timer *Timer_Init (queue *q){
  Timer *t;

  // Allocate memory for the timer
  t = (Timer *)malloc (sizeof (Timer));
  if (!t){
    printf("Failed to allocate memory for the timer!");
    return (NULL);
  }

  // Initialize the timer variables
  t->period = PERIOD;
  t->TasksToExecute = (TIME_TO_EXECUTE*1000)/PERIOD;
  t->StartDelay = START_DELAY;

  // Copy the external queue into the timer queue
  t->fifo = q;

  // Initialize the functions
  t->StartFcn = &start_Fcn;
  t->StopFcn = &stop_Fcn;
  t->ErrorFcn = &error_Fcn;

  // Initialize the array with the angles
  int *p = (int *)malloc(sizeof(int)*10);
  int k = 0;
  for(k = 0; k < 10; k++)
    p[k] = k;
  t->Timer_Work.arg = p;
  t->Timer_Work.work = &work;
  return t;
}

// Delete a timer
void Timer_Delete(Timer *t){
  free(t);
}

// Implementation of the StartFcn
void *start_Fcn(void *arg){
  printf("The timer has been started!");
  return (NULL);
}

// Implementation of the StopFcn
void *stop_Fcn(void *arg){
  printf("The timer has been stopped!");
  return (NULL);
}

// Implementation of the ErrorFcn
void *error_Fcn(void *arg){
  printf("ERROR: The queue is full");
  return (NULL);
}

// Function to start the timer
void start(Timer *t){
  (t-> StartFcn)(NULL);
  pthread_create (&(t->pro), NULL, producer, t);
  // Take the initial time, when the timer starts
  gettimeofday(&initialTime, NULL);
}

// Function to start the timer at specific time
void startat(Timer *t, int y, int m, int d, int h, int min, int sec){
  // Start the timer by calling the StartFcn
  (t-> StartFcn)(NULL);

  // Convert the specific date into a struct timeval type
  struct tm specificDateTime = {0};
  specificDateTime.tm_year = y - 1900;
  specificDateTime.tm_mon = m - 1;
  specificDateTime.tm_mday = d;
  specificDateTime.tm_hour = h;
  specificDateTime.tm_min = min;
  specificDateTime.tm_sec = sec;

  // Convert the specificDateTime to seconds by using the mktime function
  time_t DelayTime = mktime(&specificDateTime);

  // Take the current time by using the gettimeofday() function
  gettimeofday(&currentTime, NULL);

  // Calculate the time difference
  int Delay = DelayTime - (currentTime.tv_sec);// + 0.000001*currentTime.tv_usec);
  if (Delay > 0)
    t->StartDelay = Delay;
  else
    t->StartDelay = 0;

  // Add an extra delay before the producer creation
  usleep((t->StartDelay)*1000000);

  // Create the producer
  pthread_create (&(t->pro), NULL, producer, t);
}
