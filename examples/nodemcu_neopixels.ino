/*
 Example for the ESP8266 WiFi boards
 ============================================================================
 Author : Stephan Ruloff and Juriaan Gregor 
*/

// Include our libraries
// ----------------------------------------------------------------------------
 
// We have to define our Pin layout for the FASTLED library
// See https://github.com/FastLED/FastLED/wiki/ESP8266-notes
#define FASTLED_ESP8266_NODEMCU_PIN_ORDER

// Include our Artnet libraries
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArtnetnodeWifi.h>

// Include our FASTled library
#include <FastLED.h>

// SETTINGS
// ----------------------------------------------------------------------------

// Define the amount of Leds that we have to actually get started with that :)
#define NUM_LEDS 10

// Define our DATA pin
#define DATA_PIN 6

// WiFi Settings
const char* ssid = "NETWORK_NAME";
const char* password = "NETWORK_PASS";

// Artnet Settings
const int startUniverse = 0;


// Variables
// ----------------------------------------------------------------------------

// Artnet
WiFiUDP UdpSend;
ArtnetnodeWifi artnetnode;
const int numberOfChannels = NUM_LEDS * 3;
const int maxUniverses = numberOfChannels / 512 + ((numberOfChannels % 512) ? 1 : 0);
bool universesReceived[maxUniverses];
bool sendFrame = 1;
int previousDataLength = 0;

// Other 
bool setupDone     = false;

// Our table that has all the neccassary data regarding the leds 
CRGB leds[NUM_LEDS];

boolean connectWifi(void) 
{
    boolean state   = true;
    int i       = 0;

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.println("");
    Serial.println("Connecting to WiFi");
  
    // Wait for connection
    Serial.print("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
      if (i > 20){
        state = false;
          break;
      }
      i++;
    }

    if (state){
      Serial.println("");
      Serial.print("Connected to ");
      Serial.println(ssid);
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    } 
    else {
      Serial.println("");
      Serial.println("Connection failed.");
    }
  
    return state;

}

void ledTest() 
{
  Serial.println("Testing LEDs");

  // Blue
  for (int dot = 0; dot < NUM_LEDS; dot++) {
    leds[dot] = CRGB::Blue;
  }

  FastLED.show();
  delay(500);

  // Green
  for (int dot = 0; dot < NUM_LEDS; dot++) {
    leds[dot] = CRGB::Green;
  }

  FastLED.show();
  delay(500);


  // Red
  for (int dot = 0; dot < NUM_LEDS; dot++) {
    leds[dot] = CRGB::Red;
  }

  FastLED.show();
  delay(500);


  // Black
  for (int dot = 0; dot < NUM_LEDS; dot++) {
    leds[dot] = CRGB::Black;
  }

  FastLED.show();
  delay(500);

}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
{

  if (universe == 15) {
    // Brightness
  }

  // Store which universe has got in
  if ((universe - startUniverse) < maxUniverses)
    universesReceived[universe - startUniverse] = 1;

  for (int i = 0 ; i < maxUniverses ; i++)
  {
    if (universesReceived[i] == 0)
    {
      //Serial.println("Broke");
      sendFrame = 0;
      break;
    }
  }

  // read universe and put into the right part of the display buffer
  for (int i = 0; i < length / 3; i++) {
    
    // Convert our data to a valid led mapping 
    int dot   = i + (universe - startUniverse) * (previousDataLength / 3);

    if (dot <= NUM_LEDS) { 
      // Set it in our table
      leds[dot] = CRGB(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
    }
  }
  
  previousDataLength = length;     
  
  if (sendFrame)
  {
    FastLED.show();
    // Reset universeReceived to 0
    memset(universesReceived, 0, maxUniverses);
  }
}


void setup() {

    // Set our Serial
    Serial.begin(115200);
    
    // Connect to our Wifi
    connectWifi();
  
    // Set our Pixels 
    FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);
    
    // Check or the pixels are ready for usage 
    ledTest();

    // Set our name and desc of our Artnet Node 
    artnetnode.setShortName("Lighting node");
    
    // max. 63 characters
    artnetnode.setLongName("Lighting node based on the NodeMCU and Neopixels");
  
    // Start our Artnet Stream 
    artnetnode.begin();
  
    // Connect our Artnet to our Callback 
    artnetnode.setArtDmxCallback(onDmxFrame);
  
    // Set our Setupdone to true 
    setupDone     = true; 
}

void loop() {
  
    // Read our Artnet Stream 
    artnetnode.read();
}