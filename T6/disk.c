#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "disk.h"
#include "priqueue.h"
#include "spinlocks.h"

enum relativePosition   {BEHIND, FOWARD};
enum armState           {IDLE = -1};
PriQueue*               armSchedule[2];  
int spinScheduler, armTrack;

void iniDisk(void) {
    armSchedule[BEHIND] = makePriQueue();
    armSchedule[FOWARD] = makePriQueue();
    spinScheduler       = OPEN;
    armTrack            = IDLE;
}

void requestDisk(int track) {
    spinLock(&spinScheduler);
    if (armTrack != IDLE) {
        int spinWait= CLOSED;
        int relPos  = track >= armTrack ? FOWARD: BEHIND;
        priPut(armSchedule[relPos], &spinWait, track);
        spinUnlock(&spinScheduler);
        spinLock(&spinWait);
        spinLock(&spinScheduler);
    }
    armTrack = track;
    spinUnlock(&spinScheduler);
    return;
}

void releaseDisk() {
    spinLock(&spinScheduler);
    if (emptyPriQueue(armSchedule[FOWARD])){
        PriQueue* swapPri   = armSchedule[FOWARD];
        armSchedule[FOWARD] = armSchedule[BEHIND];
        armSchedule[BEHIND] = swapPri;
    }
    if (!emptyPriQueue(armSchedule[FOWARD])) {
        int *spinWaiting = priGet(armSchedule[FOWARD]);
        spinUnlock(spinWaiting);
    }
    else armTrack = IDLE;
    spinUnlock(&spinScheduler);
    return;
}