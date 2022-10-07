#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

#include "pss.h"
#include "disco.h"

/*REQUEST PATTERN*/

typedef struct {
	int ready;
	pthread_cond_t w;
} Request;

pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
int busy = 0;
Queue *q;

void ocupar() {
	pthread_mutex_lock(&m);
	if (!busy) busy = 1;
	else {
		Request req= { 0,
		PTHREAD_COND_INITIALIZER};
		put(q, &req);
		while (!req.ready)
		pthread_cond_wait(&req.w, &m);
	}
	pthread_mutex_unlock(&m);
}

void desocupar() {
	pthread_mutex_lock(&m);
		if (emptyQueue(q))
		busy = 0;
		else {
		Request *preq= get(q);
		preq->ready = 1;
		pthread_cond_signal(&preq->w);
	}
	pthread_mutex_unlock(&m);
}


/*END REQUEST PATTERN*/

Queue *q_varon, *q_dama;

typedef struct{
	char* nom;
	char* par;
	int  rdy;
} Persona;

void DiscoInit(void) {
	q = makeQueue();
	q_varon = makeQueue();
	q_dama = makeQueue();
}

void DiscoDestroy(void) {
	destroyQueue(q);
	destroyQueue(q_varon);
	destroyQueue(q_dama);
}

Persona *nuevaPersona(char *nom) {
	Persona *persona = (Persona*) malloc(sizeof(Persona));
	persona->rdy = 0;
	persona->nom = nom;
	persona->par = NULL;
	return persona;
}

void asesinar(Persona* persona) {
	free(persona->par);
	free(persona);
}

char *dama(char *nom) {
	ocupar();
	Persona* dama = nuevaPersona(nom);
	if (!emptyQueue(q_varon)) {
		Persona* varon = get(q_varon);
		dama->par = varon->nom;
		varon->par = malloc(strlen(nom));
		strcpy(varon->par, nom);
		dama->rdy = 1;
		varon->rdy = 1;
		desocupar();
	} else {
		put(q_dama, &dama);
		desocupar();
		while(!dama->rdy) {
			printf("Ayuda!\n");;
		}
		printf("Sali!");
	}
	char* ret = malloc(sizeof(dama->par));
	strcpy(ret, dama->par);
	asesinar(dama);
	return ret;
}

char *varon(char *nom) {
	ocupar();
	Persona* varon = nuevaPersona(nom);
	if (!emptyQueue(q_dama)) {
		Persona* dama = get(q_dama);
		varon->par = dama->nom;
		dama->par = malloc(strlen(nom));
		strcpy(dama->par, nom);
		varon->rdy = 1;
		dama->rdy = 1;
		desocupar();
	} else {
		put(q_varon, &varon);
		desocupar();
		while(varon->rdy) {
			;
		}
	}
	char* ret = malloc(sizeof(varon->par));
	strcpy(ret, varon->par);
	asesinar(varon);
	return ret;
}