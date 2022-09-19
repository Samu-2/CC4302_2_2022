typedef struct {
    double meta; 
    pthread_mutex_t m;
    pthread_cond_t c;
} Colecta;


Colecta *nuevaColecta(double meta) {
    Colecta *colecta = (Colecta*)malloc(sizeof(Colecta));
    colecta->meta = meta;
    pthread_mutex_init(&colecta->m, NULL);
    pthread_cond_init(&colecta->c, NULL);
    return colecta;
}


double aportar(Colecta *colecta, double monto) {
    pthread_mutex_lock(&colecta->m);
    
    if(monto < colecta->meta)
        colecta->meta -= monto;
    else {
        monto= colecta->meta;
        colecta->meta = 0;
        pthread_cond_broadcast(&colecta->c);
    }
    
    while (colecta->meta > 0)
        pthread_cond_wait(&colecta->c, &colecta->m);
        
    pthread_mutex_unlock(&colecta->m);
    return monto;    

};





