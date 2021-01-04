/*
	socket.io-esp-client: a Socket.IO client for the Esp8266

	Based off the work of  Bill Roy https://github.com/billroy and
	washo4evr https://github.com/washo4evr 
	
	MIT License
	Copyright (c) [2016] [Dolapo Toki]
	
*/
#include <SocketIOClientSecure.h>


String tmpdata = "";	//External variables
String RID = "";
String Rname = "";
String Rcontent = "";

bool SocketIOClientSecure::connect(IPAddress ip, uint16_t theport) {
	if (!client.connect(ip, theport))return false;
	serverIp = ip;
	hostname = ip.toString().c_str();
	Serial.print("Host name: ");
	Serial.println(hostname);
	port = theport;
	sendHandshake(ip);
	return readHandshake();
}

bool SocketIOClientSecure::connect(const char* thehostname, uint16_t theport) {
	if (!client.connect(thehostname, theport)) return false;
	hostname = thehostname;
	port = theport;
	sendHandshake(hostname);
	return readHandshake();
}

bool SocketIOClientSecure::connected() {

	return client.connected();
}

void SocketIOClientSecure::disconnect() {
	client.stop();
}

// find the nth colon starting from dataptr
void SocketIOClientSecure::findColon(char which) {	
	while (*dataptr) {
		if (*dataptr == ':') {
			if (--which <= 0) return;
		}
		++dataptr;
	}
}

// terminate command at dataptr at closing double quote
void SocketIOClientSecure::terminateCommand(void) {
	dataptr[strlen(dataptr)-3] = 0;
}

bool SocketIOClientSecure::monitor() {
	
	int index = -1;
	int index2 = -1;
	String tmp = "";
	*databuffer = 0;

	if (!client.connected()) {
		if (!client.connect(serverIp, port)) return 0;
	}

	if (!client.available())
	{
		return 0;
	}
	char which;
	while (client.available()) {
		readLine();
		tmp = databuffer;
		dataptr = databuffer;
		index = tmp.indexOf((char)129);	//129 DEC = 0x81 HEX = sent for proper communication
		index2 = tmp.indexOf((char)129, index + 1);
		/*Serial.print("Index = ");			//Can be used for debugging
		Serial.print(index);
		Serial.print(" & Index2 = ");
		Serial.println(index2);*/
		if (index != -1)
		{
			parser(index);
		}
		if (index2 != -1)
		{
			parser(index2);
		}
	}
	return 1;
}

void SocketIOClientSecure::parser(int index) {
	String rcvdmsg = "";
	int sizemsg = databuffer[index + 1];   // 0-125 byte, index ok        Fix provide by Galilei11. Thanks
	if (databuffer[index + 1]>125)
	{
		sizemsg = databuffer[index + 2];    // 126-255 byte
		index += 1;       // index correction to start
	}
	//Serial.print("Message size = ");	//Can be used for debugging
	//Serial.println(sizemsg);			//Can be used for debugging
	for (int i = index + 2; i < index + sizemsg + 2; i++)
		rcvdmsg += (char)databuffer[i];
	//Serial.print("Received message = ");	//Can be used for debugging
	//Serial.println(rcvdmsg);				//Can be used for debugging
	switch (rcvdmsg[0])
	{
	case '2':
		Serial.println("Ping received - Sending Pong");
		heartbeat(1);
		break;

	case '3':
		Serial.println("Pong received - All good");
		break;

	case '4':
		switch (rcvdmsg[1])
		{
		case '0':
			Serial.println("Upgrade to WebSocket confirmed");
			break;
		case '2':
			RID = rcvdmsg.substring(4, rcvdmsg.indexOf("\","));
			Rname = rcvdmsg.substring(rcvdmsg.indexOf("\",") + 4, rcvdmsg.indexOf("\":"));
			Rcontent = rcvdmsg.substring(rcvdmsg.indexOf("\":") + 3, rcvdmsg.indexOf("\"}"));
			//Serial.println("RID = " + RID);
			//Serial.println("Rname = " + Rname);
			//Serial.println("Rcontent = " + Rcontent);
			//Serial.println(rcvdmsg);
			break;
		}
	}
}

void SocketIOClientSecure::setDataArrivedDelegate(DataArrivedDelegate newdataArrivedDelegate) {
	  dataArrivedDelegate = newdataArrivedDelegate;
}

