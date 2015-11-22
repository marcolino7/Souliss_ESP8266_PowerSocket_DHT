/**************************************************************************
    Souliss - Power Socket Porting for Expressif ESP8266
			  with DHT22 Temperatue and Humidity	

	It use static IP Addressing
	Adafruit DHT library: https://github.com/adafruit/DHT-sensor-library

    Load this code on ESP8266 board using the porting of the Arduino core
    for this platform.
        
***************************************************************************/

//#define USE_DHT

// Configure the framework
#include "souliss/bconf/MCU_ESP8266.h"              // Load the code directly on the ESP8266

// **** Define the WiFi name and password ****
#include "D:\__User\Administrator\Documents\Privati\ArduinoWiFiInclude\wifi.h"
//To avoide to share my wifi credentials on git, I included them in external file
//To setup your credentials remove my include, un-comment below 3 lines and fill with
//Yours wifi credentials
//#define WIFICONF_INSKETCH
//#define WiFi_SSID               "wifi_name"
//#define WiFi_Password           "wifi_password"    

// Include framework code and libraries
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <EEPROM.h>
#include "Souliss.h"

#ifdef USE_DHT
	#include "DHT-sensor-library\dht.h"
#endif

#define VNET_DEBUG_INSKETCH
#define VNET_DEBUG  		0

// Define the network configuration according to your router settings
uint8_t ip_address[4]  = {192, 168, 1, 131};
uint8_t subnet_mask[4] = {255, 255, 255, 0};
uint8_t ip_gateway[4]  = {192, 168, 1, 1};


// This identify the number of the LED logic
#define POWER_SOCKET    0
#ifdef USE_DHT
	#define DHT_TEMP		1
	#define DHT_HUMI		3
#endif     
// **** Define here the right pin for your ESP module **** 
#define	PIN_RELE			5
#define PIN_BUTTON			0
#define PIN_LED				16
#define PIN_DHT				4

// Identify the sensor, in case of more than one used on the same board
#ifdef USE_DHT
	#define DHTTYPE DHT22
	DHT dht(PIN_DHT, DHTTYPE, 15);
	#define DEADBANDLOW	  0.005
#endif

//Useful Variable
byte led_status = 0;
byte joined = 0;
U8 value_hold=0x068;

OTA_Setup();

void setup()
{   
    Initialize();

    // Connect to the WiFi network with static IP
	Souliss_SetIPAddress(ip_address, subnet_mask, ip_gateway);
    
	// Define a T11 light logic
	Set_SimpleLight(POWER_SOCKET);			

	#ifdef USE_DHT
		//T52 Temperatur DHT
		Souliss_SetT52(memory_map, DHT_TEMP);
		//T53 Umidità
		Souliss_SetT53(memory_map, DHT_HUMI);
	#endif

    pinMode(PIN_RELE, OUTPUT);				// Use pin as output
	pinMode(PIN_BUTTON,INPUT);				// Use pin as input
	pinMode(PIN_LED, OUTPUT);				// Use pin as output
	
	Serial.begin(115200);
    Serial.println("Node Init");
}

void loop()
{ 
    // Here we start to play
    EXECUTEFAST() {                     
        UPDATEFAST();   
        
        FAST_50ms() {   // We process the logic and relevant input and output every 50 milliseconds
			
			// Detect the button press. Short press toggle, long press reset the node
			U8 invalue = LowDigInHold(PIN_BUTTON,Souliss_T1n_ToggleCmd,value_hold,POWER_SOCKET);
			if(invalue==Souliss_T1n_ToggleCmd){
				Serial.println("TOGGLE");
				mInput(POWER_SOCKET)=Souliss_T1n_ToggleCmd;
			} else if(invalue==value_hold) {
				// reset
				Serial.println("REBOOT");
				delay(1000);
				ESP.reset();
			}
			
			//Output Handling
			DigOut(PIN_RELE, Souliss_T1n_Coil,POWER_SOCKET);

			//Check if joined and take control of the led
			if (joined==1) {
				if (mOutput(POWER_SOCKET)==1) {
					digitalWrite(PIN_LED,HIGH);
				} else {
					digitalWrite(PIN_LED,LOW);
				}
			}
        } 
        
		FAST_90ms() { 
			//Apply logic if statuses changed
			Logic_SimpleLight(POWER_SOCKET);
		}

		FAST_510ms() {
			//Check if joined to gateway
			check_if_joined();
			Serial.print("JOIN STATUS=");
			Serial.println(joined);
		}
		
		FAST_1110ms() {
			#ifdef USE_DHT
				Souliss_Logic_T52(memory_map, DHT_TEMP, DEADBANDLOW, &data_changed);
				Souliss_Logic_T53(memory_map, DHT_HUMI, DEADBANDLOW, &data_changed);
			#endif
		}

        FAST_PeerComms();                                        
    }

	EXECUTESLOW() {
		UPDATESLOW();

		SLOW_10s() {
			#ifdef USE_DHT
				DHTRead();
			#endif
		}
	}
	OTA_Process();
} 

//This routine check for peer is joined to Souliss Network
//If not blink the led every 500ms, else led is a mirror of relè status
void check_if_joined() {
	if(JoinInProgress() && joined==0){
		joined=0;
		if (led_status==0){
			digitalWrite(PIN_LED,HIGH);
			led_status=1;
		}else{
			digitalWrite(PIN_LED,LOW);
			led_status=0;
		}
	}else{
		joined=1;
	}		
}

#ifdef USE_DHT
void DHTRead() {
		
		float temperature = dht.readTemperature();
		Souliss_ImportAnalog(memory_map, DHT_TEMP, &temperature);

		float humidity = dht.readHumidity();
		Souliss_ImportAnalog(memory_map, DHT_HUMI, &humidity);
}
#endif