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

#define REQUIRED VERSION(1, 6, 0)

#include "DEV_Sensors.hpp"
#include <WiFiClient.h>
#include <WebServer.h>
#include <ElegantOTA.h>
#include <HomeSpan.h>
#include "OTA.hpp"

#include <rtl_433_ESP.h>
#include <ArduinoJson.h>
#include <ArduinoLog.h>

#define BUTTON_PIN	   0
#define LED_STATUS_PIN 26

WebServer server(80);

extern "C++" bool needToWarmUp;

void setupWeb();

DEV_TemperatureSensor *TEMP;
DEV_HumiditySensor	  *HUM;

void setup() {

	Serial.begin(115200);

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
	TEMP = new DEV_TemperatureSensor(); // Create a Temperature Sensor (see DEV_Sensors.h for definition)

	new SpanAccessory();
	new Service::AccessoryInformation();
	new Characteristic::Identify();
	new Characteristic::Name("Humidity Sensor");
	HUM = new DEV_HumiditySensor(); // Create a Temperature Sensor (see DEV_Sensors.h for definition)
}

void loop() {
	homeSpan.poll();
	server.handleClient();
	// repeatedCall();
}

void setupWeb() {
	LOG0("Starting Air Quality Sensor Server Hub...\n\n");

	server.on("/metrics", HTTP_GET, []() {
		float  temp	  = TEMP->temp->getVal<float>();
		float  hum	  = HUM->hum->getVal<float>();
		float  uptime = esp_timer_get_time() / (6 * 10e6);
		float  heap	  = esp_get_free_heap_size();
		String uptimeMetric =
			"# HELP uptime Sensor "
			"uptime\nhomekit_uptime{device=\"air_sensor\",location=\"home\"} " +
			String(int(uptime));
		String heapMetric =
			"# HELP heap Available heap "
			"memory\nhomekit_heap{device=\"air_sensor\",location=\"home\"} " +
			String(int(heap));

		String tempMetric = "# HELP temp "
							"Temperature\nhomekit_temperature{device=\"air_"
							"sensor\",location=\"home\"} " +
							String(temp);
		String humMetric =
			"# HELP hum Relative "
			"Humidity\nhomekit_humidity{device=\"air_sensor\",location=\"home\"} " +
			String(hum);

		LOG1(uptimeMetric);
		LOG1(heapMetric);

		LOG1(tempMetric);
		LOG1(humMetric);

		server.send(200, "text/plain",
					uptimeMetric + "\n" + heapMetric + "\n" + tempMetric + "\n" + humMetric);
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
