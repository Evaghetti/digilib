#ifndef W0RLD_H
#define W0RLD_H

int prepareDCOMLogic();

void releaseDCOMLogic();

int readDataDCOM(void* dst, int sizeDst);

int writeDataDCOM(const void* dst, int sizeDst);

int doBattleWithDCOM();

#endif