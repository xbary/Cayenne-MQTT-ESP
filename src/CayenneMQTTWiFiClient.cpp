#include "CayenneMQTTWiFiClient.h"
#include <WiFi.h>

#ifndef NO_GLOBAL_CAYENNE
CayenneMQTTWiFiClient Cayenne;
#endif

void CayenneMQTTWiFiClient::begin(const char* username, const char* password, const char* clientID, const char* ssid, const char* wifiPassword)
{
	int status = WL_IDLE_STATUS;
	WiFi.mode(WIFI_STA);
	delay(500);
	if (WiFi.status() == WL_NO_SHIELD) {
		CAYENNE_LOG("WiFi not present");
		while (true);
	}

	CAYENNE_LOG("Connecting to %s", ssid);
	if (wifiPassword && strlen(wifiPassword)) {
		WiFi.begin(ssid, wifiPassword);
	}
	else {
		WiFi.begin(ssid);
	}
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
	}
	CAYENNE_LOG("Connected to WiFi");

	IPAddress local_ip = WiFi.localIP();
	CAYENNE_LOG("IP: %d.%d.%d.%d", local_ip[0], local_ip[1], local_ip[2], local_ip[3]);
	CayenneArduinoMQTTClient::begin(_wifiClient, username, password, clientID, WRITE_CHUNK_SIZE);
}
/**
* Begins Cayenne session, assumes that the WIFI is already up and running.
* @param username Cayenne username
* @param password Cayenne password
* @param clientID Cayennne client ID
*/
void CayenneMQTTWiFiClient::begin(const char* username, const char* password, const char* clientID)
{
	if (WiFi.status() != WL_CONNECTED) {
		CAYENNE_LOG("CayenneMQTTWiFiClient.begin called without WIFI being connected. Either use begin (..., ssid, wifipassword) to establish the connection here. Or setup the WIFI connection manually before calling this variant of the begin function");
	}
	IPAddress local_ip = WiFi.localIP();
	CAYENNE_LOG("IP: %d.%d.%d.%d", local_ip[0], local_ip[1], local_ip[2], local_ip[3]);
	CayenneArduinoMQTTClient::begin(_wifiClient, username, password, clientID, WRITE_CHUNK_SIZE);
}
