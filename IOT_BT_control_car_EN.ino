/**************************************************************
    - Original motor/light/horn + WS2812b rear light 
    - Ultrasonic distance measurement (trig=12, echo=13)
    - When distance <= 10cm or Bluetooth horn command = ON (horn==true),  buzzer sounds
      Otherwise turn off the buzzer
    - BH1750 brightness sensor monitoring, when lux < 5, turn WS2812b
      And upload brightness data to ThingSpeak
**************************************************************/

#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <BH1750.h>
#include <WiFiS3.h>               // Wi-Fi library for UNO R4 WiFi
#include <ArduinoHttpClient.h>    // Official Arduino HTTPClient library

#ifdef __AVR__
#include <avr/power.h> // For 8MHz AVR boards like Trinket
#endif

/************ 1. Define ultrasonic & global variables ************/
#define trigPin 12    
#define echoPin 13    
long duration;               
long distance;              
long targetDistance = 10;   //trig 10cm

// Timing variables
unsigned long lastDistanceCheck = 0; // Last measurement time
const unsigned long distanceInterval = 1000; // 5 seconds

/************ 2. Custom Strip class & Loop structure (WS2812b) ************/
class Strip {
public:
  uint8_t   effect;
  uint8_t   effects;
  uint16_t  effStep;
  unsigned long effStart;
  Adafruit_NeoPixel strip;

  Strip(uint16_t leds, uint8_t pin, uint8_t toteeffects, uint16_t striptype)
    : strip(leds, pin, striptype)
  {
    effect  = -1;
    effects = toteeffects;
    Reset();
  }

  void Reset() {
    effStep  = 0;
    effect   = (effect + 1) % effects;
    effStart = millis();
  }
};

struct Loop {
  uint8_t currentChild;
  uint8_t childs;
  bool timeBased;
  uint16_t cycles;
  uint16_t currentTime;

  Loop(uint8_t totchilds, bool timebased, uint16_t tottime) {
    currentChild = 0;
    childs       = totchilds;
    timeBased    = timebased;
    cycles       = tottime;
    currentTime  = 0;
  }
};

/************ 3. Create Strip/Loop instances (WS2812b) ************/
Strip strip_0(60, A2, 60, NEO_GRB + NEO_KHZ800);
Loop  strip0loop0(1, false, 1);

/************ 4. Original motor/light/horn pins and variables ************/
#define in1  5
#define in2  6
#define in3  10
#define in4  11

#define horn_Buzz A1   // Buzzer 
#define light1  A2   // WS2812b LED  

//for A4, A5 to use I2C


#define light2  A0   
#define light3  A3  
#define light4  9 

int command;           // Bluetooth command
int Speed = 204;       // 0~255
int Speedsec;
int buttonState = 0;
int lastButtonState = 0;
int Turnradius = 0;
int brakeTime = 45;
int brkonoff = 1;      // 1 = electronic brake, 0 = normal
boolean lightFront = false;
boolean lightBack  = false;  // WS2812b rear light switch
boolean horn       = false;  // Bluetooth horn command switch

/************ 5. IoT global variables ************/
const char ssid[] = "123456";         // Wi-Fi SSID
const char pass[] = "L12345678";      // Wi-Fi password

const char server[] = "api.thingspeak.com"; // ThingSpeak server
const int port = 80;                         // HTTP port (443 for HTTPS)
const char writeAPIKey[] = "BITN2UNHBRXC8G1M"; // ThingSpeak Write API Key

// Initialize Wi-Fi client
WiFiClient wifiClient;

// Initialize HTTP client
HttpClient client = HttpClient(wifiClient, server, port);

// Initialize BH1750 brightness sensor
BH1750 lightMeter;

// Brightness upload timing variables
unsigned long lastUploadTime = 0;
const unsigned long uploadInterval = 20000; // 20 seconds

// BH1750 reading timing variables
unsigned long lastLightCheck = 0;
const unsigned long lightCheckInterval = 2000; // 2 seconds

float lux = 0.0;

/************ 6. setup() ************/
void setup() {
#if defined(__AVR_ATtiny85__) && (F_CPU == 8000000)
  clock_prescale_set(clock_div_1);
#endif

  // Motor pins
  pinMode(in1, OUTPUT);
  pinMode(in2, OUTPUT);
  pinMode(in3, OUTPUT);
  pinMode(in4, OUTPUT);

  // Light and buzzer pins
  pinMode(light2, OUTPUT);
  pinMode(light4, OUTPUT);
  pinMode(light1, OUTPUT); // Using WS2812b
  pinMode(light3, OUTPUT);
  pinMode(horn_Buzz, OUTPUT);

  // Ultrasonic pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  // Serial baud rate
  Serial1.begin(9600);

  // Initialize WS2812b
  strip_0.strip.begin();
  strip_0.strip.show(); // Clear the strip

  // Initialize I2C communication
  Wire.begin();

  // Initialize BH1750 sensor
  if (lightMeter.begin()) {
    Serial.println(F("BH1750 sensor initialized successfully."));
  } else {
    Serial.println(F("BH1750 sensor initialization failed."));
    while (1); // Stop if sensor initialization fails
  }

  // Connect to Wi-Fi network
  Serial.print(F("Connecting to Wi-Fi: "));
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("."));
    delay(300); // This is initialization stage, so delay() is acceptable
  }
  Serial.println();
  Serial.println(F("Wi-Fi connected!"));
  Serial.print(F("IP address: "));
  Serial.println(WiFi.localIP());

  Serial.println(F("Setup complete."));
}

