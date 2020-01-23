
#ifndef GPIO_H
#define GPIO_H

#include <string>
#include <iostream>
#include <sstream>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;
/* GPIO Class
 * Purpose: Each object instatiated from this class will control a GPIO pin
 * The GPIO pin number must be passed to the overloaded class constructor
 */
class GPIO
{
public:
	GPIO();
	GPIO(string GPIONumber);
	~GPIO();
	
	// 
    int SetDirection(string direction);
    int SetValue(string value);
    int GetValue(string& outValue);
    string GetGPIONumber();

private:
    int Export();
	int Unexport();
	
	int m_ValueFD;
	int m_DirectionFD;
	int m_ExportFD;
	int m_UnexportFD;
	string m_GPIONumber;
	
};

#endif
