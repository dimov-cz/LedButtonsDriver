#ifndef I2CPWM_H
#define I2CPWM_H

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <wiringPi.h>
#include <wiringPiI2C.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/resource.h>

void setupI2CPWM(int i2c_address, int frequency, int range);
void finishI2CPWM();
void setPWM(int channel, int value);
void commit();

#endif // ifndef I2CPWM_H