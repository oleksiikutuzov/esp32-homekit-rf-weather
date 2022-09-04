#include <HomeSpan.h>
#include <Smoothed.h>

#define MHZ19B_TX_PIN		 19
#define MHZ19B_RX_PIN		 18
#define INTERVAL			 10	  // in seconds
#define HOMEKIT_CO2_TRIGGER	 1350 // co2 level, at which HomeKit alarm will be triggered
#define NEOPIXEL_PIN		 16	  // Pin to which NeoPixel strip is connected
#define NUMPIXELS			 1	  // Number of pixels
#define BRIGHTNESS_DEFAULT	 9	  // Default (dimmed) brightness
#define BRIGHTNESS_MAX		 150  // maximum brightness of CO2 indicator led
#define BRIGHTNESS_THRESHOLD 500  // TODO calibrate Threshold value of dimmed brightness
#define ANALOG_PIN			 35	  // Analog pin, to which light sensor is connected
#define SMOOTHING_COEFF		 10	  // Number of elements in the vector of previous values

bool			needToWarmUp  = true;
bool			playInitAnim  = true;
int				tick		  = 0;
bool			airQualityAct = false;
Smoothed<float> mySensor_co2;
Smoothed<float> mySensor_air;
Smoothed<float> mySensor_temp;
Smoothed<float> mySensor_hum;

// Declare functions
void   detect_mhz();
void   fadeIn(int pixel, int r, int g, int b, int brightnessOverride, double duration);
void   fadeOut(int pixel, int r, int g, int b, double duration);
void   initAnimation();
int	   neopixelAutoBrightness();
double getBrightness();

////////////////////////////////////
//   DEVICE-SPECIFIC LED SERVICES //
////////////////////////////////////

// Custom characteristics
// clang-format off
CUSTOM_CHAR(OffsetTemperature, 00000001-0001-0001-0001-46637266EA00, PR + PW + EV, FLOAT, 0.0, -5.0, 5.0, false); // create Custom Characteristic to "select" special effects via Eve App
CUSTOM_CHAR(OffsetHumidity, 00000002-0001-0001-0001-46637266EA00, PR + PW + EV, FLOAT, 0, -10, 10, false);
// clang-format on

struct DEV_TemperatureSensor : Service::TemperatureSensor { // A standalone Air Quality sensor

	SpanCharacteristic *temp; // reference to the Temperature Characteristic

	Characteristic::OffsetTemperature offsetTemp{0.0, true};

	DEV_TemperatureSensor() : Service::TemperatureSensor() { // constructor() method

		temp = new Characteristic::CurrentTemperature(-10.0);
		temp->setRange(-50, 100);

		offsetTemp.setUnit("Deg."); // configures custom "Selector" characteristic for use with Eve HomeKit
		offsetTemp.setDescription("Temperature Offset");
		offsetTemp.setRange(-5.0, 5.0, 0.2);

		mySensor_temp.begin(SMOOTHED_EXPONENTIAL, SMOOTHING_COEFF);

	} // end constructor

	void loop() {

		if (temp->timeVal() > INTERVAL * 1000) { // modify the Temperature Characteristic every 10 seconds

			unsigned int data[2];

			// Convert the data
			float temperature = 20;
			// temperature		  = ((175.72 * temperature) / 65536.0) - 46.85;
			mySensor_temp.add(temperature);
			float offset = offsetTemp.getVal<float>();

			LOG1("Current temperature: ");
			LOG1(mySensor_temp.get());
			LOG1("\n");

			LOG1("Offset: ");
			LOG1(offset);
			LOG1("\n");

			LOG1("Current corrected temperature: ");
			LOG1(mySensor_temp.get() + offset);
			LOG1("\n");

			temp->setVal(mySensor_temp.get() + offset);
		}

	} // loop
};

struct DEV_HumiditySensor : Service::HumiditySensor { // A standalone Air Quality sensor

	SpanCharacteristic			  *hum; // reference to the Temperature Characteristic
	Characteristic::OffsetHumidity offsetHum{0, true};

	DEV_HumiditySensor() : Service::HumiditySensor() { // constructor() method

		hum = new Characteristic::CurrentRelativeHumidity();

		Serial.print("Configuring Humidity Sensor"); // initialization message
		Serial.print("\n");

		offsetHum.setUnit("%"); // configures custom "Selector" characteristic for use with Eve HomeKit
		offsetHum.setDescription("Humidity Offset");
		offsetHum.setRange(-10, 10, 1);

		mySensor_hum.begin(SMOOTHED_EXPONENTIAL, SMOOTHING_COEFF);

	} // end constructor

	void loop() {

		if (hum->timeVal() > INTERVAL * 1000) { // modify the Temperature Characteristic every 10 seconds

			unsigned int data[2];

			// Convert the data
			float humidity = 30;
			// humidity	   = ((125 * humidity) / 65536.0) - 6;
			mySensor_hum.add(humidity);
			float offset = offsetHum.getVal<float>();

			LOG1("Current humidity: ");
			LOG1(mySensor_hum.get());
			LOG1("\n");

			LOG1("Offset: ");
			LOG1(offset);
			LOG1("\n");

			LOG1("Current corrected humidity: ");
			LOG1(mySensor_hum.get() + offset);
			LOG1("\n");

			hum->setVal(mySensor_hum.get() + offset);
		}

	} // loop
};