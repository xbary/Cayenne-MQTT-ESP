#include "CayenneArduinoMQTTClient.h"
#include "CayenneArduinoDefines.h"
#include "CayenneMQTTClient/CayenneMQTTClient.h"

CayenneMQTTClient CayenneArduinoMQTTClient::_mqttClient;

#ifdef DIGITAL_AND_ANALOG_SUPPORT
uint32_t CayenneArduinoMQTTClient::digitalChannels[MAX_CHANNEL_ARRAY_SIZE] = { 0 };
uint32_t CayenneArduinoMQTTClient::analogChannels[MAX_CHANNEL_ARRAY_SIZE] = { 0 };

void configChannel(uint32_t channelArray[], uint8_t channel, const char* bytes) {
	CAYENNE_LOG_DEBUG("configChannel: %d %s", channel, bytes);
	int enable = -1;
	if(strlen(bytes) >= 2) {
		if (strcmp(bytes, "on") == 0) {
			enable = true;
		}
		else if (strcmp(bytes, "off") == 0) {
			enable = false;
		}
		if (enable != -1) {
			CayenneArduinoMQTTClient::enableChannel(channelArray, channel, enable);
		}
	}
}
#endif

void handleMessage(CayenneMessageData* messageData) 
{
	Request request = { messageData->channel };
	const char* response = NULL;
	CayenneMessage message(messageData);
	if (strlen(messageData->values[0].value)) {
		CAYENNE_LOG_DEBUG("In: value %s, channel %d", messageData->values[0].value, request.channel);
		InputHandlerFunction handler = GetInputHandler(request.channel);
		if (handler && handler != InputHandler) {
			handler(request, message);
		} else {
			InputHandlerDefault(request, message);
		}
		response = message.getError();
	}
	else {
		response = ERROR_INCORRECT_PARAM;
	}
	if(response == NULL) {
		// If there was no error, we send the new channel state, which should be the command value we received.
		CayenneArduinoMQTTClient::publishState(DATA_TOPIC, messageData->channel, messageData->values[0].value);
	}
	CayenneArduinoMQTTClient::responseWrite(response, messageData->id);
}

#ifdef DIGITAL_AND_ANALOG_SUPPORT
void handleAnalogMessage(CayenneMessageData* messageData) {
	float value = atof(messageData->values[0].value);
	char* response = NULL;
	if (value >= 0 && value <= 1) {
		double test = value * 255;
		CAYENNE_LOG_DEBUG("aw %f, channel %d", value, messageData->channel);
		analogWrite(messageData->channel, (int)(value * 255));
		CayenneArduinoMQTTClient::publishState(ANALOG_TOPIC, messageData->channel, value);
	}
	else {
		response = ERROR_INCORRECT_PARAM;
	}
	CayenneArduinoMQTTClient::responseWrite(messageData->channel, response, messageData->id);
}

void handleDigitalMessage(CayenneMessageData* messageData) {
	char* response = NULL;
	if (messageData->values[0].value && strlen(messageData->values[0].value) == 1) {
		CAYENNE_LOG_DEBUG("dw %s, channel %d", messageData->values[0].value, messageData->channel);
		if (messageData->values[0].value[0] == '0') {
			digitalWrite(messageData->channel, LOW);
			CayenneArduinoMQTTClient::publishState(DIGITAL_TOPIC, messageData->channel, LOW);
		}
		else if (messageData->values[0].value[0] == '1') {
			digitalWrite(messageData->channel, HIGH);
			CayenneArduinoMQTTClient::publishState(DIGITAL_TOPIC, messageData->channel, HIGH);
		}
		else {
			response = ERROR_INCORRECT_PARAM;
		}
	}
	else {
		response = ERROR_INCORRECT_PARAM;
	}
	CayenneArduinoMQTTClient::responseWrite(messageData->channel, response, messageData->id);
}
#endif

