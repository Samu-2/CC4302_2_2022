#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "bolsa.h"

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  n = PTHREAD_COND_INITIALIZER;
int*   p = NULL;
int*   e;
char** s;
char** b;

int vendo(int precio, char *vendedor, char *comprador) {
  int r = 0;
  pthread_mutex_lock(&m);

  int* mp   = malloc(sizeof(int));
  *mp = precio;
  int* me   = malloc(sizeof(int));
  *me = 0;
  char** ms = malloc(sizeof(char*));
  *ms = vendedor;
  char** mb = malloc(sizeof(char*));
  *mb = comprador;

  if (p == NULL) {
    p = mp;
    e = me;
    s = ms;
    b = mb;
  }
  else if (*p > *mp) {
    p = mp;
    e = me;
    s = ms;
    b = mb;
    pthread_cond_broadcast(&n);
  }
  pthread_cond_wait(&n, &m);
  if (*me == 1) {
    r = 1;
  }
  free(mp);
  free(me);
  free(ms);
  free(mb);
  pthread_mutex_unlock(&m);
  return r;
}

int compro(char *comprador, char *vendedor) {
  int r = 0;
  pthread_mutex_lock(&m);
  if (p != NULL) {
    strcpy(*b, comprador);
    strcpy(vendedor, *s);
    *e = 1;
    r = *p;
    p = NULL;
    e = NULL;
    s = NULL;
    b = NULL;
    pthread_cond_broadcast(&n);
  }
  pthread_mutex_unlock(&m);
  return r;
}
