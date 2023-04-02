#include "i2cPWM.h"
#include "i2cInputs.h"

#include <stdio.h>
#include <stdlib.h>


#include <unistd.h>
#include <math.h>
#include "LedAnimation/LedAnimation.h"
#include <chrono>

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <sys/resource.h>
#include <csignal>

#include "UnixSocketTool/UnixSocketServer.h"

#define I2C_ADDRESS_LIGHTS 0x20
#define GPIO_INTERUPT 4
#define I2C_ADDRESS_SWITCHES 0x21

int exiting = 0;

UnixSocketServer unixSocketIf = UnixSocketServer();
LedAnimation animators[4];


unsigned long getCurrentTimeMillis() {
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
    auto epoch = ms.time_since_epoch();
    return static_cast<unsigned long>(epoch.count());
}

unsigned long timeFreeze = 0;
unsigned long getDrivenTimeMillis() {
    if (timeFreeze>0){
        return timeFreeze;
    }else{
        return getCurrentTimeMillis();
    }
}

void freezeTime(){
    //timeFreeze = getCurrentTimeMillis();
}
void unfreezeTime(){
    //timeFreeze = 0;
}

void initAnimators()
{
    freezeTime();
    for(int i=0; i<4; i++){
        animators[i] = LedAnimation();
        animators[i].setTimeCallback(getDrivenTimeMillis);
        LedStripState ledState;
        ledState.level = 100;
        ledState.r = 255;
        ledState.g = 255;
        ledState.b = 255;
        ledState.cw = 255;
        ledState.ww = 255;
        animators[i].UpdateMode1(ledState);
        animators[i].SetLevel(25);
        animators[i].SetMode(1);//preset 1 will be default state
    }
    unfreezeTime();
}

void setAnimatorsStaticPresets()
{
    animators[0].UpdatePresetDefinition(0, "[300,100,,,,,255]");//cold white
    animators[0].UpdatePresetDefinition(1, "[300,100,,255]");//green
    
    animators[1].UpdatePresetDefinition(0, "[300,100,,,,,255]");//cold white
    animators[1].UpdatePresetDefinition(1, "[300,100,,255]");//green

    animators[2].UpdatePresetDefinition(0, "[300,100,,,255]");//blue
    animators[2].UpdatePresetDefinition(1, "[300,100,255,255]");//orange

    animators[3].UpdatePresetDefinition(0, "[300,100,,,,,255]");//cold white
    animators[3].UpdatePresetDefinition(1, "[300,100,,255]");//green


    animators[0].UpdatePresetDefinition(7, "[0,0,255,255,255,255,255],[0000,0],[500,100],[500,0],[1500]");//cold white
    animators[1].UpdatePresetDefinition(7, "[0,0,255,255,255,255,255],[0500,0],[500,100],[500,0],[1000]");//cold white
    animators[2].UpdatePresetDefinition(7, "[0,0,255,255,255,255,255],[1000,0],[500,100],[500,0],[0500]");//blue
    animators[3].UpdatePresetDefinition(7, "[0,0,255,255,255,255,255],[1500,0],[500,100],[500,0],[0000]");//cold white
}

void animatorsPWMupdate()
{
    static int update;
    static LedStripState ledState;
    update = 0;
    freezeTime();
    if (animators[0].loopTick(&ledState)){
        setPWM(1, ledState.cw);
        setPWM(0, ledState.g);
        update = 1;
    }
    if (animators[1].loopTick(&ledState)){
        setPWM(2, ledState.g);
        setPWM(3, ledState.cw);
        update = 1;
    }
    if (animators[2].loopTick(&ledState)){
        setPWM(4, ledState.r);//orange led actually
        setPWM(5, ledState.b);
        update = 1;
    }
    if (animators[3].loopTick(&ledState)){
        setPWM(6, ledState.g);
        setPWM(7, ledState.cw);
        update = 1;
    }
    unfreezeTime();
    if (update) commit();
}

std::string readToken(const std::string &input, std::string &token, const char delimiter)
{
    int i = input.find(',');
    if (i == std::string::npos){
        token = input.substr(0, i);
        return "";
    }
    token = input.substr(0, i);
    return input.substr(i+1);
}
// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

void i2cInputEvent(int id, int event, int time){
    char buffer[1024];
    snprintf(buffer,1024,"input,%i,%i,%i\n", id, event, time);
    unixSocketIf.SendToAll(buffer);
}

void signal_handler(int signal_num) {
    if(signal_num == SIGINT) {
        std::cout << "Caught SIGINT signal" << std::endl;
        exiting = 1;
    }
    else if(signal_num == SIGKILL) {
        std::cout << "Caught SIGKILL signal" << std::endl;
        exiting = 2;
    }
}

int main()
{
    signal(SIGINT, signal_handler);
    signal(SIGKILL, signal_handler);


    const char *env_var_socketPath;
    env_var_socketPath = getenv("LD_SOCKET");

    if(env_var_socketPath == NULL) {
        env_var_socketPath = "/tmp/LedButtonsDriver.sock";
    }

    setupI2CInputs(I2C_ADDRESS_SWITCHES, GPIO_INTERUPT);
    setupI2CPWM(I2C_ADDRESS_LIGHTS, 200, 256);
    
    initAnimators();
    setAnimatorsStaticPresets();

    unixSocketIf.Open(env_var_socketPath);
    printf("Socket opened at %s.\n", env_var_socketPath);

    pthread_setname_np(pthread_self(), "LedBD Main");

    char buffer[1024];
    
	while (exiting == 0)
	{
        animatorsPWMupdate();

        int recLength = unixSocketIf.Loop(buffer, sizeof(buffer)-1, "HELO");
        if (recLength > 0){
            buffer[recLength] = '\0';
            printf("MSG: %s\n", buffer);
            try {
                std::string input = buffer;
                std::string token = "";
                rtrim(input);
                input = readToken(input, token, ',');
                if (token == "ledstrip")
                {
                    input = readToken(input, token, ',');
                    int animIdx =  std::stoi(token);
                    if (animIdx>=0 && animIdx<4){
                        animators[animIdx].processCommand(input);
                    }
                }
            } catch (const std::invalid_argument& e) {
            } catch (const std::out_of_range& e) {
            }
        }

		usleep(20 * 1000);    
	}

    std::cout << "Exiting" << std::endl;   
    finishI2CPWM();
    finishI2CInputs();
    unixSocketIf.Close();
    unlink(env_var_socketPath);
    std::cout << "Bye" << std::endl;   

	return 0;
}