void CayenneMessageArrived(CayenneMessageData* message) 
{
	CAYENNE_LOG_DEBUG("Message received: topic %d, channel %d", message->topic, message->channel);
	switch (message->topic)
	{
	case COMMAND_TOPIC:
		handleMessage(message);
		break;
#ifdef DIGITAL_AND_ANALOG_SUPPORT
	case DIGITAL_COMMAND_TOPIC:
		handleDigitalMessage(message);
		break;
	case DIGITAL_CONFIG_TOPIC:
		configChannel(CayenneArduinoMQTTClient::digitalChannels, message->channel, message->values[0].value);
		break;
	case ANALOG_COMMAND_TOPIC:
		handleAnalogMessage(message);
		break;
	case ANALOG_CONFIG_TOPIC:
		configChannel(CayenneArduinoMQTTClient::analogChannels, message->channel, message->values[0].value);
		break;
#endif
	default:
//#if defined(CAYENNE_DEBUG) && defined(CAYENNE_PRINT)
//		if (message->type) {
//			CAYENNE_PRINT.print("type: ");
//			CAYENNE_PRINT.print(message->type);
//			CAYENNE_PRINT.print(", ");
//		}
//		for (int i = 0; i < message->valueCount; ++i) {
//			if (message->values[i].unit) {
//				CAYENNE_PRINT.print(message->values[i].unit);
//				CAYENNE_PRINT.print("=");
//			}
//			CAYENNE_PRINT.print(message->values[i].value);
//			CAYENNE_PRINT.print(" ");
//		}
//		CAYENNE_PRINT.println();
//#endif
		break;
	}
}

void CayenneArduinoMQTTClient::begin(Client& client, const char* username, const char* password, const char* clientID, int chunkSize)
{
	NetworkInit(&_network, &client, chunkSize);
	CayenneMQTTClientInit(&_mqttClient, &_network, username, password, clientID, CayenneMessageArrived);
	connect();
}

void CayenneArduinoMQTTClient::connect()
{
	CAYENNE_LOG("Connecting to %s:%d", CAYENNE_DOMAIN, CAYENNE_PORT);
	int error = MQTT_FAILURE;
	do {
		if (!NetworkConnect(&_network, CAYENNE_DOMAIN, CAYENNE_PORT)) {
			CAYENNE_LOG("Network connect failed");
			return;
		}
		else if ((error = CayenneMQTTConnect(&_mqttClient)) != MQTT_SUCCESS) {
			CAYENNE_LOG("MQTT connect failed, error %d", error);
			NetworkDisconnect(&_network);
			return;
		}
	}

	while (error != MQTT_SUCCESS);

	CAYENNE_LOG("Connected");
	CayenneConnected();
	CayenneMQTTSubscribe(&_mqttClient, NULL, COMMAND_TOPIC, CAYENNE_ALL_CHANNELS, NULL);
#ifdef DIGITAL_AND_ANALOG_SUPPORT
	CayenneMQTTSubscribe(&_mqttClient, NULL, DIGITAL_COMMAND_TOPIC, CAYENNE_ALL_CHANNELS, NULL);
	CayenneMQTTSubscribe(&_mqttClient, NULL, DIGITAL_CONFIG_TOPIC, CAYENNE_ALL_CHANNELS, NULL);
	CayenneMQTTSubscribe(&_mqttClient, NULL, ANALOG_COMMAND_TOPIC, CAYENNE_ALL_CHANNELS, NULL);
	CayenneMQTTSubscribe(&_mqttClient, NULL, ANALOG_CONFIG_TOPIC, CAYENNE_ALL_CHANNELS, NULL);
#endif
	publishDeviceInfo();
}

void CayenneArduinoMQTTClient::loop(int yieldTime)
{
	CayenneMQTTYield(&_mqttClient, yieldTime);
	static unsigned long lastPoll = millis() - 15000;
	if (millis() - lastPoll > 15000) {
		lastPoll = millis();
		pollVirtualChannels();
	}

#ifdef DIGITAL_AND_ANALOG_SUPPORT
	pollChannels(digitalChannels);
	pollChannels(analogChannels);
#endif
	if (!NetworkConnected(&_network) || !CayenneMQTTConnected(&_mqttClient))
	{
		CayenneMQTTDisconnect(&_mqttClient);
		NetworkDisconnect(&_network);
		CayenneDisconnected();
		CAYENNE_LOG("Disconnected");
		connect();
	}
#ifdef CAYENNE_DEBUG
	else
	{
		static unsigned long lastMillis = millis();
		if (millis() - lastMillis > 10000) {
			lastMillis = millis();
			CAYENNE_LOG_DEBUG("Connection ok");
		}
	}
#endif
}

