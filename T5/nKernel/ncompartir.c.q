#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

typedef nThread     nTh;                        // this because if i don't do it vscode highlight leaves the chat
typedef long long    ll;

static nTh*         nTh_c;                      // thread of the one sharing
static int          counter;                    // counter of the readers waiting
static Queue*       nThQ_a;                     // Normal Queue since timing + scheduling have issues with the nThreadQueue


void nth_compartirInit(void) {
    START_CRITICAL 
    nTh_c   = NULL;
    counter = 0;
    nThQ_a  = makeQueue();
    END_CRITICAL
}

void nCompartir(void *ptr) {
    START_CRITICAL
    nTh self    = nSelf();                      // Descriptor of current nThread
    self->ptr   = ptr;                          // Assigning the sharing pointer to its descriptor
    nTh_c       = &self;                        // Pointing int the global schema this thread

                                          
    while(!emptyQueue(nThQ_a)){                 // If it ins't empty then we gotta wake up them
        nTh th = get(nThQ_a);
        switch (th->status) {                   // Switch over, we are looking for acceder n acceder_timeout
            case WAIT_ACCEDER_TIMEOUT:
                nth_cancelThread(th);           // If it's a timed one, first cancel their thread
            case WAIT_ACCEDER:
                setReady(th);                   // Then we set it up ready
                break;
            default:                            // Any other case we don't care about we don't do anything
                break;                      
        }
    }

    while (nTh_c == &self) {                    // Waiting until it gets freed by nDevolver
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
    nTh self = nSelf();                         // Get self descriptor
    if (!nTh_c) {                               // If there isn't sb already sharing
        put(nThQ_a, self);                      // We put this one in queue
        if (ms > 0) {                           // Given that it has a well defined timeout
            ll ns = ms * 1e6;                   // ms -> ns
            suspend(WAIT_ACCEDER_TIMEOUT);      // wait as timeout
            nth_programTimer(ns, NULL);
        } else {
            suspend(WAIT_ACCEDER);              // wait as usual
        }
        schedule();                             // Schedule!
    }
    if (!nTh_c) {
    }
    ret     = nTh_c ? (*nTh_c)->ptr: NULL;      // If it timed out then there isn't sb sharing 
    counter = ret   ? counter      : counter-1; // If it timed out then we don't count it as acceding to the ptr

    if (!ret) {                                 // Quitamos a los timed out por que sino se vuelven punteros locos
        Queue* nq = makeQueue();
        while (!emptyQueue(nThQ_a)) {
            nTh th = get(nThQ_a);
            if (th != self)
                put(nq, th);
        }
        destroyQueue(nThQ_a);
        nThQ_a = nq;
    }
    END_CRITICAL
    return  ret;
}

void nDevolver(void) {
    START_CRITICAL
    counter = counter ? counter -1 : counter;   // we make sure counter >= 0

    if (!counter) {                             // Last call will do this
        if ((*nTh_c)->status != READY)          // Can't ready up an already Ready nThread
            setReady(*nTh_c);                   // Ready up nTh_c to return
        nTh_c = NULL;                           // We drop the nTh sharing
    }

    END_CRITICAL
    return;
}
