#include "i2cPWM.h"



#define CHANNELS_COUNT 8
#define EXECPLAN_MAXSTEPS CHANNELS_COUNT+3 //start + end + stopper

int fd_i2c_output;

pthread_t timer_thread;
pthread_mutex_t mutex;
int threadOnExit =0;

int FREQ;
int RANGE;
struct timespec tvTick;

int channelsValues[CHANNELS_COUNT*10];

typedef struct {
    int time;
    int value;
} ExecutionStep;

int execPlanState = 0;
ExecutionStep executionPlan[EXECPLAN_MAXSTEPS];
ExecutionStep executionPlanThread[EXECPLAN_MAXSTEPS];


void sendPWM(int registerIdx, unsigned char data)
{
    int lastDataSent = -1;
    if (data != lastDataSent){
        wiringPiI2CWrite(fd_i2c_output, data);
        lastDataSent = data;
    }   
}

int calcNextStepOfExecutionPlan(ExecutionStep execPlan[], int stepIdx, int tick)
{
    int nextStepChannelIdx = -1;
    executionPlan[stepIdx].value = 0;
    for(int i=0; i<8; i++)
    {
        if (channelsValues[i] > tick)
        {
            executionPlan[stepIdx].value |= 1 << i;
            if (nextStepChannelIdx == -1 || channelsValues[i]<channelsValues[nextStepChannelIdx])
            { 
                nextStepChannelIdx = i;
            }
        }
    }
    int nextTick;
    if (nextStepChannelIdx != -1)
    {
        nextTick = channelsValues[nextStepChannelIdx];
    }else{
        nextTick = RANGE - 1;
    }
    executionPlan[stepIdx].time = (nextTick - tick) * tvTick.tv_nsec;
    return nextTick;
}

void buildExecutionPlan(ExecutionStep execPlan[])
{
    int currentStepIdx = 0;
    int tick = 0;
    while (tick < RANGE-1){
        tick = calcNextStepOfExecutionPlan(execPlan, currentStepIdx, tick);
        currentStepIdx++;
    }
    executionPlan[currentStepIdx].time = 0;
}

void i2cpwm_changeAffinity()
{
    static int coresCount = sysconf(_SC_NPROCESSORS_CONF);
    static int currentCore = -1;
    if (currentCore == -2){
        return;
    }

    currentCore++;
    if (currentCore >= coresCount) currentCore = 0;
    
    printf("Jumping to core %i\n", currentCore+1);
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(currentCore, &mask);
    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) == -1) {
        perror("sched_setaffinity failed");
    }

    struct sched_param param;
    int policy;
    // Get the current scheduling policy
    policy = sched_getscheduler(0);
    // Change the scheduling policy to SCHED_FIFO
    param.sched_priority =  sched_get_priority_max(SCHED_FIFO);
    if(sched_setscheduler(0, SCHED_FIFO, &param) == -1) {
        perror("Unable to set RT priority, must be runned as root to work. Flickering expected.");
        currentCore = -2;
        return;
    }
    // Set the real-time priority to 99
    param.sched_priority = 99;
    if(sched_setparam(0, &param) == -1) {
        perror("sched_setparam");
        return;
    }

    printf("Scheduling policy set to SCHED_FIFO, priority set to 99.\n");
}

void *timer_thread_body(void *args)
{
    pthread_setname_np(pthread_self(), "LedBD Out PWM");   

    int i;
    struct timespec tv;

    i2cpwm_changeAffinity();
    int checkStabilityFactor;
    int checkWorkTimeSum = 0;
    int checkClockTime = 0;
    int checkWarning = 0;
    int checkEach = 1;
    

    executionPlanThread[0].time = 0;//empty
    int loopCounter = 0;
	while (!threadOnExit)
	{
        loopCounter++;
        //exec plan update:
        if (execPlanState == 2){ //avoid locking first
            //pthread_mutex_lock(&mutex);
            if (pthread_mutex_trylock(&mutex) == 0) {
                if (execPlanState == 2){//test again in mutex
                    //printf("UPDATE\n");
                    memcpy(executionPlanThread, executionPlan, sizeof(ExecutionStep) * EXECPLAN_MAXSTEPS);
                    execPlanState = 0;
                }
                pthread_mutex_unlock(&mutex);
            }
        }

        checkWorkTimeSum = 0;
        checkClockTime = ( ((loopCounter%checkEach) == 0)) ? micros() : 0;
        //exec plan fire:
        i=0;
        while (executionPlanThread[i].time>0){
            sendPWM(0, executionPlanThread[i].value);
            tv.tv_sec = 0;
            tv.tv_nsec = executionPlanThread[i].time;
            nanosleep(&tv, NULL);

            checkWorkTimeSum += executionPlanThread[i].time/1000;//ns->us
            i++;
        }
        //check stability
        if (checkClockTime){
            checkClockTime = micros() - checkClockTime;

            if (checkWorkTimeSum > 0){
                checkStabilityFactor = checkClockTime*1000 / checkWorkTimeSum;
                if (checkStabilityFactor>1500){
                    checkWarning += checkStabilityFactor/100;
                    //printf("%i %i\n", checkStabilityFactor, checkWarning);
                    if (checkWarning > 100){
                        i2cpwm_changeAffinity();
                        checkWarning = 0;
                    }
                }else{
                    checkWarning--;
                    if (checkWarning<0) checkWarning=0;
                }
            }
        }
	}
    return NULL;
}



void setupParams(int frequency, int range)
{
    FREQ = frequency;
    RANGE = range;

    tvTick.tv_sec = 0;
    tvTick.tv_nsec = round(1.0 / (double)(FREQ * RANGE) *1000000000.0);
}

/**
 * base freq 64 is sufficient, range 256 is ok
*/
void setupI2CPWM(int i2c_address, int frequency, int range) 
{
    setupParams(frequency, range);

    fd_i2c_output = wiringPiI2CSetup(i2c_address);

    pthread_mutex_init(&mutex, NULL);
    pthread_create(&timer_thread, NULL, timer_thread_body, NULL);
    
}

void finishI2CPWM(){
    threadOnExit = 1;
	pthread_join(timer_thread, NULL);
    pthread_mutex_destroy(&mutex);
}

/**
 * 0 - 255
*/
void setPWM(int channel, int value)
{
    if (channel<0 || channel>7) return;
    if (value<0) value = 0;
    if (value>255) value = 255;

    //normalize to RANGE, max value of RANGE == always on
    if (RANGE != 256){ //optimization
        channelsValues[channel] = round((float)value/255 * (RANGE-1));
    }else{
        channelsValues[channel] = value;
    }
}

void commit()
{
    pthread_mutex_lock(&mutex);
    execPlanState = 1;
    pthread_mutex_unlock(&mutex);

    buildExecutionPlan(executionPlan);

    pthread_mutex_lock(&mutex);
    execPlanState = 2;
    pthread_mutex_unlock(&mutex);
}