/*
	socket.io-esp-client: a Socket.IO client for the Esp8266

	Based off the work of  Bill Roy https://github.com/billroy and
	washo4evr https://github.com/washo4evr 

	MIT License
	Copyright (c) [2016] [Dolapo Toki]
*/
#include "Arduino.h"
// #include <ESP8266WiFi.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include "SPI.h"


// Length of static data buffers
#define DATA_BUFFER_LEN 120
#define SID_LEN 24

//Using the https protocol
class SocketIOClientSecure {
	public:
		typedef void (*DataArrivedDelegate)(SocketIOClientSecure client, char *data);
		//Connect to the socket server over https
		bool connect(IPAddress ip, uint16_t port = 443) ;
		bool connect(const char* name, uint16_t port = 443) ;

        bool connected();
		bool  monitor();

        void disconnect();
		void parser(int index);
		void heartbeat(int select);
		
		//Callback method
		void setDataArrivedDelegate(DataArrivedDelegate dataArrivedDelegate);
		
		//Sending data to the server
		void send(String RID, String Rname, String Rcontent);
		void sendJSON(String RID, String JSON);

	private:
		
		WiFiClientSecure client;
		DataArrivedDelegate dataArrivedDelegate;
		IPAddress serverIp;
		char *dataptr;
		char databuffer[DATA_BUFFER_LEN];
		char sid[SID_LEN];
		char key[28];
		const char* hostname;

		void sendHandshake(IPAddress ip);
        void sendHandshake(const char* name);
      
        bool readHandshake();
		bool waitForInput(void);
		uint16_t port;

		void readLine();
		void findColon(char which);
		void terminateCommand(void);
		void eatHeader(void);
};
