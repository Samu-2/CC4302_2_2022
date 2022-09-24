#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include "bolsa.h"

//Informacion logica del problema
int exisVendedor = 0;
int isVendido = 0;

//Informacion vendedor 
char* vendedorGlob;
int precioGlob;

//informacion comprador 
char *compradorGlob;

//Pthread mutex and cond
static pthread_cond_t c = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;

int vendo(int precio, char *vendedor, char *comprador) {
    //if not exist seller, so push the new seller in the market
    while (isVendido == 1){
        pthread_cond_wait(&c,&m);
    }
    //printf("inicio vendedor %s y con una oferta de: %i\n ",vendedor,precio);
    pthread_mutex_lock(&m);
    if ((exisVendedor == 0) || (precioGlob > precio)){
        //we subtitute the var global for information of new seller
        vendedorGlob = vendedor;
        precioGlob = precio;
        exisVendedor = 1; //now exist seller, so "exisVendor" is 1
        printf("precio remplazado, precio actual es: %i, lo vende: %s\n", precioGlob,vendedor);
        pthread_cond_broadcast(&c);
    }
    else{
        pthread_mutex_unlock(&m);
        return 0;
    }
    pthread_cond_wait(&c,&m);
    //printf("desperto el vendedor %s\n",vendedor);

    if(isVendido == 1){
        strcpy(comprador,compradorGlob);
        printf("fue comprado por: %i\n",precio);
        precioGlob = 0;
        isVendido = 0;
        exisVendedor = 0;
        pthread_mutex_unlock(&m);
        return 1;
    }
    pthread_mutex_unlock(&m);
    return 0;

}

int compro(char *comprador, char *vendedor) {
    pthread_mutex_lock(&m);
    while (isVendido == 1){
        pthread_cond_wait(&c,&m);
    }
    //printf("inicio comprador %s \n",comprador);
    if (exisVendedor == 1){
        strcpy(vendedor,vendedorGlob);
        compradorGlob = comprador;
        int pricePay = precioGlob;
        isVendido = 1;
        //printf("precio pagado por %s: %i\n",comprador , pricePay);
        pthread_cond_broadcast(&c);
        pthread_mutex_unlock(&m);
        return pricePay;
    }
    //printf("no hay nada a la venta cuando el comprado: %s, paso \n",comprador);
    pthread_mutex_unlock(&m);
    return 0;

}