#include <HomeSpan.h>

struct DEV_TemperatureSensor : Service::TemperatureSensor { // A standalone Air Quality sensor

	SpanCharacteristic *temp; // reference to the Temperature Characteristic

	DEV_TemperatureSensor() : Service::TemperatureSensor() { // constructor() method

		temp = new Characteristic::CurrentTemperature(0, true);
		temp->setRange(-50, 100);

		LOG0("Configuring Temperature Sensor"); // initialization message
		LOG0("\n");

	} // end constructor
};

struct DEV_HumiditySensor : Service::HumiditySensor { // A standalone Air Quality sensor

	SpanCharacteristic *hum;     // reference to the Humidity Characteristic
	SpanCharacteristic *battery; // reference to Low Battery Characteristic

	DEV_HumiditySensor() : Service::HumiditySensor() { // constructor() method

		hum = new Characteristic::CurrentRelativeHumidity(0, true);

		battery = new Characteristic::StatusLowBattery();

		LOG0("Configuring Humidity Sensor"); // initialization message
		LOG0("\n");

	} // end constructor
};