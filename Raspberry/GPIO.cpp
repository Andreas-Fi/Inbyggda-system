
#include "GPIO.h"

using namespace std;

GPIO::GPIO()
	:m_ValueFD(-1),m_DirectionFD(-1),m_ExportFD(-1),m_UnexportFD(-1),m_GPIONumber("4")
{
    //GPIO4 is default
	Export();
}

GPIO::GPIO(string GPIONumber)
	:m_ValueFD(-1),m_DirectionFD(-1),m_ExportFD(-1),m_UnexportFD(-1),m_GPIONumber(GPIONumber)
{
	//Instatiate GPIOClass object for GPIO pin number "GPIONumber"
	Export();
}

GPIO::~GPIO()
{
	Unexport();
}


int GPIO::Export()
{
	int statusValue = -1;

	string exportStream = "/sys/class/gpio/export";
	m_ExportFD = statusValue = open(exportStream.c_str(),  O_WRONLY);
	if (statusValue < 0)
	{
		perror("could not open SYSFS GPIO export device");
        exit(1);
	}
	
	stringstream ss;
	ss << m_GPIONumber;
	string numberStream = ss.str();

	statusValue = write(m_ExportFD, numberStream.c_str(), numberStream.length());
	if (statusValue < 0)
	{
		perror("could not write to SYSFS GPIO export device");
        exit(1);
	}
	
	statusValue = close(this->m_ExportFD);
	if (statusValue < 0)
	{
		perror("could not close SYSFS GPIO export device");
        exit(1);
	}

    return statusValue;
}

int GPIO::Unexport()
{
	int statusValue = -1;
	string unexportStream = "/sys/class/gpio/unexport";
	m_UnexportFD = statusValue = open(unexportStream.c_str(),  O_WRONLY);
	if (statusValue < 0)
	{
		perror("could not open SYSFS GPIO unexport device");
        exit(1);
	}

	stringstream ss;
	ss << this->m_GPIONumber;
	string numberStream = ss.str();
	statusValue = write(m_UnexportFD, numberStream.c_str(), numberStream.length());
	if (statusValue < 0)
	{
		perror("could not write to SYSFS GPIO unexport device");
        exit(1);
	}
	
	statusValue = close(m_UnexportFD);
	if (statusValue < 0)
	{
		perror("could not close SYSFS GPIO unexport device");
        exit(1);
	}
	
	return statusValue;
}

int GPIO::SetDirection(string direction)
{
	int statusValue = -1;
	string setdirStr ="/sys/class/gpio/gpio" + m_GPIONumber + "/direction";
	
	
	statusValue = open(setdirStr.c_str(), O_WRONLY); // open direction file for gpio
	m_DirectionFD = statusValue;
	if (statusValue < 0)
	{
		perror("could not open SYSFS GPIO direction device");
        exit(1);
	}
		
	if (direction.compare("in") != 0 && direction.compare("out") != 0 ) 
	{
		fprintf(stderr, "Invalid direction value. Should be \"in\" or \"out\". \n");
		exit(1);
	}
		
	statusValue = write(m_DirectionFD, direction.c_str(), direction.length());
	if (statusValue < 0)
	{
		perror("could not write to SYSFS GPIO direction device");
        exit(1);
	}
	
	statusValue = close(m_DirectionFD);
	if (statusValue < 0)
	{
		perror("could not close SYSFS GPIO direction device");
        exit(1);
	}
	
	return statusValue;
}


int GPIO::SetValue(string value)
{

    int statusValue = -1;
	string valueStream = "/sys/class/gpio/gpio" + m_GPIONumber + "/value";
	
	m_ValueFD = statusValue = open(valueStream.c_str(), O_WRONLY);
	if (statusValue < 0)
	{
		perror("could not open SYSFS GPIO value device");
        exit(1);
	}
		
	if (value.compare("1") != 0 && value.compare("0") != 0 ) 
	{
		fprintf(stderr, "Invalid  value. Should be \"1\" or \"0\". \n");
		exit(1);
	}
		
	statusValue = write(m_ValueFD, value.c_str(), value.length());
	if (statusValue < 0)
	{
		perror("could not write to SYSFS GPIO value device");
        exit(1);
	}
	
	statusValue = close(m_ValueFD);
	if (statusValue < 0)
	{
		perror("could not close SYSFS GPIO value device");
        exit(1);
	}

	    return statusValue;
}


int GPIO::GetValue(string& outValue){

	string getValStr = "/sys/class/gpio/gpio" + m_GPIONumber + "/value";
	char buffert[10];
	int statusValue = -1;
	m_ValueFD = statusValue = open(getValStr.c_str(), O_RDONLY);
	if (statusValue < 0)
	{
		perror("could not open SYSFS GPIO value device");
        exit(1);
	}

	statusValue = read(m_ValueFD, &buffert, 1);
	if (statusValue < 0)
	{
		perror("could not read SYSFS GPIO value device");
        exit(1);
	}
	
	buffert[1]='\0';
	
	outValue = string(buffert);
	
	if (outValue.compare("1") != 0 && outValue.compare("0") != 0 ) 
	{
		fprintf(stderr, "Invalid  value read. Should be \"1\" or \"0\". \n");
		exit(1);
	}
	
	statusValue = close(m_ValueFD);
	if (statusValue < 0)
	{
		perror("could not close SYSFS GPIO value device");
        exit(1);
	}

	return statusValue;
}


string GPIO::GetGPIONumber()
{
	return m_GPIONumber;
}
