void quicksort_seq(int a[], int i, int j) {
    if (i < j) {
        int h = particionar(a, i, j);
        quicksort_seq(a, i, h-1);
        quicksort_seq(a, h+1, j);
    }
}

typedef struct {
    int a[];
    int i, j;
    int n;
} Args;

void *thread_function(void *p){
    Args *arg = (Args*) p;
    quicksort(arg->a, arg->i, arg->j, arg->n);
    return NULL;
}

void quicksort(int a[], int i, int j, int n) {
    if (i < j) {
        if (n==1) {
            quicksort_seq(a, i, j);
        else {
            int h = particionar(a, i, j);
            pthread_t pid;
            Args args = {a, i, h-1, n/2};
            pthread_create(&pid, NULL, thread_function, &args);
            quicksort(a, h+1, j, n -(n/2));
            pthread_join(pid, NULL);
        }
    }
}
