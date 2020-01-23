#include <stdio.h>
#include <ctime>
#include <string>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <iostream>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <thread>

#include <wiringSerial.h>

#include <mosquitto.h>

#include "GPIO.h"
#include "timestamp.h"
#include "TestMqtt.h"

#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"

#define MaxDevice 24

#define MQTT_HOST       "localhost"
#define MQTT_PORT       1883
#define TARGET_TOPIC    "novia/data"

char device[MaxDevice][15];
bool runningThreads[MaxDevice];
string aggregatorID;

using namespace std;
using namespace rapidjson;

int stopListeningThread = 0;
bool debug = 0;
double targetTemperature[MaxDevice], targetOxygen[MaxDevice];

void ctrlC_Handler(sig_atomic_t s)
{
	//printf("Caught signal %d\n",s);
	stopListeningThread = 1;
	int fd;
	fd = open("/sys/class/gpio/unexport", O_WRONLY);
	char gpio[MaxDevice][3] = { "21", "20", "16", "12", "7\0" , "8\0", "25", "24", "23", "18", "15", "26", "29", "13", "6\0", "5\0", "11", "9\0", "10", "22", "27", "17", "4\0", "3\0" }; //...
	for(int i = 0; i < MaxDevice; i++)
	{
		write(fd,gpio[i],2);
	}
	close(fd);
	mosqpp::lib_cleanup();
	exit(0);
}