/************ 7. loop() ************/
void loop() {
  // 7.1 Check Bluetooth commands
  if (Serial1.available() > 0) {
    command = Serial1.read();
    Stop(); // Stop the motors each time a new command arrives

    // Front lights
    if (lightFront) {
      digitalWrite(light2, HIGH);
      digitalWrite(light4, HIGH);
    } else {
      digitalWrite(light2, LOW);
      digitalWrite(light4, LOW);
    }

    // Rear lights (traditional digitalWrite does not affect WS2812b, but keep original structure)
    if (lightBack) {
      digitalWrite(light1, HIGH);
      digitalWrite(light3, HIGH);
    } else {
      digitalWrite(light1, LOW);
      digitalWrite(light3, LOW);
    }

    // -------------- Parse Bluetooth commands --------------
    switch (command) {
      // Motor direction
      case 'F': forward();       break;
      case 'B': back();          break;
      case 'L': left();          break;
      case 'R': right();         break;
      case 'G': forwardleft();   break;
      case 'I': forwardright();  break;
      case 'H': backleft();      break;
      case 'J': backright();     break;

      // Speed
      case '0': Speed = 100;     break;
      case '1': Speed = 140;     break;
      case '2': Speed = 153;     break;
      case '3': Speed = 165;     break;
      case '4': Speed = 178;     break;
      case '5': Speed = 191;     break;
      case '6': Speed = 204;     break;
      case '7': Speed = 216;     break;
      case '8': Speed = 229;     break;
      case '9': Speed = 242;     break;
      case 'q': Speed = 255;     break;

      // Front light/horn
      case 'W': lightFront = true;  break;
      case 'w': lightFront = false; break;
      case 'V': horn       = true;  break;
      case 'v': horn       = false; break;

      // Use 'U' to toggle rear WS2812b
      case 'U':
        lightBack = !lightBack;  // Toggle on/off
        break;

      // Electronic brake 'S' is handled in brakeOn()
    }

    // Brake logic
    Speedsec = Turnradius;
    if (brkonoff == 1) {
      brakeOn();
    } else {
      brakeOff();
    }
  }

  // 7.2 Play/turn off WS2812b rear light animation
  if (lightBack) {
    if (strip0_loop0() & 0x01) {
      strip_0.strip.show();
    }
  } else {
    strip_0.strip.clear();
    strip_0.strip.show();
  }

  // 7.3 Ultrasonic measurement (every 5 seconds)
  unsigned long currentMillis = millis();
  if (currentMillis - lastDistanceCheck >= distanceInterval) {
    distanceDetect();
    lastDistanceCheck = currentMillis;
  }

  // 7.4 Buzzer logic:
  //     If distance <= threshold or horn (Bluetooth command) is true, buzzer sounds
  //     Otherwise off
  if (distance <= targetDistance || horn) {
    digitalWrite(horn_Buzz, HIGH);
  } else {
    digitalWrite(horn_Buzz, LOW);
  }

  // 7.5 BH1750 brightness detection (every 2 seconds)
  if (currentMillis - lastLightCheck >= lightCheckInterval) {
    lux = lightMeter.readLightLevel();
    if (lux < 0) {
      Serial.println(F("Error reading brightness."));
    } else {
      Serial.print(F("Brightness: "));
      Serial.print(lux);
      Serial.println(F(" lx"));

      // Control WS2812b based on brightness
      if (lux < 5) {
        lightBack = true;
      } else {
        // If not turned on due to brightness, keep original state
        // If you need to force turn off when lux >= 5, uncomment the line below
        // lightBack = false;
      }

      // Update rear light state
      if (lightBack) {
        digitalWrite(light1, HIGH);
        digitalWrite(light3, HIGH);
      } else {
        digitalWrite(light1, LOW);
        digitalWrite(light3, LOW);
      }

      // Upload brightness to ThingSpeak (every 20 seconds)
      if (currentMillis - lastUploadTime >= uploadInterval) {
        uploadToThingSpeak(lux);
        lastUploadTime = currentMillis;
      }
    }
    lastLightCheck = currentMillis;
  }
}

/************ 8. Original motor/brake functions (unchanged) ************/
void forward() {
  analogWrite(in1, Speed);
  analogWrite(in3, Speed);
}
void back() {
  analogWrite(in2, Speed);
  analogWrite(in4, Speed);
}
void left() {
  analogWrite(in3, Speed);
  analogWrite(in2, Speed);
}
void right() {
  analogWrite(in4, Speed);
  analogWrite(in1, Speed);
}
void forwardleft() {
  analogWrite(in1, Speedsec);
  analogWrite(in3, Speed);
}
void forwardright() {
  analogWrite(in1, Speed);
  analogWrite(in3, Speedsec);
}
void backright() {
  analogWrite(in2, Speed);
  analogWrite(in4, Speedsec);
}
void backleft() {
  analogWrite(in2, Speedsec);
  analogWrite(in4, Speed);
}

