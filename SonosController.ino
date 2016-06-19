//#include <Adafruit_NeoPixel.h>
#include <SonosUPnP.h>
#include <MicroXPath_P.h>
//#include <MicroXPath.h>

#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <DNSServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
MDNSResponder mdns;
#include <ArduinoOTA.h>

#include <WiFiUDP.h>
WiFiUDP Udp;

#define SERIAL_DATA_THRESHOLD_MS 500
#define SERIAL_ERROR_TIMEOUT "E: Serial"
#define ETHERNET_ERROR_DHCP "E: DHCP"
#define ETHERNET_ERROR_CONNECT "E: Connect"

void ethConnectError();
//EthernetClient g_ethClient;
WiFiClient client;
SonosUPnP g_sonos = SonosUPnP(client, ethConnectError);

//IPAddress sonosIP;

void setup()
{
	Serial.begin(115200);
	WiFiManager wifiManager;

	//handle the conncetion; if not successful
	//starts the webserver and asks for wifi AP credentials
	wifiManager.autoConnect();

	Serial.println("connected to WiFi");
	/*if (mdns.begin("esp8266", WiFi.localIP())) {
	Serial.println("MDNS responder started");
	}*/

	//OTA = ESP8266 Over The Air update
	//if it fails try it via serial
	ArduinoOTA.onStart([]() {
		Serial.println("Start");
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("\nEnd");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
		else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
		else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
		else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
		else if (error == OTA_END_ERROR) Serial.println("End Failed");
	});
	ArduinoOTA.begin();
	Serial.println("Ready");
	Serial.print("IP address: ");
	Serial.println(WiFi.localIP());

	//for recover to get a OTA chance after reset

	unsigned long lastcheck = millis();

	while ((millis() - lastcheck) < 5000) {
		delay(100);
		yield();
		ArduinoOTA.handle();
	}

}

void ethConnectError()
{
	Serial.println(ETHERNET_ERROR_CONNECT);
	Serial.println("mir geht es nicht gut");
}



IPAddress discoverSonos(IPAddress &sonosIP){
	
	//needed for the SSDP discover
	Udp.begin(1901); //we should use any other port??
	bool found = false;

		Serial.println("Sending M-SEARCH multicast");
		Udp.beginPacketMulticast(IPAddress(239, 255, 255, 250), 1900, WiFi.localIP());
		Udp.write("M-SEARCH * HTTP/1.1\r\n"
		"HOST: 239.255.255.250:1900\r\n"
		"MAN: \"ssdp:discover\"\r\n"
		"MX: 1\r\n"
		"ST: urn:schemas-upnp-org:device:ZonePlayer:1\r\n"
		"\r\n");
		Udp.endPacket();
		unsigned long lastcheck = millis();
		
		while (!found && ((millis() - lastcheck) < 5000)) {
			int packetSize = Udp.parsePacket();
			if (packetSize) {
				Serial.print("Received packet of size ");
				Serial.println(packetSize);
				Serial.print("From ");
				sonosIP = Udp.remoteIP();
				found = true;
				Serial.print(sonosIP);
				Serial.print(", port ");
				Serial.println(Udp.remotePort());

				Udp.flush();

				
						
				/* debug only

				char packetBuffer[255];
				// read the packet into packetBufffer
				int len = Udp.read(packetBuffer, 255);
				if (len > 0) {
				packetBuffer[len] = 0;
				}

				Serial.println("Contents:");
				Serial.println(packetBuffer);
				*/
			}
			delay(50);
		}

		Udp.stop();
		
			
	return found;
}


void check_Sonos()
{	
	IPAddress sonosIP;
	char uri[256] = "";
	
	if (discoverSonos(sonosIP)) {

		TrackInfo track = g_sonos.getTrackInfo(sonosIP, uri, sizeof(uri));
		Serial.print("Trackinfo: ");
		Serial.println(track.uri);

		//this is tricky as we might start playing without user wish
		if (strlen(track.uri) == 0) {
			//delay(5000);
			g_sonos.playRadio(sonosIP, "//swr-mp3-m-swr3.akacast.akamaistream.net/7/720/137136/v1/gnl.akacast.akamaistream.net/swr-mp3-m-swr3", "SWR3");
		};
	};
}

unsigned long lastcheck = 0;
void loop()
{
	//WiFiClient client;
	
	delay(1000);
	yield();
	ArduinoOTA.handle();

	if ((unsigned long) (millis() - lastcheck) > 5000ul) {
		check_Sonos();
		lastcheck = millis();	
	}

	//ESP.deepSleep(30000000);

}
