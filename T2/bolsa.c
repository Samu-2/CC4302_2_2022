#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "bolsa.h"

//Global Variables
//Logic
int *isSold; //if is 1, so was sold

//Info of seller
char **nameSeller;
int *priceSeller = NULL; //Cond initial is Null

//Info of buyer
char **nameBuyer;

//Cond and Mutex of pthread
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t c = PTHREAD_COND_INITIALIZER;

int vendo(int precio, char *vendedor, char *comprador) {
    pthread_mutex_lock(&m);
    int* myPrice = malloc(sizeof(int));
    *myPrice = precio;//init: our price
    int* myIsSold = malloc(sizeof(int));
    *myIsSold = 0; //init: false
    char** myBuyer = malloc(sizeof(char*));
    *myBuyer = comprador;//init: this will modify our buyer
    char** myseller = malloc(sizeof(char*));
    *myseller = vendedor;//init: our name

    if(priceSeller == NULL){//if not exist a seller, so we change the global data with the own data
        isSold = myIsSold;
        priceSeller = myPrice;
        nameSeller = myseller;
        nameBuyer = myBuyer;
    }
    else if(*priceSeller > *myPrice){//if we have better price than the actually seller, so we change the global data with the own data
        isSold = myIsSold;
        priceSeller = myPrice;
        nameSeller = myseller;
        nameBuyer = myBuyer;
        pthread_cond_broadcast(&c);// and we wake up the previus seller
    }

    pthread_cond_wait(&c,&m);
    int t = 0; //comparator
    if(*myIsSold == 1){
        t = 1; //now we return 1 
    }
    //we free the memory
    free(myPrice);
    free(myBuyer);
    free(myIsSold);
    free(myseller);
    pthread_mutex_unlock(&m); //Our information is the pointer 
    return t;
}


int compro(char *comprador, char *vendedor) {
    pthread_mutex_lock(&m);
    int r = 0;
    if (priceSeller != NULL) {
      //we obtain the seller´s data
      strcpy(vendedor,*nameSeller);
      strcpy(*nameBuyer,comprador);//now the seller has the seller's name
      r = *priceSeller;
      *isSold = 1;
      //and change the buyer's data with the own

      //we erase the pointer (the seller has this info)
      isSold = NULL;
      nameBuyer = NULL;
      nameSeller = NULL;
      priceSeller = NULL;
      pthread_cond_broadcast(&c);
    }
    pthread_mutex_unlock(&m);
    return r;
}