/// gpioNr = which gpio pin the devices emergency led will use
/// device = which tty the collector is connected to
/// deviceNr = device id used for both the target
///				value arrays and for device uid (+1)
void main_Program(char* gpioNr, char* device, int deviceNr)
{
	int fd;
	GPIO leds = GPIO(gpioNr);

	leds.SetDirection("out");
	leds.SetValue("0");

	if ((fd = serialOpen(device, 115200)) < 0)
	{
		fd = open("/sys/class/gpio/unexport", O_WRONLY);
		write(fd,"23",2);
		close(fd);
		if ((fd = serialOpen(device, 115200)) < 0)
		{
			fprintf(stderr, "Unable to open serial device %s: %s\n", device, strerror(errno));
			return;
		}
		return;
	}

	try
    {
		class TestMqtt* mqtt;
		int iReturnCode;

		mosqpp::lib_init();

		mqtt = new TestMqtt("novia_data", TARGET_TOPIC, MQTT_HOST, MQTT_PORT);
		mqtt->debug = debug;

		int uid = deviceNr + 1;
		
		
		string lastResponse = "";
		
		for (; stopListeningThread == 0;)
		{
			/*double nomTemperature = 0, nomOxygen = 0;
			nomTemperature = (128*targetTemperature[deviceNr]+12800)/25;
			nomOxygen = ((256*targetOxygen[deviceNr]-25600)/25)*-1;*/
			
			char rec;
			char str[100] = {'\0'};
			

			while(str[0] != '{' && str[strlen(str)-1] != '}')
			{
				for(int ii = 0; true; ii++)
				{
					rec = serialGetchar(fd);
					if(rec == 10)
					{
						break;
					}
					str[ii] = rec;
					sleep(1/115200); //Ts = 1/115200 = ~8,68 us
				}
				if(debug)
				{
					printf("rec: %s\n",str);
				}
			}

			Document doc;
			ParseResult parseResult = doc.Parse(str);

			if(parseResult)
			{				
				if(doc.HasMember("message"))
				{
					char msg[100] = {'\0'};
					strcpy(msg,doc["message"].GetString());

					const char whoami[] = "who am i";
					if(strcmp(msg,whoami) == 0)
					{
						if(debug)
						{
							printf("Who am i received");
						}
						
						char init[100] = "\0";
						//sprintf(init,"{\"uid\":%d,\"nomtemperature\":%f1,\"nomoxygen\":%f1,\"power\":0}",uid,targetTemperature,targetOxygen);
						sprintf(init,"{\"uid\":%d,\"nomtemperature\":%f1,\"power\":0}",uid,targetTemperature[deviceNr]);

						serialPrintf(fd,init);
						if(debug)
						{
							printf("Who am i received and responded with:\n\t%s\n",init);
						}
					}
				}
				else if(doc.HasMember("temperature") && doc.HasMember("oxygen") && doc.HasMember("uid") && doc.HasMember("power"))
				{
					int temperatureAvg = 0, oxygenAvg = 0, uid = 0, power = 0;
					temperatureAvg = doc["temperature"].GetInt();
					oxygenAvg = doc["oxygen"].GetInt();
					uid = doc["uid"].GetInt();
					power = doc["power"].GetInt();

					//double ohmPerC = (double)100 / (double)1024;
					double temperature = ((25*temperatureAvg)/128)-100;//ohmPerC * ((1024 - (1024*(temperatureAvg*ohmPerC*2)/100)) * -1);
					double oxygen = (-((25*oxygenAvg)/256))+100; //ohmPerC * (1024 - (1024*(oxygenAvg*ohmPerC)/100));

					if(debug)
					{
						printf("Temperature = %f, Oxygen = %f\n",temperature,oxygen);
					}

					ostringstream temperatureStream;
					temperatureStream << temperature;
					ostringstream oxygenStream;
					oxygenStream << oxygen;
					ostringstream uidStream;
					uidStream << uid;
					ostringstream powerStream;
					powerStream << power;

					if(power != 0)
					{
						leds.SetValue("1");
						
						char init[100] = "\0";
						//sprintf(init,"{\"uid\":%d,\"nomtemperature\":%f1,\"nomoxygen\":%f1,\"power\":0}",uid,targetTemperature,targetOxygen);
						sprintf(init,"{\"uid\":%d,\"nomtemperature\":%f1,\"power\":%d}",uid,targetTemperature[deviceNr],power);
						//string strToSend = "{\"uid\":" + powerStream.str() + ",\"nomtemperature\":512,\"nomoxygen\":512,\"power\":0}";
						
						//serialPrintf(fd,strToSend.c_str());
						serialPrintf(fd,init);
					}
					else
					{
						leds.SetValue("0");
					}

					string sTimeStamp;
					sTimeStamp = stamp();

					string json = "{\"aggregatorID\":\"" + aggregatorID +
								  "\",\"uid\":" + uidStream.str() +
								  ",\"temperature\":" + temperatureStream.str() +
								  ",\"oxygen\":" + oxygenStream.str() +
								  ",\"power\":" + powerStream.str() +
								  ",\"time\":\"" + sTimeStamp.c_str() +"\"}";
					printf(json.c_str());
					printf("\n");

					mqtt->send_msg(json);
					iReturnCode = mqtt->loop();

					if (iReturnCode)
					{
						mqtt->reconnect();
					}
					
					json = "{\"uid\":" + uidStream.str();
					//Temperature leds above nominal
					if (temperature >= (targetTemperature[deviceNr] * 1.5))
					{
						json += ",\"13\":1";
						json += ",\"12\":0";
						json += ",\"11\":0";
						json += ",\"10\":0";
						json += ",\"9\":0";
					}
					else if (temperature >= (targetTemperature[deviceNr] * 1.3))
					{
						json += ",\"13\":2";
						json += ",\"12\":0";
						json += ",\"11\":0";
						json += ",\"10\":0";
						json += ",\"9\":0";
					}
					else if (temperature >= (targetTemperature[deviceNr] * 1.2))
					{
						json += ",\"12\":1";
						json += ",\"13\":0";
						json += ",\"11\":0";
						json += ",\"10\":0";
						json += ",\"9\":0";
					}
					else if (temperature >= (targetTemperature[deviceNr] * 1.1))
					{
						json += ",\"12\":2";
						json += ",\"13\":0";
						json += ",\"11\":0";
						json += ",\"10\":0";
						json += ",\"9\":0";
					}
					//Temperature leds below   nominal
					else if (temperature <= (targetTemperature[deviceNr] * 0.5))
					{
						json += ",\"9\":1";
						json += ",\"12\":0";
						json += ",\"11\":0";
						json += ",\"10\":0";
						json += ",\"13\":0";
					}
					else if (temperature <= (targetTemperature[deviceNr] * 0.7))
					{
						json += ",\"9\":2";
						json += ",\"12\":0";
						json += ",\"11\":0";
						json += ",\"10\":0";
						json += ",\"13\":0";
					}
					else if (temperature <= (targetTemperature[deviceNr] * 0.8))
					{
						json += ",\"10\":1";
						json += ",\"12\":0";
						json += ",\"11\":0";
						json += ",\"13\":0";
						json += ",\"9\":0";
					}
					else if (temperature <= (targetTemperature[deviceNr] * 0.9))
					{
						json += ",\"10\":2";
						json += ",\"12\":0";
						json += ",\"11\":0";
						json += ",\"13\":0";
						json += ",\"9\":0";
					}
					else //if (temperature < (targetTemperature[deviceNr] * 1.1))
					{
						json += ",\"11\":1";
						json += ",\"12\":0";
						json += ",\"13\":0";
						json += ",\"10\":0";
						json += ",\"9\":0";
					}

					//Oxygen leds above nominal
					if (oxygen >= (targetOxygen[deviceNr] * 1.5))
					{
						json += ",\"8\":1";
						json += ",\"7\":0";
						json += ",\"6\":0";
						json += ",\"5\":0";
						json += ",\"4\":0}";
					}
					else if (oxygen >= (targetOxygen[deviceNr] * 1.3))
					{
						json += ",\"8\":2";
						json += ",\"7\":0";
						json += ",\"6\":0";
						json += ",\"5\":0";
						json += ",\"4\":0}";
					}
					else if (oxygen >= (targetOxygen[deviceNr] * 1.2))
					{
						json += ",\"7\":1";
						json += ",\"8\":0";
						json += ",\"6\":0";
						json += ",\"5\":0";
						json += ",\"4\":0}";
					}
					else if (oxygen >= (targetOxygen[deviceNr] * 1.1))
					{
						json += ",\"7\":2";
						json += ",\"8\":0";
						json += ",\"6\":0";
						json += ",\"5\":0";
						json += ",\"4\":0}";
					}
					//Oxygen leds below nominal
					else if (oxygen <= (targetOxygen[deviceNr] * 0.5))
					{
						json += ",\"4\":1";
						json += ",\"7\":0";
						json += ",\"6\":0";
						json += ",\"5\":0";
						json += ",\"8\":0}";
					}
					else if (oxygen <= (targetOxygen[deviceNr] * 0.7))
					{
						json += ",\"4\":2";
						json += ",\"7\":0";
						json += ",\"6\":0";
						json += ",\"5\":0";
						json += ",\"8\":0}";
					}
					else if (oxygen <= (targetOxygen[deviceNr] * 0.8))
					{
						json += ",\"5\":1";
						json += ",\"7\":0";
						json += ",\"6\":0";
						json += ",\"8\":0";
						json += ",\"4\":0}";
					}
					else if (oxygen <= (targetOxygen[deviceNr] * 0.9))
					{
						json += ",\"5\":2";
						json += ",\"7\":0";
						json += ",\"6\":0";
						json += ",\"8\":0";
						json += ",\"4\":0}";
					}
					else //if (oxygen < (targetOxygen[deviceNr] * 1.1))
					{
						json += ",\"6\":1";
						json += ",\"7\":0";
						json += ",\"6\":0";
						json += ",\"5\":0";
						json += ",\"4\":0}";
					}
					
					
					if(debug)
					{
						printf("Res: %s\n",json.c_str());
					}
					
					serialPrintf(fd,json.c_str());
					
				}
				else if(doc.HasMember("power") && doc.HasMember("uid"))
				{
					int uid = 0, power = 0;
					uid = doc["uid"].GetInt();
					power = doc["power"].GetInt();

					ostringstream uidStream, powerStream;
					uidStream << uid; powerStream << power;

					if(power != 0)
					{
						leds.SetValue("1");
						
						char init[100] = "\0";
						sprintf(init,"{\"uid\":%d,\"nomtemperature\":%f1,\"power\":%d}",uid,targetTemperature[deviceNr],power);
						//sprintf(init,"{\"uid\":%d,\"nomtemperature\":%f1,\"nomoxygen\":%f1,\"power\":0}",uid,targetTemperature,targetOxygen);
						//string strToSend = "{\"uid\":" + powerStream.str() + ",\"nomtemperature\":512,\"nomoxygen\":512,\"power\":0}";
						
						//serialPrintf(fd,strToSend.c_str());
						serialPrintf(fd,init);
					}
					else
					{
						leds.SetValue("0");
					}

					string sTimeStamp;
					sTimeStamp = stamp();

					string json = "{\"aggregatorID\":\"" + aggregatorID +
								  "\",\"uid\":\"" + uidStream.str() +
								  ",\"power\":\"" + powerStream.str() +
								  "\",\"time\":\"" + sTimeStamp.c_str() +"\"}";
					printf(json.c_str());
					printf("\n");

				}
			}
			//usleep(1000);

		}
		mosqpp::lib_cleanup();
	}
    catch (const exception &e)
    {
		//if exception occured in constructor. see class declaration.
		cerr << "Error on Network Connection.\n"
			 << "Check that mosquitto is running & IP/PORT\n";
    }

	close(fd);
	if(debug)
	{
		printf("end loop.\n");
		printf("Stop.\n");
	}
	return;
}

