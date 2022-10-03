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

// TODO Status fault when no data for some time

#define REQUIRED VERSION(1, 6, 0)

#include "DEV_Sensors.hpp"
#include "OTA.hpp"
#include <ElegantOTA.h>
#include <HomeSpan.h>
#include <WebServer.h>
#include <WiFiClient.h>

#include <ArduinoJson.h>
#include <ArduinoLog.h>
#include <rtl_433_ESP.h>

// *** Defines for RF receiver ***
#ifndef RF_MODULE_FREQUENCY
	#define RF_MODULE_FREQUENCY 433.92
#endif

#define JSON_MSG_BUFFER 512

#ifdef DEV_BOARD
	#define LED_CH1    13
	#define LED_CH2    21
	#define LED_CH3    17
	#define LED_STATUS 16
	#define BUTTON_PIN 15
#else
	#define LED_CH1    13
	#define LED_CH2    15
	#define LED_CH3    17
	#define LED_STATUS 32
	#define BUTTON_PIN 21
#endif

void logJson(JsonObject &jsondata);
void blinkLed(int pin);
void setupWeb();

char messageBuffer[JSON_MSG_BUFFER];

rtl_433_ESP rf(-1); // use -1 to disable transmitter

WebServer server(80);

char sNumber[18] = "11:11:11:11:11:11";

DEV_Settings *SETTINGS;

int                    led_pin[3] = {LED_CH1, LED_CH2, LED_CH3};
DEV_TemperatureSensor *TEMP_SENSORS[3];
DEV_HumiditySensor    *HUM_SENSORS[3];
int                    receivedPackets[3];

void rtl_433_Callback(char *message) {
	DynamicJsonBuffer jsonBuffer2(JSON_MSG_BUFFER);
	JsonObject       &RFrtl_433_ESPdata = jsonBuffer2.parseObject(message);
	logJson(RFrtl_433_ESPdata);

	const char *model      = RFrtl_433_ESPdata["model"];
	int         id         = RFrtl_433_ESPdata["id"];
	int         channel    = RFrtl_433_ESPdata["channel"];
	bool        battery_ok = RFrtl_433_ESPdata["battery_ok"];
	String      battery    = (battery_ok == 1) ? "Ok" : "Low";
	float       temp       = RFrtl_433_ESPdata["temperature_C"];
	float       hum        = RFrtl_433_ESPdata["humidity"];

	// in this sensor channels are encoded as 0...2
	if (RFrtl_433_ESPdata["model"] == "LaCrosse-TX141THBv2") {
		channel++;
	}

	// get number of channels
	int  sensors = SETTINGS->num_sensors.getVal();
	bool led_on  = SETTINGS->leds_on.getVal();

	for (int i = 0; i < sensors; i++) {
		if (hum > 0 && temp > 0) {
			if (i + 1 == channel) {
				LOG1("\n");
				LOG1("Received on channel " + String(i + 1));

				LOG1("\n");
				LOG1("Model: " + (String)model);
				LOG1("\n");
				LOG1("ID: " + (String)id);
				LOG1("\n");
				LOG1("Channel: " + (String)channel);
				LOG1("\n");
				LOG1("Battery: " + battery);
				LOG1("\n");
				LOG1("Temperature: " + (String)temp);
				LOG1("\n");
				LOG1("Humidity: " + (String)hum);
				LOG1("\n");

				receivedPackets[i]++;
				if (led_on == true) {
					blinkLed(led_pin[i]);
				}

				HUM_SENSORS[i]->hum->setVal(hum);
				HUM_SENSORS[i]->fault->setVal(0);
				TEMP_SENSORS[i]->temp->setVal(temp);
				if (battery_ok == true) {
					HUM_SENSORS[i]->battery->setVal(0);
				} else {
					HUM_SENSORS[i]->battery->setVal(1);
				}
			}
		}
	}
}

void logJson(JsonObject &jsondata) {
	char JSONmessageBuffer[jsondata.measureLength() + 1];
	jsondata.printTo(JSONmessageBuffer, sizeof(JSONmessageBuffer));
	Log.notice(F("Received message : %s" CR), JSONmessageBuffer);
}

