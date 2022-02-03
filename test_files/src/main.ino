#include <WiFi.h>
#include <OneWire.h>

/* change ssid and password according to yours WiFi*/
const char* ssid     = "OnePlus 7";
const char* password = "PikachuLicorne92";

/*
 * This is the IP address of your PC
 * [Wins: use ipconfig command, Linux: use ifconfig command]
*/
const char* host = "192.168.180.204";
const int port = 8088;

OneWire  ds(4);  // on pin 10 (a 4.7K resistor is necessary)

void setup()
{
    Serial.begin(115200);
    Serial.print("Connecting to ");
    Serial.println(ssid);

    /* connect to your WiFi */
    WiFi.begin(ssid, password);

    /* wait until ESP32 connect to WiFi*/
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
	}
    Serial.println("");
    Serial.println("WiFi connected with IP address: ");
    Serial.println(WiFi.localIP());
}

void loop()
{
	byte i;
	byte present = 0;
	byte type_s;
	byte data[12];
	byte addr[8];
	float celsius;

    /* Use WiFiClient class to create TCP connections */
    WiFiClient client;

    if (!client.connect(host, port)) {
        Serial.println("connection failed");
        return;
    }

	if ( !ds.search(addr)) {
		ds.reset_search();
		delay(1000);
		return;
	}

	ds.reset();
	ds.select(addr);
	ds.write(0x44, 1);        // start conversion, with parasite power on at the end
	delay(1000);     // maybe 750ms is enough, maybe not

	present = ds.reset();
	ds.select(addr);
	ds.write(0xBE);         // Read Scratchpad

	for ( i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = ds.read();
	}

	// Convert the data to actual temperature
	// because the result is a 16 bit signed integer, it should
	// be stored to an "int16_t" type, which is always 16 bits
	// even when compiled on a 32 bit processor.
	int16_t raw = (data[1] << 8) | data[0];
	if (type_s)
	{
		raw = raw << 3; // 9 bit resolution default
		if (data[7] == 0x10) {
		// "count remain" gives full 12 bit resolution
		raw = (raw & 0xFFF0) + 12 - data[6];
		}
	}
	else
	{
		byte cfg = (data[4] & 0x60);
		// at lower res, the low bits are undefined, so let's zero them
		if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
		else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
		//// default is 12 bit resolution, 750 ms conversion time
	}
	celsius = (float)raw / 16.0;
	Serial.println(celsius);

    /* This will send the data to the server */
    client.print(celsius);
    client.stop();
}
