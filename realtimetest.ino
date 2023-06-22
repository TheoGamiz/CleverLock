#include <Arduino.h>
#if defined(ESP32)
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif
#include <Firebase_ESP_Client.h>

//Provide the token generation process info.
#include "addons/TokenHelper.h"
//Provide the RTDB payload printing info and other helper functions.
#include "addons/RTDBHelper.h"

// Insert your network credentials
#define WIFI_SSID "Bbox-7A11FB53"
#define WIFI_PASSWORD "aKWsa2y61r6ED7VJ2d"

// Insert Firebase project API Key
#define API_KEY "AIzaSyCUWxKi6okHafUKpHNj-vuYie_BS7e_uwM"

// Insert RTDB URLefine the RTDB URL */
#define DATABASE_URL "https://projetserrure-86439-default-rtdb.europe-west1.firebasedatabase.app/" 

//Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

unsigned long sendDataPrevMillis = 0;
int intValue;
int ledValue;
float floatValue;
bool signupOK = false;

void setup() {
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  //digitalWrite(14, LOW);
  Serial.begin(115200);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

  /* Assign the api key (required) */
  config.api_key = API_KEY;

  /* Assign the RTDB URL (required) */
  config.database_url = DATABASE_URL;

  /* Sign up */
  if (Firebase.signUp(&config, &auth, "", "")) {
    Serial.println("ok");
    signupOK = true;
  }
  else {
    Serial.printf("%s\n", config.signer.signupError.message.c_str());
  }

  /* Assign the callback function for the long running token generation task */
  config.token_status_callback = tokenStatusCallback; //see addons/TokenHelper.h

  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

void loop() {
  if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 500 || sendDataPrevMillis == 0)) {
    sendDataPrevMillis = millis();
  
    if (Firebase.RTDB.getInt(&fbdo, "/test/led")) {
      if (fbdo.dataType() == "int") {
        ledValue = fbdo.intData();
        if(ledValue == 1){
          digitalWrite(13, HIGH);
          digitalWrite(14, HIGH);

        }
        else {
          digitalWrite(13, LOW);
          digitalWrite(14, LOW);
        }
        Serial.println(ledValue);
      }
    }

    else {
      Serial.println(fbdo.errorReason());
    }
  }
}