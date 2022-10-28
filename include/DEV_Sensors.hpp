#include <HomeSpan.h>

// clang-format off
CUSTOM_SERV(Settings, 00000001-0001-0001-0001-46637266EA00);
CUSTOM_CHAR(Selector, 00000002-0001-0001-0001-46637266EA00, PR + PW + EV, UINT8, 1, 1, 3, false); // create Custom Characteristic to "select" special effects via Eve App
CUSTOM_CHAR(LedsOn, 00000003-0001-0001-0001-46637266EA00, PR + PW + EV, BOOL, 0, 0, 1, false);
CUSTOM_CHAR_STRING(IPAddress, 00000005-0001-0001-0001-46637266EA00, PR + EV, "");
CUSTOM_CHAR(Reboot, 00000006-0001-0001-0001-46637266EA00, PR + PW + EV, BOOL, 0, 0, 1, false);
// clang-format on

struct DEV_Settings : Service::Settings {

	Characteristic::Selector  num_sensors{1, true};
	Characteristic::LedsOn    leds_on{true, true};
	Characteristic::IPAddress ip_address{"0.0.0.0"};
	Characteristic::Reboot    reboot{false, false};

	DEV_Settings() : Service::Settings() { // constructor() method

		num_sensors.setUnit(""); // configures custom "Selector" characteristic for use with Eve HomeKit
		num_sensors.setDescription("Number of channels");
		num_sensors.setRange(1, 3, 1);

		leds_on.setDescription("Blink LEDs on receive");

		ip_address.setDescription("IP Address");

		reboot.setDescription("Reboot Device");

		LOG0("Configuring Settings Service"); // initialization message
		LOG0("\n");

	} // end constructor

	boolean update() override {

		if (reboot.updated()) {
			if (reboot.getNewVal() == true) {
				LOG1("Rebooting");
				vTaskDelay(1000 / portTICK_PERIOD_MS);
				ESP.restart();
			}
		}

		return (true);
	}
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
	SpanCharacteristic *fault;   // reference to Fault Characteristic

	DEV_HumiditySensor() : Service::HumiditySensor() { // constructor() method

		hum = new Characteristic::CurrentRelativeHumidity(0, true);

		battery = new Characteristic::StatusLowBattery();

		fault = new Characteristic::StatusFault(0);

		LOG0("Configuring Humidity Sensor"); // initialization message
		LOG0("\n");

	} // end constructor

	void loop() {

		if (hum->timeVal() > 5 * 60 * 1000 && !fault->getVal()) { // else if it has been a while since last update (120 seconds), and there is no current fault
			fault->setVal(1);                                     // set fault state
			LOG1("Sensor update: FAULT\n");
		}
	} // loop
};