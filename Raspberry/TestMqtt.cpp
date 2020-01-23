#include <iostream>
#include <cstring>
#include <stdexcept>

#include "TestMqtt.h"
#include <mosquittopp.h>

using namespace std;

TestMqtt::TestMqtt(const string sId, const string sTopic, const string sHost, int iPort) : mosquittopp(sId.c_str())
{
	debug = false;
	miKeepAliveCount = 60;
	msId = sId;
	miPort = iPort;
	msHost = sHost;
	msTopic = sTopic;

	if (connect(msHost.c_str(), miPort, miKeepAliveCount) == MOSQ_ERR_ERRNO)
	{
		throw runtime_error("##ERROR##");
	}

	loop_start();
};

TestMqtt::~TestMqtt()
{
	//kills the thread
	loop_stop();
}

void TestMqtt::on_connect(int iReturnCode)
{
	if (iReturnCode == 0 && debug)
	{
		cout << " ## - Connected with Broker - ## " << std::endl;
	}
	else if(iReturnCode != 0)
	{
		cout << "## - Unable to Connect Broker - ## " << std::endl;
	}
}

bool TestMqtt::send_msg(const string sMessage)
{
	int ret = publish(nullptr, msTopic.c_str(), sMessage.length(), sMessage.c_str(), 2, false);

	return (ret == MOSQ_ERR_SUCCESS);
}

void TestMqtt::on_disconnect()
{
	if(debug)
		cout << " ## - Disconnected from Broker - ## " << std::endl;
}

void TestMqtt::on_publish(int mid)
{
	if(debug)
		cout << "## - Message published successfully - ##" << endl;
}

void TestMqtt::on_subscribe(int mid, int qos_count, const int *granted_qos)
{
    //if(debug)
		//cout << "## - Subscribed successfully - ##" << endl;
}

bool TestMqtt::receive_message(const char * message)
{
	printf("receive_message: %s;;;",message);
	return 0;
}

void TestMqtt::on_message(const struct mosquitto_message *message)
{
	if(debug)
		cout<<"\n\n ## - Message - ##\n\n";
		
	cout << "Received message of topic: " << message->topic << " Data: " << reinterpret_cast<char*>(message->payload) << "\n";
		
	//strcpy(c_message,message->payload);
	return;	
}