//Send the apopraite headerfiles to try establish a commumication using ip
void SocketIOClientSecure::sendHandshake(IPAddress ip) {
	client.println("GET /socket.io/?EIO=3&transport=polling HTTP/1.1");
	client.println(F("Host: 104.198.2.177:8081"));
	client.println(F("Origin: Arduino\r\n"));
}
//Send the apopraite headerfiles to try establish a commumication using dns
void SocketIOClientSecure::sendHandshake(const char* hostname) {
	client.println(F("GET /socket.io/?EIO=3&transport=polling HTTP/1.1"));
	client.print(F("Host: 104.198.2.177:8081"));
	client.println(F("User-Agent: Arduino/1.0\r\n"));
}

bool SocketIOClientSecure::waitForInput(void) {
	unsigned long now = millis();
	while (!client.available() && ((millis() - now) < 30000UL)) { ; }
	return client.available();
}

void SocketIOClientSecure::eatHeader(void) {
	while (client.available()) {	// consume the header
		readLine();
		if (strlen(databuffer) == 0) break;
	}
}

bool SocketIOClientSecure::readHandshake() {

	if (!waitForInput()) return false;

	// check for happy "HTTP/1.1 200" response
	readLine();
	if (atoi(&databuffer[9]) != 200) {
		while (client.available()) readLine();
		client.stop();
		return false;
	}
	eatHeader();
	readLine();	// read first line of response



	//Get the sid form the hand shake
	char *iptr = databuffer;
	char *optr = sid;
	while (*iptr && (*iptr != '{'))
		iptr++;
	iptr += 8;
	while (*iptr && (*iptr != '"') && (optr < &sid[SID_LEN - 2]))
		*optr++ = *iptr++;
	*optr = 0;

	//Output the sid
	Serial.print(F("Connected. SID="));
	Serial.println(sid);	// sid:transport:timeout 

	while (client.available()) readLine();
	client.stop();
	delay(1000);

	// reconnect on websocket connection
	Serial.print(F("WS Connect..."));
	// if there is a hostname else use the ip address
	
	if (!client.connect(serverIp, port)) {
		Serial.print(F("Reconnect failed."));
		return false;
	}
	Serial.println(F("Reconnected."));
	//Send header to the server
	client.print(F("GET /socket.io/1/?EIO=3&transport=websocket&sid="));
	client.print(sid);
	client.println(F(" HTTP/1.1"));
	client.println(F("Connection: Upgrade"));
	client.println(F("Upgrade: websocket"));
	client.print(F("Host: "));
	client.print(hostname);
	client.print(F(":"));
	client.println(port);
	client.println(F("Sec-WebSocket-Version: 13"));
	client.println(F("Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ=="));
	client.println();
	//Wait for response from the server
	if (!waitForInput()) return false;
	readLine();
	//Check if the responce gotten back from the server is okay
	if (atoi(&databuffer[9]) != 101) {
		while (client.available()) readLine();
		client.stop();
		Serial.println("Abort 2");
		return false;
	}
	//Read the next 3 lines
	readLine();
	readLine();
	readLine();
	//Get the Sec-WebSocket-Accept key from header
	for (int i = 0; i < 28; i++)
	{
		key[i] = databuffer[i + 22];	//key contains the Sec-WebSocket-Accept, could be used for verification
	}
	eatHeader();
	//Need to decode data mask data being dent to the server https://developer.mozilla.org/en-US/docs/Web/API/WebSockets_API/Writing_WebSocket_servers
	randomSeed(analogRead(0));
	String mask = "";
	String masked = "52";
	String message = "52";
	for (int i = 0; i < 4; i++)	//generate a random mask, 4 bytes, ASCII 0 to 9
	{
		char a = random(48, 57);
		mask += a;
	}

	for (int i = 0; i < message.length(); i++)
		masked[i] = message[i] ^ mask[i % 4];	//apply the "mask" to the message ("52")

	client.print((char)0x81);	//has to be sent for proper communication
	client.print((char)130);	//size of the message (2) + 128 because message has to be masked
	client.print(mask);
	client.print(masked);

	monitor();		// treat the response as input
	return true;
}
//Read data line 
void SocketIOClientSecure::readLine() {
	dataptr = databuffer;
	while (client.available() && (dataptr < &databuffer[DATA_BUFFER_LEN-2])) {
		char c = client.read();
		//Serial.print(c);
		if (c == 0) Serial.print(F("NULL"));
		else if (c == 255) Serial.print(F("0x255"));
		else if (c == '\r') {;}
		else if (c == '\n') break;
		else *dataptr++ = c;
	}
	*dataptr = 0;
}

