#include <ArduinoJson.h>
#include <Arduino.h>
#if defined(ESP32) || defined(ARDUINO_RASPBERRY_PI_PICO_W)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <Firebase_ESP_Client.h>

// Provide the token generation process info.
#include <addons/TokenHelper.h>

/* 1. Define the WiFi credentials */
#define WIFI_SSID "Bbox-7A11FB53"
#define WIFI_PASSWORD "aKWsa2y61r6ED7VJ2d"

/* 2. Define the API Key */
#define API_KEY "AIzaSyC8evfxnTsV25SvaeAMr6T0rcoAzHJOjZk"

/* 3. Define the project ID */
#define FIREBASE_PROJECT_ID "smart-lock-4ee3f"

/* 4. Define the user Email and password that alreadey registerd or added in your project */
#define USER_EMAIL "gamiz.theo@gmail.com"
#define USER_PASSWORD "test123"

// Define Firebase Data object
FirebaseData fbdo;

FirebaseAuth auth;
FirebaseConfig config;

bool taskCompleted = false;
int locker = 14;

unsigned long dataMillis = 0;

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

void setup()
{

    Serial.begin(115200);
    pinMode(locker, OUTPUT);

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    multi.addAP(WIFI_SSID, WIFI_PASSWORD);
    multi.run();
#else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
#endif

    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
        if (millis() - ms > 10000)
            break;
#endif
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

    /* Assign the api key (required) */
    config.api_key = API_KEY;

    /* Assign the user sign in credentials */
    auth.user.email = USER_EMAIL;
    auth.user.password = USER_PASSWORD;

    // The WiFi credentials are required for Pico W
    // due to it does not have reconnect feature.
#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    config.wifi.clearAP();
    config.wifi.addAP(WIFI_SSID, WIFI_PASSWORD);
#endif

    /* Assign the callback function for the long running token generation task */
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h

#if defined(ESP8266)
    // In ESP8266 required for BearSSL rx/tx buffer for large data handle, increase Rx size as needed.
    fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */, 2048 /* Tx buffer size in bytes from 512 - 16384 */);
#endif

    // Limit the size of response payload to be collected in FirebaseData
    fbdo.setResponseSize(2048);

    Firebase.begin(&config, &auth);

    Firebase.reconnectWiFi(true);

    // You can use TCP KeepAlive in FirebaseData object and tracking the server connection status, please read this for detail.
    // https://github.com/mobizt/Firebase-ESP-Client#about-firebasedata-object
    // fbdo.keepAlive(5, 5, 1);
}

void loop()
{

    // Firebase.ready() should be called repeatedly to handle authentication tasks.

    if (Firebase.ready() && (millis() - dataMillis > 200 || dataMillis == 0))
    {
        dataMillis = millis();

        if (!taskCompleted)
        {
            taskCompleted = true;

            // For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create.ino
            FirebaseJson content;

           /* content.set("fields/Japan/mapValue/fields/time_zone/integerValue", "9");
            content.set("fields/Japan/mapValue/fields/population/integerValue", "125570000");

            content.set("fields/Belgium/mapValue/fields/time_zone/integerValue", "1");
            content.set("fields/Belgium/mapValue/fields/population/integerValue", "11492641");

            content.set("fields/Singapore/mapValue/fields/time_zone/integerValue", "8");
            content.set("fields/Singapore/mapValue/fields/population/integerValue", "5703600");

            // info is the collection id, countries is the document id in collection info.
            String documentPath = "info/countries";

            Serial.print("Create document... ");

            if (Firebase.Firestore.createDocument(&fbdo, FIREBASE_PROJECT_ID, "" , documentPath.c_str(), content.raw()))
                Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
            else
                Serial.println(fbdo.errorReason());*/
        }

        String documentPath = "lock/XhCV3TOYYNEt8aR3jmzj";
        String mask = "lock";
       

        if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), mask.c_str())) {
          //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());

          // Parse JSON payload
          DynamicJsonDocument jsonDoc(1024);  // Adjust the size if necessary
          DeserializationError error = deserializeJson(jsonDoc, fbdo.payload().c_str());

          // Check for parsing errors
          if (error) {
            Serial.print("JSON parsing error: ");
            Serial.println(error.c_str());
            return;
          }

          // Extract the population value
          bool lock = jsonDoc["fields"]["lock"]["booleanValue"].as<bool>();
          if (lock == 1){
            digitalWrite(locker, HIGH);

          }
          else {
            digitalWrite(locker, LOW);

          }
          Serial.println(lock);
        }
        else {
          Serial.println(fbdo.errorReason());
        }
      }
}