void stopFeature()
{
	GPIO button = GPIO("14");
	button.SetDirection("in");

	while(stopListeningThread == 0)
	{
		string buttonState = "";
		button.GetValue(buttonState);

        if(buttonState == "1")
        {
			if(debug)
			{
				printf("Stop button pressed\n");
			}
			char str[] = "{\"uid\":\"all\",\"power\":1}";

			Document doc;
			ParseResult parseResult = doc.Parse(str);
			if(parseResult)
			{
				for(int i = 0; i < MaxDevice; i++)
				{
					if(runningThreads[i] == true)
					{
						int fd;
						if ((fd = serialOpen(device[i], 115200)) < 0)
						{

						}
						else
						{
							serialPrintf(fd,str);
							close(fd);
						}
					}
				}
			}
		}
	}


	return;
}


void messageCallback (struct mosquitto *mosq, void *obj, const struct mosquitto_message *message) {
    if(debug)
    {
		cout << "got message  " << (char *) message->payload << "  from topic  " << message->topic << endl;
	}
	
    Document doc;
	ParseResult parseResult = doc.Parse((char *) message->payload);
	if(parseResult)
	{
		if(doc.HasMember("aggregatorID") == true)
		{
			string sAll = "all";
			if(doc["aggregatorID"].GetString() == sAll || doc["aggregatorID"].GetString() == aggregatorID)
			{
				if(doc.HasMember("temperature") == false && doc.HasMember("oxygen") == false)
				{
					for(int i = 0; i < MaxDevice; i++)
					{
						if(runningThreads[i] == true)
						{
							int fd;
							if ((fd = serialOpen(device[i], 115200)) < 0)
							{

							}
							else
							{
								if(debug)
								{
									cout << "Sending message  " << (char *) message->payload << "  to  " << device[i] << endl;
								}
								serialPrintf(fd,(char *) message->payload);
								close(fd);
							}
						}
						if(doc.HasMember("uid") == true && doc.HasMember("nomtemperature") == true && doc.HasMember("nomoxygen") == true)
						{
							if(doc["uid"].GetInt() > 0)
							{				
								targetTemperature[doc["uid"].GetInt()-1] = doc["nomtemperature"].GetInt();
								targetOxygen[doc["uid"].GetInt()-1] = doc["nomoxygen"].GetInt();
							}
							else if(doc["uid"].GetInt() == 0)
							{
								for(int i = 0; i < MaxDevice; i++)
								{
									targetTemperature[i] = doc["nomtemperature"].GetInt();
									targetOxygen[i] = doc["nomoxygen"].GetInt();
								}
							}
						}
						else if(doc.HasMember("uid") == true && doc.HasMember("nomtemperature") == true)
						{
							if(doc["uid"].GetInt() > 0)
							{				
								targetTemperature[doc["uid"].GetInt()-1] = doc["nomtemperature"].GetInt();
							}
							else if(doc["uid"].GetInt() == 0)
							{
								for(int i = 0; i < MaxDevice; i++)
								{
									targetTemperature[i] = doc["nomtemperature"].GetInt();
								}
							}
						}
						else if(doc.HasMember("uid") == true && doc.HasMember("nomoxygen") == true)
						{
							if(doc["uid"].GetInt() > 0)
							{				
								targetOxygen[doc["uid"].GetInt()-1] = doc["nomoxygen"].GetInt();
							}
							else if(doc["uid"].GetInt() == 0)
							{
								for(int i = 0; i < MaxDevice; i++)
								{
									targetOxygen[i] = doc["nomoxygen"].GetInt();
								}
							}
						}
					}
				}
			}
		}
		
	}
	else
	{
		cout<<"Parse Error";
	}
}