void setup() {

	Serial.begin(115200);

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

	String     temp           = FW_VERSION;
	const char compile_date[] = __DATE__ " " __TIME__;
	char      *fw_ver         = new char[temp.length() + 30];
	strcpy(fw_ver, temp.c_str());
	strcat(fw_ver, " (");
	strcat(fw_ver, compile_date);
	strcat(fw_ver, ")");

	homeSpan.setControlPin(BUTTON_PIN);                        // Set button pin
	homeSpan.setStatusPin(LED_STATUS);                         // Set status led pin
	homeSpan.setLogLevel(1);                                   // set log level
	homeSpan.setPortNum(88);                                   // change port number for HomeSpan so we can use port 80 for the Web Server
	homeSpan.setStatusAutoOff(10);                             // turn off status led after 10 seconds of inactivity
	homeSpan.setWifiCallback(setupWeb);                        // need to start Web Server after WiFi is established
	homeSpan.reserveSocketConnections(3);                      // reserve 3 socket connections for Web Server
	homeSpan.enableWebLog(10, "pool.ntp.org", "UTC", "myLog"); // enable Web Log
	homeSpan.enableAutoStartAP();                              // enable auto start AP
	homeSpan.setSketchVersion(fw_ver);

	homeSpan.begin(Category::Bridges, "HomeSpan RF Weather Bridge");

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::FirmwareRevision(temp.c_str());

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("RF Weather Bridge Settings");
	SETTINGS = new DEV_Settings();

	// get number of channels
	int sensors = SETTINGS->num_sensors.getVal();

	for (int i = 0; i < 3; i++) {
		pinMode(led_pin[i], OUTPUT);
	}

	for (int i = 0; i < sensors; i++) {

		// blink led
		blinkLed(led_pin[i]);

		String channel_string   = "Channel ";
		String temp_name_string = "Temperature Sensor CH";
		String hum_name_string  = "Humidity Sensor CH";

		char *channel_char   = new char[channel_string.length() + 2];
		char *temp_name_char = new char[temp_name_string.length() + 2];
		char *hum_name_char  = new char[hum_name_string.length() + 2];
		char  channel_num[1];

		sprintf(channel_num, "%d", i + 1);

		strcpy(channel_char, channel_string.c_str());
		strcpy(temp_name_char, temp_name_string.c_str());
		strcpy(hum_name_char, hum_name_string.c_str());

		strcat(channel_char, channel_num);
		strcat(temp_name_char, channel_num);
		strcat(hum_name_char, channel_num);

		new SpanAccessory();
		new Service::AccessoryInformation();
		new Characteristic::Identify();
		new Characteristic::Name(temp_name_char);
		new Characteristic::Model(channel_char);
		TEMP_SENSORS[i] = new DEV_TemperatureSensor();

		new SpanAccessory();
		new Service::AccessoryInformation();
		new Characteristic::Identify();
		new Characteristic::Name(hum_name_char);
		new Characteristic::Model(channel_char);
		HUM_SENSORS[i] = new DEV_HumiditySensor();
	}
}

void loop() {
	homeSpan.poll();
	server.handleClient();
	if (SETTINGS->auto_update.getVal() == true)
		repeatedCall();
	rf.loop();
}

void setupWeb() {
	LOG0("Starting Air Quality Sensor Server Hub...\n\n");

	server.on("/metrics", HTTP_GET, []() {
		// get number of channels
		int sensors = SETTINGS->num_sensors.getVal();

		float  temps[3];
		float  hums[3];
		String temp_metrics[3];
		String hum_metrics[3];
		String received_metrics[3];

		for (int i = 0; i < sensors; i++) {
			temps[i] = TEMP_SENSORS[i]->temp->getVal<float>();
			hums[i]  = HUM_SENSORS[i]->hum->getVal<float>();
		}

		float  uptime       = esp_timer_get_time() / (6 * 10e6);
		float  heap         = esp_get_free_heap_size();
		String uptimeMetric = "# HELP uptime Sensor uptime\nhomekit_uptime{device=\"rf_bridge\",location=\"home\"} " + String(int(uptime));
		String heapMetric   = "# HELP heap Available heap memory\nhomekit_heap{device=\"rf_bridge\",location=\"home\"} " + String(int(heap));

		LOG1("\n");
		LOG1(uptimeMetric);
		LOG1("\n");
		LOG1(heapMetric);
		LOG1("\n");

		for (int i = 0; i < sensors; i++) {
			temp_metrics[i]     = "# HELP temp Temperature\nhomekit_temperature{device=\"rf_bridge\",channel=\"" + String(i + 1) + "\",location=\"home\"} " + String(temps[i]);
			hum_metrics[i]      = "# HELP hum Relative Humidity\nhomekit_humidity{device=\"rf_bridge\",channel=\"" + String(i + 1) + "\",location=\"home\"} " + String(hums[i]);
			received_metrics[i] = "# HELP received Number of received samples\nhomekit_received{device=\"rf_bridge\",channel=\"" + String(i + 1) + "\",location=\"home\"} " + String(receivedPackets[i]);

			LOG1(temp_metrics[i]);
			LOG1("\n");
			LOG1(hum_metrics[i]);
			LOG1("\n");
			LOG1(received_metrics[i]);
			LOG1("\n");
		}

		if (sensors == 1) {
			server.send(200, "text/plain", uptimeMetric + "\n" + heapMetric + "\n" + temp_metrics[0] + "\n" + hum_metrics[0] + "\n" + received_metrics[0]);
		} else if (sensors == 2) {
			server.send(200, "text/plain", uptimeMetric + "\n" + heapMetric + "\n" + temp_metrics[0] + "\n" + hum_metrics[0] + "\n" + received_metrics[0] + "\n" + temp_metrics[1] + "\n" + hum_metrics[1] + "\n" + received_metrics[1]);
		} else if (sensors == 3) {
			server.send(200, "text/plain", uptimeMetric + "\n" + heapMetric + "\n" + temp_metrics[0] + "\n" + hum_metrics[0] + "\n" + received_metrics[0] + "\n" + temp_metrics[1] + "\n" + hum_metrics[1] + "\n" + received_metrics[1] + "\n" + temp_metrics[2] + "\n" + hum_metrics[2] + "\n" + received_metrics[2]);
		}
	});

	server.on("/reboot", HTTP_GET, []() {
		String content = "<html><body>Rebooting!  Will return to configuration "
		                 "page in 10 seconds.<br><br>";
		content += "<meta http-equiv = \"refresh\" content = \"10; url = /\" />";
		server.send(200, "text/html", content);

		ESP.restart();
	});

	SETTINGS->ip_address.setString(WiFi.localIP().toString().c_str());
	ElegantOTA.begin(&server); // Start ElegantOTA
	server.begin();
	Serial.println("HTTP server started");
} // setupWeb

void blinkLed(int pin) {

	digitalWrite(pin, HIGH);
	delay(100);
	digitalWrite(pin, LOW);
}