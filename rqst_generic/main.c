#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

// #include "pss.h"
#include "rqst.h"

/*
    MAIN TEST EXAMPLE FOR THE BS REQUEST IMPLEMENTATION
    THAT I'M DOING INSTEAD OF STUDYING. GOD PLEASE HELP
    - SAMUEL CHAVEZ @ UCHILE
*/

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    RqstHold* a;
    int       b;
}args;

void* funA (void* ptr) {
    args* a = (args*) ptr;
    printf("Fun A: XD %d \n", a->b);
    rqst_HoldOn(a->a);
    printf("Fun A: xd %d \n", a->b);
    return NULL;
}

void* funB (void* ptr) {
    args* a = (args*) ptr;
    printf("Fun B: XD %d \n", a->b);
    sleep(2);
    a->b = 2;
    rqst_HoldOff(a->a);
    printf("Fun B: xd %d \n", a->b);
    return NULL;
}   
// int arg __attribute__((unused)), char **argv __attribute__((unused))
int main() {
    args* a = malloc(sizeof(args));
    a->a = rqst_HoldInit();
    a->b = 1;
    pthread_t lt, rt;
    pthread_create(&lt, NULL, funA, &a);
    pthread_create(&rt, NULL, funB, &a);
    pthread_join(lt, NULL);
    pthread_join(rt, NULL);
    rqst_HoldFree(a->a);
    free(a);
    return 0;
}