//Send an individual message to the server
void SocketIOClientSecure::send(String RID, String Rname, String Rcontent) {

	String message = "42[\"" + RID + "\",{\"" + Rname + "\":\"" + Rcontent + "\"}]";
	int header[10];
	header[0] = 0x81;
	int msglength = message.length();
	randomSeed(analogRead(0));
	String mask = "";
	String masked = message;
	for (int i = 0; i < 4; i++)
	{
		char a = random(48, 57);
		mask += a;
	}
	for (int i = 0; i < msglength; i++)
		masked[i] = message[i] ^ mask[i % 4];

	client.print((char)header[0]);	//has to be sent for proper communication
									//Depending on the size of the message
	if (msglength <= 125)
	{
		header[1] = msglength + 128;
		client.print((char)header[1]);	//size of the message + 128 because message has to be masked
	}
	else if (msglength >= 126 && msglength <= 65535)
	{
		header[1] = 126 + 128;
		client.print((char)header[1]);
		header[2] = (msglength >> 8) & 255;
		client.print((char)header[2]);
		header[3] = (msglength) & 255;
		client.print((char)header[3]);
	}
	else
	{
		header[1] = 127 + 128;
		client.print((char)header[1]);
		header[2] = (msglength >> 56) & 255;
		client.print((char)header[2]);
		header[3] = (msglength >> 48) & 255;
		client.print((char)header[4]);
		header[4] = (msglength >> 40) & 255;
		client.print((char)header[4]);
		header[5] = (msglength >> 32) & 255;
		client.print((char)header[5]);
		header[6] = (msglength >> 24) & 255;
		client.print((char)header[6]);
		header[7] = (msglength >> 16) & 255;
		client.print((char)header[7]);
		header[8] = (msglength >> 8) & 255;
		client.print((char)header[8]);
		header[9] = (msglength) & 255;
		client.print((char)header[9]);
	}

	client.print(mask);
	client.print(masked);
}
//Send json data to the server
void SocketIOClientSecure::sendJSON(String RID, String JSON) {
	String message = "42[\"" + RID + "\"," + JSON + "]";
	int header[10];
	header[0] = 0x81;
	int msglength = message.length();
	randomSeed(analogRead(0));
	String mask = "";
	String masked = message;
	for (int i = 0; i < 4; i++)
	{
		char a = random(48, 57);
		mask += a;
	}
	for (int i = 0; i < msglength; i++)
		masked[i] = message[i] ^ mask[i % 4];

	client.print((char)header[0]);	//has to be sent for proper communication
									//Depending on the size of the message
	if (msglength <= 125)
	{
		header[1] = msglength + 128;
		client.print((char)header[1]);	//size of the message + 128 because message has to be masked
	}
	else if (msglength >= 126 && msglength <= 65535)
	{
		header[1] = 126 + 128;
		client.print((char)header[1]);
		header[2] = (msglength >> 8) & 255;
		client.print((char)header[2]);
		header[3] = (msglength) & 255;
		client.print((char)header[3]);
	}
	else
	{
		header[1] = 127 + 128;
		client.print((char)header[1]);
		header[2] = (msglength >> 56) & 255;
		client.print((char)header[2]);
		header[3] = (msglength >> 48) & 255;
		client.print((char)header[4]);
		header[4] = (msglength >> 40) & 255;
		client.print((char)header[4]);
		header[5] = (msglength >> 32) & 255;
		client.print((char)header[5]);
		header[6] = (msglength >> 24) & 255;
		client.print((char)header[6]);
		header[7] = (msglength >> 16) & 255;
		client.print((char)header[7]);
		header[8] = (msglength >> 8) & 255;
		client.print((char)header[8]);
		header[9] = (msglength) & 255;
		client.print((char)header[9]);
	}

	client.print(mask);
	client.print(masked);
}


//Generate random mask
void SocketIOClientSecure::heartbeat(int select) {
	randomSeed(analogRead(0));
	String mask = "";
	String masked = "";
	String message = "";
	if (select == 0)
	{
		masked = "2";
		message = "2";
	}
	else
	{
		masked = "3";
		message = "3";
	}
	for (int i = 0; i < 4; i++)	//generate a random mask, 4 bytes, ASCII 0 to 9
	{
		char a = random(48, 57);
		mask += a;
	}

	for (int i = 0; i < message.length(); i++)
		masked[i] = message[i] ^ mask[i % 4];	//apply the "mask" to the message ("2" : ping or "3" : pong)



	client.print((char)0x81);	//has to be sent for proper communication
	client.print((char)129);	//size of the message (1) + 128 because message has to be masked
	client.print(mask);
	client.print(masked);
}
