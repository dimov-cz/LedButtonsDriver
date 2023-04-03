

all:
	#g++ i2cPWM.c -O2 -o i2cPWM.o -c
	#g++ LedAnimation/LedAnimation.cpp LedAnimation/LedAnimation_structs.cpp -o LedAnimation.o

	g++ main.cpp \
	 i2cPWM.c i2cInputs.c \
	 LedAnimation/LedAnimation.cpp LedAnimation/LedAnimation_structs.cpp LedAnimation/DefinitionParser.cpp \
	 UnixSocketTool/UnixSocketServer.cpp \
	 -Ofast -lwiringPi -lpthread -lm -o ledButtonsDriverDaemon

run:
	#avoid core 2?
	sudo taskset -c 0,3  chrt 99 ./ledButtonsDriverDaemon
	#sudo taskset -c 2  chrt 99 ./test123

reinstall:
	systemd/service.uninstall
	systemd/service.install