void CayenneArduinoMQTTClient::publishDeviceInfo()
{
	publishData(SYS_MODEL_TOPIC, CAYENNE_NO_CHANNEL, CAYENNE_FLASH(INFO_DEVICE));
	publishData(SYS_CPU_MODEL_TOPIC, CAYENNE_NO_CHANNEL, CAYENNE_FLASH(INFO_CPU));
	publishData(SYS_CPU_SPEED_TOPIC, CAYENNE_NO_CHANNEL, F_CPU);
	publishData(SYS_VERSION_TOPIC, CAYENNE_NO_CHANNEL, CAYENNE_FLASH(CAYENNE_VERSION));
}
/*
template <typename T> 
void CayenneArduinoMQTTClient::virtualWrite(unsigned int channel, const T& data, const char* type, const char* unit)
{
	publishData(DATA_TOPIC, channel, data, type, unit);
}

void CayenneArduinoMQTTClient::virtualWrite(unsigned int channel, const CayenneDataArray& values, const char* type)
{
	publishData(DATA_TOPIC, channel, values.getArray(), values.getCount(), type);
}
*/
#ifdef CAYENNE_USING_PROGMEM
void CayenneArduinoMQTTClient::virtualWrite(unsigned int channel, const T& data, const __FlashStringHelper* type, const __FlashStringHelper* unit = NULL)
{
	publishData(DATA_TOPIC, channel, data, type, unit);
}

void CayenneArduinoMQTTClient::virtualWrite(unsigned int channel, const CayenneDataArray& values, const __FlashStringHelper* type)
{
	publishData(DATA_TOPIC, channel, values.getArray(), values.getCount(), type);
}
#endif

void CayenneArduinoMQTTClient::responseWrite(const char* error, const char* id)
{
	CAYENNE_LOG_DEBUG("Send response: %s %s", id, error);
	CayenneMQTTPublishResponse(&_mqttClient, NULL, id, error);
}

template <typename T>
void CayenneArduinoMQTTClient::publishState(CayenneTopic topic, unsigned int channel, const T& value)
{
	CAYENNE_LOG_DEBUG("publishState: topic %d channel %u", topic, channel);
	publishData(topic, channel, value);
}

void CayenneArduinoMQTTClient::celsiusWrite(unsigned int channel, float value)
{
	virtualWrite(channel, value, CAYENNE_FLASH(TYPE_TEMPERATURE), CAYENNE_FLASH(UNIT_CELSIUS));
}

void CayenneArduinoMQTTClient::fahrenheitWrite(unsigned int channel, float value)
{
	virtualWrite(channel, value, CAYENNE_FLASH(TYPE_TEMPERATURE), CAYENNE_FLASH(UNIT_FAHRENHEIT));
}

void CayenneArduinoMQTTClient::kelvinWrite(unsigned int channel, float value)
{
	virtualWrite(channel, value, CAYENNE_FLASH(TYPE_TEMPERATURE), CAYENNE_FLASH(UNIT_KELVIN));
}

void CayenneArduinoMQTTClient::luxWrite(unsigned int channel, float value)
{
	virtualWrite(channel, value, CAYENNE_FLASH(TYPE_LUMINOSITY), CAYENNE_FLASH(UNIT_LUX));
}

void CayenneArduinoMQTTClient::pascalWrite(unsigned int channel, float value)
{
	virtualWrite(channel, value, CAYENNE_FLASH(TYPE_BAROMETRIC_PRESSURE), CAYENNE_FLASH(UNIT_PASCAL));
}

void CayenneArduinoMQTTClient::hectoPascalWrite(unsigned int channel, float value)
{
	virtualWrite(channel, value, CAYENNE_FLASH(TYPE_BAROMETRIC_PRESSURE), CAYENNE_FLASH(UNIT_HECTOPASCAL));
}

void CayenneArduinoMQTTClient::digitalSensorWrite(unsigned int channel, int value)
{
	virtualWrite(channel, value, CAYENNE_FLASH(TYPE_DIGITAL_SENSOR), CAYENNE_FLASH(UNIT_DIGITAL));
}

void CayenneArduinoMQTTClient::syncAll()
{
	//Not implemented. This is not needed with MQTT since the broker keeps the last message so we don't need to request it.
}

void CayenneArduinoMQTTClient::syncVirtual(int channel)
{
	//Not implemented. This is not needed with MQTT since the broker keeps the last message so we don't need to request it.
}

