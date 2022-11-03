#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

typedef nThread     nTh;                        // this because if i don't do it vscode highlight leaves the chat
typedef long long    ll;

static nTh*         nTh_c;                      // thread of the one sharing
static int          counter;                    // counter of the readers waiting
static NthQueue*    nThQ_a;                     // queue of the one accessing


void nth_compartirInit(void) {
    START_CRITICAL 
    nTh_c   = NULL;
    counter = 0;
    nThQ_a  = nth_makeQueue();
    END_CRITICAL
}

void nSuicide() {
    START_CRITICAL
    nTh self = nSelf();
    if (nth_queryThread(nThQ_a, self)) {
        nth_delQueue(nThQ_a, self);
        counter--;
    }
    END_CRITICAL
}

void nWakeUp() {
    while(!nth_emptyQueue(nThQ_a)){
        nTh th = nth_getFront(nThQ_a);
        switch (th->status) {
            case WAIT_ACCEDER_TIMEOUT:
                printf("\t > WAKEUP! w/ time up\n");
                nth_cancelThread(th);
            case WAIT_ACCEDER:
                printf("\t > WAKEUP! no time up\n");
                nSuicide();
                setReady(th);
                break;

            default: 
                break;
        }
    }
}

void nCompartir(void *ptr) {
    START_CRITICAL
    nTh self    = nSelf();                      // Descriptor of current nThread
    self->ptr   = ptr;                          // Assigning the sharing pointer to its descriptor
    nTh_c       = &self;                        // Pointing int the global schema this thread

    if (!nth_emptyQueue(nThQ_a)) {
        nWakeUp();
    }
    while (nTh_c == &self) {
        suspend(WAIT_COMPARTIR);
        schedule();
    }
    END_CRITICAL
    return;
}

void *nAcceder(int ms) {
    void*   ret;
    START_CRITICAL
    counter++;                                  // +1 acceding the shared ptr
    if (!nTh_c) {                               // If there isn't sb already sharing
        nTh self = nSelf();                     // Get self descriptor
        nth_putBack(nThQ_a, self);              // We put this one in queue
        if (ms > 0) {                           // Given that it has a well defined timeout
            ll ns = ms * 1e6;                   // ms -> ns
            printf("\t > 1\n");
            suspend(WAIT_ACCEDER_TIMEOUT);      // wait as time  out
            printf("\t > 2\n");
            nth_programTimer(ms, nSuicide);     // TODO: parse function which makes it to deal with itself
            printf("\t > 3\n");
            if (ms)
                printf("\t > estoy mal hecho");
        } else {
            suspend(WAIT_ACCEDER);
        }
        printf("\t > 4\n");
        schedule();
        printf("\t > 5\n");

    }
    ret = nTh_c ? (*nTh_c)->ptr: NULL;
    END_CRITICAL
    return  ret;
}

void nDevolver(void) {
    START_CRITICAL
    counter = counter ? counter -1 : counter;   // counter >= 0
    if (!counter) {                             // Last call will do this
        if ((*nTh_c)->status != READY)          // Can't ready up an already Ready nThread
            setReady(*nTh_c);                   // Ready up nTh_c to return
        nTh_c = NULL;                           // We drop the nTh sharing
    }
    END_CRITICAL
    return;
}
