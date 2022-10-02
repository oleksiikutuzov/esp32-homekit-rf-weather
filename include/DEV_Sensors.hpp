#include <HomeSpan.h>

// clang-format off
CUSTOM_CHAR(Selector, 00000001-0001-0001-0001-46637266EA00, PR + PW + EV, UINT8, 1, 1, 3, false); // create Custom Characteristic to "select" special effects via Eve App
CUSTOM_CHAR(LedsOn, 00000002-0001-0001-0001-46637266EA00, PR + PW + EV, BOOL, 0, 0, 1, true);
CUSTOM_CHAR(AutoUpdate, 00000003-0001-0001-0001-46637266EA00, PR + PW + EV, BOOL, 0, 0, 1, true);
CUSTOM_CHAR_STRING(IPAddress, 00000004-0001-0001-0001-46637266EA00, PR + EV, "");
// clang-format on

struct DEV_Settings : Service::StatelessProgrammableSwitch {

	SpanCharacteristic        *switchEvent;
	Characteristic::Selector   num_sensors{1, true};
	Characteristic::LedsOn     leds_on{true, true};
	Characteristic::AutoUpdate auto_update{true, true};
	Characteristic::IPAddress  ip_address{"0.0.0.0"};

	DEV_Settings() : Service::StatelessProgrammableSwitch() { // constructor() method

		switchEvent = new Characteristic::ProgrammableSwitchEvent();

		num_sensors.setUnit(""); // configures custom "Selector" characteristic for use with Eve HomeKit
		num_sensors.setDescription("Number of channels");
		num_sensors.setRange(1, 3, 1);

		leds_on.setDescription("LEDs on");

		auto_update.setDescription("Auto OTA Update");

		ip_address.setDescription("IP Address");

		LOG0("Configuring Settings Service"); // initialization message
		LOG0("\n");

	} // end constructor
};

struct DEV_TemperatureSensor : Service::TemperatureSensor {

	SpanCharacteristic *temp; // reference to the Temperature Characteristic

	DEV_TemperatureSensor() : Service::TemperatureSensor() { // constructor() method

		temp = new Characteristic::CurrentTemperature(0, true);
		temp->setRange(-50, 100);

		LOG0("Configuring Temperature Sensor"); // initialization message
		LOG0("\n");

	} // end constructor
};

struct DEV_HumiditySensor : Service::HumiditySensor {

	SpanCharacteristic *hum;     // reference to the Humidity Characteristic
	SpanCharacteristic *battery; // reference to Low Battery Characteristic

	DEV_HumiditySensor() : Service::HumiditySensor() { // constructor() method

		hum = new Characteristic::CurrentRelativeHumidity(0, true);

		battery = new Characteristic::StatusLowBattery();

		LOG0("Configuring Humidity Sensor"); // initialization message
		LOG0("\n");

	} // end constructor
};