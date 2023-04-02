#include "i2cInputs.h"

#define EVENT_PUSHED 1
#define EVENT_HOLDING 2
#define EVENT_RELEASED 3

int fd_inputs;
int inputs_state;
int inputs_holdTime[8];

int ic2_input_threadOnExit = 0;
pthread_t ic2_input_timer_thread;

void updateInputState()
{
    //inputs_state = wiringPiI2CReadReg8(fd_inputs, 0x00);
    inputs_state = wiringPiI2CRead(fd_inputs);
    wiringPiI2CWriteReg8(fd_inputs, 0x00, 0xff);//pull high
}

void i2c_input_ChangeEvent(int bit, int value)
{
    //0 == pushed, 1 == released
    
    if (value==0){
        inputs_holdTime[bit] = millis();
        i2cInputEvent(bit, EVENT_PUSHED, 0);
    }else{
        int holdtime = millis() - inputs_holdTime[bit];
        inputs_holdTime[bit] = 0;
        i2cInputEvent(bit, EVENT_RELEASED, holdtime);
    }
}

void i2c_input_TimerEvent(int bit, int value)
{
    if (value==0){
        int holdtime = millis() - inputs_holdTime[bit];
        i2cInputEvent(bit, EVENT_HOLDING, holdtime);
    }
}

static void i2c_input_EventHandler()
{
    pthread_setname_np(pthread_self(), "LedBD In I2C");
    int from = inputs_state;
    updateInputState();
    int change = inputs_state ^ from;
    if (change > 0){
        for(int i=0; i<8; i++){
            if (change & (1 << i)){
                i2c_input_ChangeEvent(i, (inputs_state >> i) & 1);
            }
        }
    }
}

void *ic2_input_timer_thread_body(void *args)
{
    pthread_setname_np(pthread_self(), "LedBD In Timed");
    int i;
    struct timespec tv;
    tv.tv_sec = 0;

	while (!ic2_input_threadOnExit)
	{
        for(i=0; i<8; i++){
            i2c_input_TimerEvent(i, (inputs_state >> i) & 1);
        }

        tv.tv_nsec = 10*1000*1000; //10ms
        nanosleep(&tv, NULL);
	}
    return NULL;
}

void setupI2CInputs(int i2cAddress, int intPin)
{
    if (wiringPiSetupGpio() < 0){
        perror("Unable to setup GPIO");
    }

    fd_inputs = wiringPiI2CSetup(i2cAddress);
    updateInputState();
    
    pinMode(intPin, INPUT);
    pullUpDnControl(intPin, PUD_UP);
    if(wiringPiISR(intPin, INT_EDGE_BOTH, &i2c_input_EventHandler) < 0){
        perror("Unable to setup ISR");
    }

    pthread_create(&ic2_input_timer_thread, NULL, ic2_input_timer_thread_body, NULL);
}

void finishI2CInputs(){
    ic2_input_threadOnExit = 1;
	pthread_join(ic2_input_timer_thread, NULL);
}