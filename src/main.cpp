/*********************************************************************************
 *  MIT License
 *
 *  Copyright (c) 2020 Gregg E. Berman
 *
 *  https://github.com/HomeSpan/HomeSpan
 *
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to
 *deal in the Software without restriction, including without limitation the
 *rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 *sell copies of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be included in
 *all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 *FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 *IN THE SOFTWARE.
 *
 ********************************************************************************/

/*
 *                  ESP-WROOM-32 Utilized pins
 *                ╔═════════════════════════════╗
 *                ║┌─┬─┐  ┌──┐  ┌─┐             ║
 *                ║│ | └──┘  └──┘ |             ║
 *                ║│ |            |             ║
 *                ╠═════════════════════════════╣
 *            +++ ║GND                       GND║ +++
 *            +++ ║3.3V                     IO23║ USED_FOR_NOTHING
 *                ║                         IO22║
 *                ║IO36                      IO1║ TX
 *                ║IO39                      IO3║ RX
 *                ║IO34                     IO21║
 *                ║IO35                         ║ NC
 *        RED_LED ║IO32                     IO19║
 *                ║IO33                     IO18║
 *                ║IO25                      IO5║
 *     LED_YELLOW ║IO26                     IO17║
 *                ║IO27                     IO16║ NEOPIXEL
 *                ║IO14                      IO4║
 *                ║IO12                      IO0║ +++, BUTTONS
 *                ╚═════════════════════════════╝
 */

// TODO battery status

#define REQUIRED VERSION(1, 6, 0)

#ifndef NUM_CHANNELS
#define NUM_CHANNELS 3 // set number of channels here (1...3)
#endif

#include "DEV_Sensors.hpp"
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include <HomeSpan.h>
#include "OTA.hpp"

#include <rtl_433_ESP.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>

// *** Defines for RF receiver ***
#ifndef RF_MODULE_FREQUENCY
#define RF_MODULE_FREQUENCY 433.92
#endif

#define JSON_MSG_BUFFER 512

void logJson(JsonObject &jsondata);

char messageBuffer[JSON_MSG_BUFFER];

rtl_433_ESP rf(-1); // use -1 to disable transmitter
// ***

#define BUTTON_PIN	   0
#define LED_STATUS_PIN 26

WebServer server(80);

char sNumber[18] = "11:11:11:11:11:11";

void setupWeb();

DEV_TemperatureSensor *TEMP_1;
DEV_HumiditySensor	  *HUM_1;
int					   receivedPackets_1 = 0;

#if NUM_CHANNELS >= 2
DEV_TemperatureSensor *TEMP_2;
DEV_HumiditySensor	  *HUM_2;
int					   receivedPackets_2 = 0;
#endif

#if NUM_CHANNELS == 3
DEV_TemperatureSensor *TEMP_3;
DEV_HumiditySensor	  *HUM_3;
int					   receivedPackets_3 = 0;
#endif

void rtl_433_Callback(char *message) {
	DynamicJsonBuffer jsonBuffer2(JSON_MSG_BUFFER);
	JsonObject		 &RFrtl_433_ESPdata = jsonBuffer2.parseObject(message);
	logJson(RFrtl_433_ESPdata);

	const char *model	   = RFrtl_433_ESPdata["model"];
	int			id		   = RFrtl_433_ESPdata["id"];
	int			channel	   = RFrtl_433_ESPdata["channel"];
	bool		battery_ok = RFrtl_433_ESPdata["battery_ok"];
	String		battery	   = (battery_ok == 1) ? "Ok" : "Low";
	float		temp	   = RFrtl_433_ESPdata["temperature_C"];
	float		hum		   = RFrtl_433_ESPdata["humidity"];

	Serial.println("Model: " + (String)model);
	Serial.println("ID: " + (String)id);
	Serial.println("Channel: " + (String)channel);
	Serial.println("Battery: " + battery);
	Serial.println("Temperature: " + (String)temp);
	Serial.println("Humidity: " + (String)hum);

	if (RFrtl_433_ESPdata["channel"] == 1) {

		Serial.println("Received on channel 1");

		receivedPackets_1++;

		if (hum != 0 && temp != 0) {
			HUM_1->hum->setVal(hum);
			TEMP_1->temp->setVal(temp);
		}
	}

#if NUM_CHANNELS >= 2
	if (RFrtl_433_ESPdata["channel"] == 2) {

		Serial.println("Received on channel 2");

		receivedPackets_2++;

		if (hum != 0 && temp != 0) {
			HUM_2->hum->setVal(hum);
			TEMP_2->temp->setVal(temp);
		}
	}
#endif

#if NUM_CHANNELS == 3
	if (RFrtl_433_ESPdata["channel"] == 3) {

		Serial.println("Received on channel 3");

		receivedPackets_3++;

		if (hum != 0 && temp != 0) {
			HUM_3->hum->setVal(hum);
			TEMP_3->temp->setVal(temp);
		}
	}
#endif
}