void mqttReceiver()
{
	try
    {
		//uint8_t reconnect       = true;
		string clientID         = "mosquitto_client_" + to_string (getpid());
		struct mosquitto *mosq  = nullptr;
		int resCode             = 0;

		mosquitto_lib_init ();

		mosq    = mosquitto_new (clientID.c_str(), true, 0);

		
		if(mosq)
		{
			mosquitto_message_callback_set (mosq, messageCallback);

			cout << "Connection result: " << mosquitto_strerror (mosquitto_connect (mosq, MQTT_HOST, MQTT_PORT, 60)) << endl;
			cout << "Subscription result: " << mosquitto_strerror (mosquitto_subscribe (mosq, NULL, TARGET_TOPIC, 0)) << endl;
				
			while (stopListeningThread == 0) 
			{
				resCode = mosquitto_loop (mosq, 20, 1);
				if (resCode) 
				{
					cout << "ERROR:  " << mosquitto_strerror (resCode) << "  (" << resCode << ")\n";
					sleep(1);
					mosquitto_reconnect (mosq);
				}
			}
			
			mosquitto_destroy (mosq);
		}
		else
		{
			cout<<"Error in mqttReciever";
			sleep(10000);
		}
		
		mosquitto_lib_cleanup ();
	}
	catch (const exception &e)
    {
		//if exception occured in constructor. see class declaration.
		cerr << "Error on Network Connection.\n"
			 << "Check that mosquitto is running & IP/PORT\n";
    }
}