void Stop() {
  analogWrite(in1, 0);
  analogWrite(in2, 0);
  analogWrite(in3, 0);
  analogWrite(in4, 0);
}

void brakeOn() {
  buttonState = command;
  if (buttonState != lastButtonState) {
    if (buttonState == 'S') {
      if (lastButtonState != buttonState) {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, HIGH);
        digitalWrite(in3, HIGH);
        digitalWrite(in4, HIGH);
        Stop();
      }
    }
    lastButtonState = buttonState;
  }
}
void brakeOff() {
  // Empty function
}

/************ 9. Ultrasonic measurement function ************/
void distanceDetect() {
  // Send trigger pulse
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(4);
  digitalWrite(trigPin, LOW);

  // Read echo pulse length
  duration = pulseIn(echoPin, HIGH, 30000); // Add timeout, 30000us=30ms

  if (duration == 0) { // Timeout, no detection
    distance = 999; // Set a large distance value
  } else {
    distance = (duration / 2) * 0.0343;  // Convert to cm
  }

  // Optional: print to serial
  Serial.print("Detected distance: ");
  Serial.print(distance);
  Serial.println(" cm");
}

/************ 10. WS2812b rainbow animation logic (unchanged) ************/
uint8_t strip0_loop0() {
  uint8_t ret = 0x00;
  switch (strip0loop0.currentChild) {
    case 0:
      ret = strip0_loop0_eff0();
      break;
  }
  if(ret & 0x02) {
    ret &= 0xfd;
    if(strip0loop0.currentChild + 1 >= strip0loop0.childs) {
      strip0loop0.currentChild = 0;
      if(++strip0loop0.currentTime >= strip0loop0.cycles) {
        strip0loop0.currentTime = 0;
        ret |= 0x02;
      }
    } else {
      strip0loop0.currentChild++;
    }
  }
  return ret;
}

uint8_t strip0_loop0_eff0() {
  // Steps=60, Delay=25, toLeft=true
  // Colors: (0.255.255)->(0.0.255)->(255.0.255)

  if(millis() - strip_0.effStart < 25ul * (strip_0.effStep)) return 0x00;

  float factor1, factor2;
  uint16_t ind;
  for(uint16_t j=0; j<60; j++) {
    ind = strip_0.effStep + j * 1; // "toLeft" => use +

    switch((int)((ind % 60) / 20)) {
      case 0:
        factor1 = 1.0 - ((float)(ind % 60 -  0) / 20);
        factor2 = (float)((int)(ind - 0) % 60) / 20;
        strip_0.strip.setPixelColor(j,
          (uint8_t)(0    * factor1 + 0   * factor2),
          (uint8_t)(255  * factor1 + 0   * factor2),
          (uint8_t)(255  * factor1 + 255 * factor2)
        );
        break;

      case 1:
        factor1 = 1.0 - ((float)(ind % 60 - 20) / 20);
        factor2 = (float)((int)(ind - 20) % 60) / 20;
        strip_0.strip.setPixelColor(j,
          (uint8_t)(0    * factor1 + 255 * factor2),
          (uint8_t)(0    * factor1 + 0   * factor2),
          (uint8_t)(255  * factor1 + 255 * factor2)
        );
        break;

      case 2:
        factor1 = 1.0 - ((float)(ind % 60 - 40) / 20);
        factor2 = (float)((int)(ind - 40) % 60) / 20;
        strip_0.strip.setPixelColor(j,
          (uint8_t)(255  * factor1 + 0   * factor2),
          (uint8_t)(0    * factor1 + 255 * factor2),
          (uint8_t)(255  * factor1 + 255 * factor2)
        );
        break;
    }
  }

  if(strip_0.effStep >= 60) {
    strip_0.Reset();
    return 0x03; 
  } else {
    strip_0.effStep++;
  }
  return 0x01;
}

/************ 11. ThingSpeak upload function ************/
void uploadToThingSpeak(float luxValue) {
  // Prepare ThingSpeak API request URL
  String url = "/update?api_key=";
  url += writeAPIKey;      // Add Write API Key
  url += "&field1=";
  url += String(luxValue); // Add brightness value to field1

  // Send GET request to ThingSpeak
  Serial.print(F("Sending data to ThingSpeak... "));
  client.get(url);

  // Wait for response
  int statusCode = client.responseStatusCode();
  String response = client.responseBody();

  // Print response status code and content
  Serial.print(F("Status code: "));
  Serial.println(statusCode);
  Serial.print(F("Response: "));
  Serial.println(response);

  // Check if data was sent successfully
  if (statusCode == 200) {
    Serial.println(F("Data sent to ThingSpeak successfully."));
  } else {
    Serial.println(F("Failed to send data to ThingSpeak."));
  }

  // Close connection
  client.stop();
}
