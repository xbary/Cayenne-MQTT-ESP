/*
The MIT License(MIT)

Cayenne Arduino Client Library
Copyright (c) 2016 myDevices

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
documentation files(the "Software"), to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense, and / or sell copies of the Software,
and to permit persons to whom the Software is furnished to do so, subject to the following conditions :
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef _CAYENNEARDUINOMQTTCLIENT_h
#define _CAYENNEARDUINOMQTTCLIENT_h

#include "CayenneArduinoDefines.h"
#include "CayenneMQTTClient/CayenneMQTTClient.h"

const int MAX_CHANNEL_ARRAY_SIZE = 4;

void CayenneMessageArrived(CayenneMessageData* message);

class CayenneArduinoMQTTClient
{
public:

#ifdef DIGITAL_AND_ANALOG_SUPPORT
	static uint32_t digitalChannels[MAX_CHANNEL_ARRAY_SIZE];
	static uint32_t analogChannels[MAX_CHANNEL_ARRAY_SIZE];
#endif

	/**
	* Initializes Cayenne
	* @param client The networking client
	* @param username Cayenne username
	* @param password Cayenne password
	* @param clientID Cayennne client ID
	* @param chunkSize Size of chunks to use when writing the send buffer to the client, 0 to just send the full buffer.
	*/
	void begin(Client& client, const char* username, const char* password, const char* clientID, int chunkSize = 0);

	/**
	* Connects to Cayenne
	*/
	void connect();

	/**
	* Main Cayenne loop
	*
	* @param yieldTime  Time in milliseconds to yield to allow processing of incoming MQTT messages and keep alive packets.
	* NOTE: Decreasing the yieldTime while calling write functions (e.g. virtualWrite) in your main loop could cause a 
	* large number of messages to be sent to the Cayenne server. Use caution when adjusting this because sending too many 
	* messages could cause your IP to be rate limited or even blocked. If you would like to reduce the yieldTime to cause your 
	* main loop to run faster, make sure you use a timer for your write functions to prevent them from running too often. 
	*/
	void loop(int yieldTime = 1000);

	/**
	* Send device info
	*/
	void publishDeviceInfo();

	/**
	* Sends a measurement to a Cayenne channel
	*
	* @param channel  Cayenne channel number
	* @param data  Data to be sent
	* @param type  Measurement type
	* @param unit  Measurement unit
	*/
	template <typename T>
	void virtualWrite(unsigned int channel, const T& data, const char* type = NULL, const char* unit = NULL);

	/**
	* Sends an array of measurements to a Cayenne channel
	*
	* @param channel  Cayenne channel number
	* @param values  Array of values to be sent
	* @param type  Measurement type
	*/
	void virtualWrite(unsigned int channel, const CayenneDataArray& values, const char* type);

#ifdef CAYENNE_USING_PROGMEM
	/**
	* Sends a measurement to a Cayenne channel
	*
	* @param channel  Cayenne channel number
	* @param data  Data to be sent
	* @param type  Measurement type
	* @param unit  Measurement unit
	*/
	template <typename T>
	void virtualWrite(unsigned int channel, const T& data, const __FlashStringHelper* type, const __FlashStringHelper* unit = NULL);

	/**
	* Sends an array of measurements to a Cayenne channel
	*
	* @param channel  Cayenne channel number
	* @param values  Array of values to be sent
	* @param type  Measurement type
	*/
	void virtualWrite(unsigned int channel, const CayenneDataArray& values, const __FlashStringHelper* type);
