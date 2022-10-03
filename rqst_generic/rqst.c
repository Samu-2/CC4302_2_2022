#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

// #include "pss.h"
#include "rqst.h"

/*
	THIS IS MY OWN IMPLEMENTATION FOR A GENERIC
	REQUEST PATTERN THAT CAN BE USED FOR WHATEVER
	I'D LIKE TO
	- SAMUEL CHAVEZ @ UCHILE
*/

typedef struct {
	int 			status;
	pthread_mutex_t mtx;
	void* 			payload;
} rqst;


// REQUEST HOLDER IMPLEMENTATION

// Request Holder, used as a condition to wait
struct rqstHold {
	pthread_mutex_t rqsthMtx;
	pthread_cond_t  rqsthCon;
	int rqsthDone;
};

// Fires a holder off
int rqst_HoldOff(RqstHold* rqstHold){
	pthread_mutex_lock(&rqstHold->rqsthMtx);
	rqstHold->rqsthDone = 1;
	pthread_cond_broadcast(&rqstHold->rqsthCon);
	pthread_mutex_unlock(&rqstHold->rqsthMtx);
	return 0;
}

// Waits for a Holdster to be turned off
void rqst_HoldOn(RqstHold* rqstHold){
	pthread_mutex_lock(&rqstHold->rqsthMtx);
	while (!rqstHold->rqsthDone) 
		pthread_cond_wait(&rqstHold->rqsthCon, &rqstHold->rqsthMtx);
	rqstHold->rqsthDone = 0;
	pthread_mutex_unlock(&rqstHold->rqsthMtx);
}

// Frees up a RequestHoldster
int rqst_HoldFree(RqstHold* rqstHold){
	free(rqstHold);
	return 0;
}

// Returns a RequestHoldster to later be used.
RqstHold* rqst_HoldInit(){
	RqstHold* reqHol = malloc(sizeof(RqstHold));
	if(!pthread_mutex_init(&reqHol->rqsthMtx, NULL))
		return NULL;
	if(!pthread_cond_init (&reqHol->rqsthCon, NULL)) 
		return NULL;
	reqHol->rqsthDone = 1;
	return reqHol;
}