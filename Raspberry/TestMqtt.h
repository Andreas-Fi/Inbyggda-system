#ifndef TESTMQTT_H
#define TESTMQTT_H

#include <string>
using namespace std;

#include <mosquittopp.h>

class TestMqtt : public mosqpp::mosquittopp
{
  public:

   TestMqtt(const string sId, const string sTopic, const string sHost, int iPort);
   ~TestMqtt();
   bool debug;
   bool send_msg(const string sMessage);
   char c_message[151];
	bool receive_message(const char * message);
private:
	string 	msHost;
	string 	msId;
	string 	msTopic;
	int	miPort;
	int	miKeepAliveCount;

	void on_connect(int iReturnCode);
	void on_disconnect();
	void on_publish(int iMessageId);
	void on_subscribe(int mid, int qos_count, const int *granted_qos);
	void on_message(const struct mosquitto_message *message);


};

#endif
