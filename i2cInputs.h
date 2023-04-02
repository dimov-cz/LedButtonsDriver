#ifndef I2CINPUTS_H
#define I2CINPUTS_H

#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <pthread.h>
#include <sched.h>
#include <errno.h>
#include <stdio.h>

extern void i2cInputEvent(int id, int event, int time);
/*{
    std::cout << "Event: " << id << "  " << event << " " << time << "\n";
}*/

void setupI2CInputs(int i2cAddress, int intPin);
void finishI2CInputs();

#endif 