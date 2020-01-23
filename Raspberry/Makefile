INCLUDE	= -I/usr/local/include
INCLUDE_FLAGS=-I/usr/local/include
CFLAGS=-Wall -ggdb -std=c++0x
LDFLAGS	= -L/usr/local/lib -lmosquittopp -lmosquitto
LDLIBS  = -lwiringPi -lwiringPiDev -lpthread -lm -lrt	

all: temperature

temperature: main.o GPIO.o timestamp.a TestMqtt.o
	g++ -Wall main.o GPIO.o timestamp.a TestMqtt.o -g -o temperature $(INCLUDE) $(LDFLAGS) $(LDLIBS)

main.o: main.cpp TestMqtt.o
	g++ main.cpp -Wall -Wextra -g -c $(INCLUDE) $(LDFLAGS) $(LDLIBS) 
	
GPIO.o: GPIO.cpp GPIO.h 
	g++ -Wall -c -g GPIO.cpp 
	
timestamp.o: timestamp.cpp timestamp.h
	g++ -g -c timestamp.cpp -Wall -Wextra -o2 -o timestamp.o

timestamp.a: timestamp.o
	ar rcs timestamp.a timestamp.o

TestMqtt.o: TestMqtt.cpp
	g++ -c $^ -o $@ ${CFLAGS}
	
	

clean:
	rm -f *.o temperature timestamp.a