void logJson(JsonObject &jsondata) {
#if defined(ESP8266) || defined(ESP32) || defined(__AVR_ATmega2560__) || \
	defined(__AVR_ATmega1280__)
	char JSONmessageBuffer[jsondata.measureLength() + 1];
#else
	char JSONmessageBuffer[JSON_MSG_BUFFER];
#endif
	jsondata.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
	Log.notice(F("Received message : %s" CR), JSONmessageBuffer);
}

void setup() {

	Serial.begin(115200);

	delay(1000);

	Log.begin(LOG_LEVEL, &Serial);
	Log.notice(F(" " CR));
	Log.notice(F("****** setup ******" CR));
	rf.initReceiver(RF_MODULE_RECEIVER_GPIO, RF_MODULE_FREQUENCY);
	rf.setCallback(rtl_433_Callback, messageBuffer, JSON_MSG_BUFFER);
	//  ELECHOUSE_cc1101.SetRx(CC1101_FREQUENCY); // set Receive on
	rf.enableReceiver(RF_MODULE_RECEIVER_GPIO);
	Log.notice(F("****** setup complete ******" CR));

	Serial.print("Active firmware version: ");
	Serial.println(FirmwareVer);

	String	   temp			  = FW_VERSION;
	const char compile_date[] = __DATE__ " " __TIME__;
	char	  *fw_ver		  = new char[temp.length() + 30];
	strcpy(fw_ver, temp.c_str());
	strcat(fw_ver, " (");
	strcat(fw_ver, compile_date);
	strcat(fw_ver, ")");

	homeSpan.setControlPin(BUTTON_PIN);						   // Set button pin
	homeSpan.setStatusPin(LED_STATUS_PIN);					   // Set status led pin
	homeSpan.setLogLevel(1);								   // set log level
	homeSpan.setPortNum(88);								   // change port number for HomeSpan so we can use port 80 for the Web Server
	homeSpan.setStatusAutoOff(10);							   // turn off status led after 10 seconds of inactivity
	homeSpan.setWifiCallback(setupWeb);						   // need to start Web Server after WiFi is established
	homeSpan.reserveSocketConnections(3);					   // reserve 3 socket connections for Web Server
	homeSpan.enableWebLog(10, "pool.ntp.org", "UTC", "myLog"); // enable Web Log
	homeSpan.enableAutoStartAP();							   // enable auto start AP
	homeSpan.setSketchVersion(fw_ver);

	homeSpan.begin(Category::Bridges, "HomeSpan RF Weather Bridge");

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::FirmwareRevision(temp.c_str());

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("Temperature Sensor");
	new Characteristic::Model("Channel 1");
	TEMP_1 = new DEV_TemperatureSensor(); // Create a Temperature Sensor (see DEV_Sensors.h for definition)

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("Humidity Sensor");
	new Characteristic::Model("Channel 1");
	HUM_1 = new DEV_HumiditySensor();

#if NUM_CHANNELS >= 2
	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("Temperature Sensor");
	new Characteristic::Model("Channel 2");
	TEMP_2 = new DEV_TemperatureSensor();

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("Humidity Sensor");
	new Characteristic::Model("Channel 2");
	HUM_2 = new DEV_HumiditySensor();
#endif

#if NUM_CHANNELS == 3
	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("Temperature Sensor");
	new Characteristic::Model("Channel 3");
	TEMP_3 = new DEV_TemperatureSensor();

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("Humidity Sensor");
	new Characteristic::Model("Channel 3");
	HUM_3 = new DEV_HumiditySensor();
#endif
}

void loop() {
	homeSpan.poll();
	server.handleClient();
	repeatedCall();
	rf.loop();
}

