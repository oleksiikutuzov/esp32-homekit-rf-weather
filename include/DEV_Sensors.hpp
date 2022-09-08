#include <HomeSpan.h>

struct DEV_TemperatureSensor : Service::TemperatureSensor { // A standalone Air Quality sensor

	SpanCharacteristic *temp; // reference to the Temperature Characteristic

	DEV_TemperatureSensor() : Service::TemperatureSensor() { // constructor() method

		temp = new Characteristic::CurrentTemperature(0, true);
		temp->setRange(-50, 100);

	} // end constructor

	void loop() {

	} // loop
};

struct DEV_HumiditySensor : Service::HumiditySensor { // A standalone Air Quality sensor

	SpanCharacteristic *hum; // reference to the Temperature Characteristic

	DEV_HumiditySensor() : Service::HumiditySensor() { // constructor() method

		hum = new Characteristic::CurrentRelativeHumidity(0, true);

		Serial.print("Configuring Humidity Sensor"); // initialization message
		Serial.print("\n");

	} // end constructor

	void loop() {
	}

	// } // loop
};