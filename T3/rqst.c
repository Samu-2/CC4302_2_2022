#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "pss.h"

// REQUEST GENERIC IMPLEMENTATION
typedef struct {
  int             ready;
  pthread_cond_t  w;
  void*           args;
}rqst;

pthread_mutex_t _rqst_mtx = PTHREAD_MUTEX_INITIALIZER;
int             _rqst_bsy = 0;
Queue*          _rqst_q;

void* request_to(void* (*f)(void* args), void* args) {
    f(args);
}