void setupWeb() {
	LOG0("Starting Air Quality Sensor Server Hub...\n\n");

	server.on("/metrics", HTTP_GET, []() {
		float temp_1 = TEMP_1->temp->getVal<float>();
		float hum_1	 = HUM_1->hum->getVal<float>();

#if NUM_CHANNELS >= 2
		float temp_2 = TEMP_2->temp->getVal<float>();
		float hum_2	 = HUM_2->hum->getVal<float>();
#endif

#if NUM_CHANNELS == 3
		float temp_3 = TEMP_3->temp->getVal<float>();
		float hum_3	 = HUM_3->hum->getVal<float>();
#endif
		float  uptime		= esp_timer_get_time() / (6 * 10e6);
		float  heap			= esp_get_free_heap_size();
		String uptimeMetric = "# HELP uptime Sensor uptime\nhomekit_uptime{device=\"rf_bridge\",location=\"home\"} " + String(int(uptime));
		String heapMetric	= "# HELP heap Available heap memory\nhomekit_heap{device=\"rf_bridge\",location=\"home\"} " + String(int(heap));

		String tempMetric_1		= "# HELP temp Temperature\nhomekit_temperature{device=\"rf_bridge\",channel=\"1\",location=\"home\"} " + String(temp_1);
		String humMetric_1		= "# HELP hum Relative Humidity\nhomekit_humidity{device=\"rf_bridge\",channel=\"1\",location=\"home\"} " + String(hum_1);
		String receivedMetric_1 = "# HELP received Number of received samples\nhomekit_received{device=\"rf_bridge\",channel=\"1\",location=\"home\"} " + String(receivedPackets_1);

		LOG1("\n");
		LOG1(uptimeMetric);
		LOG1("\n");
		LOG1(heapMetric);
		LOG1("\n");
		LOG1(tempMetric_1);
		LOG1("\n");
		LOG1(humMetric_1);
		LOG1("\n");
		LOG1(receivedMetric_1);
		LOG1("\n");

#if NUM_CHANNELS >= 2
		String tempMetric_2		= "# HELP temp Temperature\nhomekit_temperature{device=\"rf_bridge\",channel=\"2\",location=\"home\"} " + String(temp_2);
		String humMetric_2		= "# HELP hum Relative Humidity\nhomekit_humidity{device=\"rf_bridge\",channel=\"2\",location=\"home\"} " + String(hum_2);
		String receivedMetric_2 = "# HELP received Number of received samples\nhomekit_received{device=\"rf_bridge\",channel=\"2\",location=\"home\"} " + String(receivedPackets_2);

		LOG1(tempMetric_2);
		LOG1("\n");
		LOG1(humMetric_2);
		LOG1("\n");
		LOG1(receivedMetric_2);
		LOG1("\n");
#endif

#if NUM_CHANNELS == 3
		String tempMetric_3		= "# HELP temp Temperature\nhomekit_temperature{device=\"rf_bridge\",channel=\"3\",location=\"home\"} " + String(temp_2);
		String humMetric_3		= "# HELP hum Relative Humidity\nhomekit_humidity{device=\"rf_bridge\",channel=\"3\",location=\"home\"} " + String(hum_2);
		String receivedMetric_3 = "# HELP received Number of received samples\nhomekit_received{device=\"rf_bridge\",channel=\"3\",location=\"home\"} " + String(receivedPackets_2);

		LOG1(tempMetric_3);
		LOG1("\n");
		LOG1(humMetric_3);
		LOG1("\n");
		LOG1(receivedMetric_3);
		LOG1("\n");
#endif

#if NUM_CHANNELS == 1
		server.send(200, "text/plain", uptimeMetric + "\n" + heapMetric + "\n" + tempMetric_1 + "\n" + humMetric_1 + "\n" + receivedMetric_1);
#elif NUM_CHANNELS == 2
			server.send(200, "text/plain", uptimeMetric + "\n" + heapMetric + "\n" + tempMetric_1 + "\n" + humMetric_1 + "\n" + receivedMetric_1 + "\n" + tempMetric_2 + "\n" + humMetric_2 + "\n" + receivedMetric_2);
#elif NUM_CHANNELS == 3
			server.send(200, "text/plain", uptimeMetric + "\n" + heapMetric + "\n" + tempMetric_1 + "\n" + humMetric_1 + "\n" + receivedMetric_1 + "\n" + tempMetric_2 + "\n" + humMetric_2 + "\n" + receivedMetric_2 + "\n" + tempMetric_3 + "\n" + humMetric_3 + "\n" + receivedMetric_3);
#endif
	});

	server.on("/reboot", HTTP_GET, []() {
		String content = "<html><body>Rebooting!  Will return to configuration "
						 "page in 10 seconds.<br><br>";
		content += "<meta http-equiv = \"refresh\" content = \"10; url = /\" />";
		server.send(200, "text/html", content);

		ESP.restart();
	});

	ElegantOTA.begin(&server); // Start ElegantOTA
	server.begin();
	Serial.println("HTTP server started");
} // setupWeb