void CayenneArduinoMQTTClient::enableChannel(uint32_t channelArray[], uint8_t channel, bool enable)
{
	uint8_t index = channel / 32;
	if (index >= MAX_CHANNEL_ARRAY_SIZE)
		return;

	uint8_t mod = channel % 32;
	if (enable) {
		channelArray[index] |= ((uint32_t)1 << mod);
	}
	else {
		channelArray[index] &= ~((uint32_t)1 << mod);
	}
	CAYENNE_LOG_DEBUG("enableChannel: %d, %d: %lX %lX %lX %lX", channel, (int)enable, channelArray[0], channelArray[1], channelArray[2], channelArray[3]);
}

template <typename T>
void CayenneArduinoMQTTClient::publishData(CayenneTopic topic, unsigned int channel, const T& data, const char* key, const char* subkey)
{
	CayenneDataArray values;
	values.add(subkey, data);
	publishData(topic, channel, values.getArray(), values.getCount(), key);
}

void CayenneArduinoMQTTClient::publishData(CayenneTopic topic, unsigned int channel, const CayenneValuePair values[], size_t valueCount, const char* key) 
{
	CAYENNE_LOG_DEBUG("Publish: topic %d, channel %u, value %s, subkey %s, key %s", topic, channel, values[0].value, values[0].unit, key);
	CayenneMQTTPublishDataArray(&_mqttClient, NULL, topic, channel, key, values, valueCount);
}

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
void CayenneArduinoMQTTClient::publishData(CayenneTopic topic, unsigned int channel, const T& data, const __FlashStringHelper* key, const __FlashStringHelper* subkey) 
{
	char keyBuffer[MAX_TYPE_LENGTH + 1];
	CayenneDataArray values;
	values.add(subkey, data);
	CAYENNE_MEMCPY(keyBuffer, reinterpret_cast<const char *>(key), CAYENNE_STRLEN(reinterpret_cast<const char *>(key)) + 1);
	publishData(topic, channel, values.getArray(), values.getCount(), keyBuffer);
}

/**
* Publish value array using specified topic suffix
* @param topic Cayenne topic
* @param channel Cayenne channel number
* @param values  Array of values to be sent
* @param valueCount  Count of values in array
* @param key Optional key to use for a key=data pair
*/
void CayenneArduinoMQTTClient::publishData(CayenneTopic topic, unsigned int channel, const CayenneValuePair values[], size_t valueCount, const __FlashStringHelper* key) 
{
	char keyBuffer[MAX_TYPE_LENGTH + 1];
	CAYENNE_MEMCPY(keyBuffer, reinterpret_cast<const char *>(key), CAYENNE_STRLEN(reinterpret_cast<const char *>(key)) + 1);
	CAYENNE_LOG_DEBUG("Publish: topic %d, channel %u, value %s, subkey %s, key %s", topic, channel, values[0].value, values[0].unit, keyBuffer);
	CayenneMQTTPublishDataArray(&_mqttClient, NULL, topic, channel, keyBuffer, values, valueCount);
}
#endif

void CayenneArduinoMQTTClient::pollVirtualChannels()
{
	for (unsigned int channel = 0; channel < 32; ++channel) {
		Request request = { channel };
		OutputHandlerFunction handler = GetOutputHandler(request.channel);
		if (handler && handler != OutputHandler) {
			handler(request);
		}
	}
	if (CayenneOutDefault != EmptyHandler) {
		CayenneOutDefault();
	}
}

#ifdef DIGITAL_AND_ANALOG_SUPPORT
/**
* Polls enabled digital channels and sends the matching pin's current value.
*/
void CayenneArduinoMQTTClient::pollChannels(uint32_t channelArray[])
{
	for (size_t index = 0; index < MAX_CHANNEL_ARRAY_SIZE; ++index) {
		if (channelArray[index]) {
			for (size_t flag = 0; flag < 32; ++flag) {
				if (channelArray[index] & ((uint32_t)1 << flag)) {
					unsigned int channel = flag + (index * 32);
					if (channelArray == digitalChannels)
					{
						CAYENNE_LOG_DEBUG("Send digital channel %d %d", channel, digitalRead(channel));
						publishData(DIGITAL_TOPIC, channel, digitalRead(channel));
					}
					else if (channelArray == analogChannels)
					{
						CAYENNE_LOG_DEBUG("Send analog channel %d %d", channel, analogRead(channel));
						publishData(ANALOG_TOPIC, channel, analogRead(channel));
					}
				}
			}
		}
	}
}
#endif
