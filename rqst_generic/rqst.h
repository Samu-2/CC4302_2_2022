

// Hold
typedef struct rqstHold RqstHold;
void        rqst_HoldOn(RqstHold* rqstHold);
int         rqst_HoldOff(RqstHold* rqstHold);
RqstHold*   rqst_HoldInit();
int         rqst_HoldFree(RqstHold* rqstHold);