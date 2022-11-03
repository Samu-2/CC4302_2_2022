#define _XOPEN_SOURCE 500

#include "nthread-impl.h"

// Use los estados predefinidos WAIT_ACCEDER y WAIT_COMPARTIR
// El descriptor de thread incluye el campo ptr para que Ud. lo use
// a su antojo.
// ... defina aca sus variables globales con el atributo static ...
// nth_compartirInit se invoca al partir nThreads para Ud. inicialize
// sus variables globales

static nThread*     _nth_c;
static int          _counter;
static NthQueue*    _qnth_a;

void nth_compartirInit(void) {
    START_CRITICAL
    _qnth_a = nth_makeQueue();
    _counter= 0;
    _nth_c  = NULL;
    END_CRITICAL
    return;
}

void nDespertar(NthQueue* nthq) {
    while(!nth_emptyQueue(nthq)){
        nThread th = nth_getFront(nthq);
        setReady(th);
    }
}

void nCompartir(void *ptr) {
    START_CRITICAL
    nThread self = nSelf();
    self->ptr = ptr;
    _nth_c = &self;

    if (nth_emptyQueue(_qnth_a)) {
        suspend(WAIT_COMPARTIR);
        schedule();
    } else {
        nDespertar(_qnth_a);
    }
     
    while (_nth_c == &self) {
        suspend(WAIT_COMPARTIR);
        schedule();
    }
    
    END_CRITICAL
    return;
}

void *nAcceder(int max_millis) {
    START_CRITICAL
    _counter++;
    if (!_nth_c) {
        nThread self = nSelf();
        nth_putBack(_qnth_a, self);
        suspend(WAIT_ACCEDER);
        schedule();
    } else {
        if ((*_nth_c)->status != READY) setReady(*_nth_c);
    }
    void* ret = (*_nth_c)->ptr;
    END_CRITICAL
    return ret;
}

void nDevolver(void) {
    // Notifica que los datos compartidos ya no se
    // usarán.
    START_CRITICAL
    _counter = _counter ? _counter -1 : _counter;
    if (!_counter) {
        if ((*_nth_c)->status != READY) setReady(*_nth_c);
        _nth_c = NULL;
    }
    END_CRITICAL
    return;
}