#endif

	/**
	* Sends a response after processing a command
	*
	* @param channel  Cayenne channel number
	* @param error  Error message, NULL for success
	* @param id  Message id
	*/
	static void responseWrite(const char* error, const char* id);
	
	/**
	* Publish channel state.
	*
	* @param topic Cayenne topic
	* @param channel Channel number
	* @param value State of channel
	*/
	template <typename T>
	static void publishState(CayenneTopic topic, unsigned int channel, const T& value);

	/**
	* Sends a Celsius value to a Cayenne channel
	*
	* @param channel  Cayenne channel number
	* @param value  Value to be sent
	*/
	void celsiusWrite(unsigned int channel, float value);

	/**
	* Sends a Fahrenheit value to a Cayenne channel
	*
	* @param channel  Cayenne channel number
	* @param value  Value to be sent
	*/
	void fahrenheitWrite(unsigned int channel, float value);

	/**
	* Sends a Kelvin value to a Cayenne channel
	*
	* @param channel  Cayenne channel number
	* @param value  Value to be sent
	*/
	void kelvinWrite(unsigned int channel, float value);

	/**
	* Sends a Lux value to a Cayenne channel
	*
	* @param channel  Cayenne channel number
	* @param value  Value to be sent
	*/
	void luxWrite(unsigned int channel, float value);

	/**
	* Sends a Pascal value to a Cayenne channel
	*
	* @param channel  Cayenne channel number
	* @param value  Value to be sent
	*/
	void pascalWrite(unsigned int channel, float value);

	/**
	* Sends a Hectopascal value to a Cayenne channel
	*
	* @param channel  Cayenne channel number
	* @param value  Value to be sent
	*/
	void hectoPascalWrite(unsigned int channel, float value);

	/**
	* Sends a digital sensor value to a Cayenne channel
	*
	* @param channel  Cayenne channel number
	* @param value  Value to be sent
	*/
	void digitalSensorWrite(unsigned int channel, int value);

	/**
	* Requests Server to re-send current values for all widgets.
	*/
	void syncAll();

	/**
	* Requests App or Server to re-send current value of a Cayenne channel.
	* This will cause the user-defined CAYENNE_IN handler to be called.
	* @param channel  Cayenne channel number
	*/
	void syncVirtual(int channel);

	/**
	* Enables/disables polling for a channel.
	* @param topic Cayenne topic
	* @param channelArray  Channel array to modify
	* @param channel  Channel number
	* @param enable  Enable or disable polling of channel
	*/
	static void enableChannel(uint32_t channelArray[], uint8_t channel, bool enable);

private:

	/**
	* Publish data using specified topic suffix
	* @param topic Cayenne topic
	* @param channel Cayenne channel number
	* @param data Data to send
	* @param key Optional key to use for a key=data pair
	* @param subkey Optional subkey to use for a key,subkey=data pair
	*/
	template <typename T>
	static void publishData(CayenneTopic topic, unsigned int channel, const T& data, const char* key = NULL, const char* subkey = NULL);

	/**
	* Publish value array using specified topic suffix
	* @param topic Cayenne topic
	* @param channel Cayenne channel number
	* @param values  Array of values to be sent
	* @param valueCount  Count of values in array
	* @param key Optional key to use for a key=data pair
	*/
	static void publishData(CayenneTopic topic, unsigned int channel, const CayenneValuePair values[], size_t valueCount, const char* key);

#ifdef CAYENNE_USING_PROGMEM
	/**
	* Publish data using specified topic suffix
	* @param topic Cayenne topic
	* @param channel Cayenne channel number
	* @param data Data to send
	* @param key Optional key to use for a key=data pair
	* @param subkey Optional subkey to use for a key,subkey=data pair
	*/
	template <typename T>
	static void publishData(CayenneTopic topic, unsigned int channel, const T& data, const __FlashStringHelper* key, const __FlashStringHelper* subkey = NULL); 
	/**
	* Publish value array using specified topic suffix
	* @param topic Cayenne topic
	* @param channel Cayenne channel number
	* @param values  Array of values to be sent
	* @param valueCount  Count of values in array
	* @param key Optional key to use for a key=data pair
	*/
	static void publishData(CayenneTopic topic, unsigned int channel, const CayenneValuePair values[], size_t valueCount, const __FlashStringHelper* key);
#endif

	/**
	* Call enabled virtual channel handlers to send channel data.
	*/
	void pollVirtualChannels();

#ifdef DIGITAL_AND_ANALOG_SUPPORT
	/**
	* Polls enabled digital channels and sends the matching pin's current value.
	*/
	void pollChannels(uint32_t channelArray[]);
#endif

	static CayenneMQTTClient _mqttClient;
	Network _network;
};

//extern CayenneMQTTClient CayenneArduinoMQTTClient::_mqttClient;

#endif