int main(int argc, char *argv[])
{
	signal (SIGINT,ctrlC_Handler);
	mosqpp::lib_init();
	aggregatorID = "";

	for(int i = 1; i < argc; i++)
	{
		if(strcmp("--aggregatorID",argv[i]) == 0)
		{
			aggregatorID = string(argv[1+i]);
		}
		else if(strcmp("--debug",argv[i]) == 0)
		{
			debug = true;
		}
		else if (strcmp("--help",argv[i]) == 0 || strcmp("-h",argv[i]) == 0)
		{
			printf("--aggregatorID <string>\t to set collectors id\n");
			printf("--debug \t\t for debugging\n");
			return 0;
		}
	}
	
	while(aggregatorID == "")
	{
		cout<< "Enter the aggregator id:\n";
		getline(cin,aggregatorID);
	}

	//char device[MaxDevice][15];
	//bool runningThreads[MaxDevice];
	char gpio[MaxDevice][3] = { "21", "20", "16", "12", "7\0" , "8\0", "25", "24", "23", "18", "15", "26", "29", "13", "6\0", "5\0", "11", "9\0", "10", "22", "27", "17", "4\0", "3\0" }; //...
	thread listeningThread[MaxDevice];
	

	//Setup
	for(int i = 0; i < MaxDevice;i++)
	{
		/*gpio[i][0] = '2';
		gpio[i][1] = '1'; */
		gpio[i][2] = '\0';

		for(int j = 0; j < 15;j++)
		{
			device[i][j] = '\0';
		}
		sprintf(device[i], "/dev/ttyACM%d",i);
		
		targetTemperature[i] = 20;
		targetOxygen[i] = 20;

		runningThreads[i] = false;
	}
	//Initial thread launch
	for(int i = 0; i < MaxDevice;i++)
	{
		int fd;
		if ((fd = serialOpen(device[i],115200)) < 0)
		{
			if(debug)
			{
				fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
			}
		}
		else
		{
			printf("Starting thread: \n\tgpio: %s,\n\tdevice: %s\n",gpio[i],device[i]);
			thread startThread (main_Program,gpio[i],device[i],i);
			listeningThread[i] = move(startThread);
			runningThreads[i] = true;
			close(fd);
		}
	}

	//thread stopFeatureThread (stopFeature,device,runningThreads);
	//thread mqttReceiverThread (mqttReceiver,device,runningThreads);
	thread stopFeatureThread (stopFeature);
	thread mqttReceiverThread (mqttReceiver);


	string stopSignal = "";
	while(stopSignal != "stop" && stopSignal != "quit" && stopSignal != "exit")
	{
		stopSignal = "";
		cout<<"Enter a command: \n";
		cin >> stopSignal;

		if(stopSignal == "update")
		{
			for(int i = 0; i < MaxDevice;i++)
			{
				if(runningThreads[i] == false)
				{
					int fd;
					if ((fd = serialOpen(device[i],115200)) < 0)
					{
						//fprintf(stderr, "Unable to open serial device: %s\n", strerror(errno));
					}
					else
					{
						printf("Starting thread: \n\tgpio: %s,\n\tdevice: %s\n",gpio[i],device[i]);
						thread startThread (main_Program,gpio[i],device[i],i);
						listeningThread[i] = move(startThread);
						runningThreads[i] = true;
						close(fd);
					}
				}
			}
		}
		else if(stopSignal == "help")
		{
			printf("stop \t- Quits the program\nupdate\t- Adds new collectors");
		}
		else if(debug)
		{
			char str[100];
			strcpy(str,stopSignal.c_str());

			Document doc;
			ParseResult parseResult = doc.Parse(str);
			if(parseResult)
			{
				for(int i = 0; i < MaxDevice; i++)
				{
					if(runningThreads[i] == true)
					{
						int fd;
						if ((fd = serialOpen(device[i], 115200)) < 0)
						{

						}
						else
						{
							serialPrintf(fd,str);
							close(fd);
						}
					}
				}
			}
		}
	}
	stopListeningThread = 1;
	cout<<"Stopping...\n";

	//Waits for all threads to join
	stopFeatureThread.join();
	mqttReceiverThread.join();
	for(int i = 0; i < MaxDevice; i++)
	{
		if(runningThreads[i] == true)
		{
			listeningThread[i].join();
		}
	}
	return 0;
}
