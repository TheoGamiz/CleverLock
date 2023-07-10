/* 1. Define the WiFi credentials */
//#define WIFI_SSID "Bbox-7A11FB53"
//#define WIFI_PASSWORD "aKWsa2y61r6ED7VJ2d"

#include <Adafruit_Fingerprint.h>

#if(defined(__AVR__) || defined(ESP8266)) && !defined(__AVR_ATmega2560__)
SoftwareSerial mySerial(4, 5);
#else
#define mySerial Serial1
#endif

Adafruit_Fingerprint finger = Adafruit_Fingerprint( & mySerial);

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
#define WIFI_SSID "HUAWEI_E5576_346C"
#define WIFI_PASSWORD "0YFTJ29F03Q"

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
uint8_t id;

#if defined(ARDUINO_RASPBERRY_PI_PICO_W)
WiFiMulti multi;
#endif

void setup() {

    Serial.begin(115200);
    pinMode(locker, OUTPUT);

    finger.begin(57600);
    delay(5);
    if (finger.verifyPassword()) {
        Serial.println("Found fingerprint sensor!");
    } else {
        Serial.println("Did not find fingerprint sensor :(");
        while (1) {
            delay(1);
        }
    }

    #if defined(ARDUINO_RASPBERRY_PI_PICO_W)
    multi.addAP(WIFI_SSID, WIFI_PASSWORD);
    multi.run();
    #else
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    #endif

    Serial.print("Connecting to Wi-Fi");
    unsigned long ms = millis();
    while (WiFi.status() != WL_CONNECTED) {
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
    fbdo.setBSSLBufferSize(2048 /* Rx buffer size in bytes from 512 - 16384 */ , 2048 /* Tx buffer size in bytes from 512 - 16384 */ );
    #endif

    // Limit the size of response payload to be collected in FirebaseData
    fbdo.setResponseSize(2048);

    Firebase.begin( & config, & auth);

    Firebase.reconnectWiFi(true);

    // You can use TCP KeepAlive in FirebaseData object and tracking the server connection status, please read this for detail.
    // https://github.com/mobizt/Firebase-ESP-Client#about-firebasedata-object
    // fbdo.keepAlive(5, 5, 1);
}

void loop() {

    // Firebase.ready() should be called repeatedly to handle authentication tasks.

    if (Firebase.ready() && (millis() - dataMillis > 2000 || dataMillis == 0)) {

        uint8_t p = finger.getImage();
        if (p == FINGERPRINT_NOFINGER) goto NoFinger;
        else if (p == FINGERPRINT_OK) {
            p = finger.image2Tz();
            if (p != FINGERPRINT_OK) goto NoMatch;
            p = finger.fingerFastSearch();
            if (p != FINGERPRINT_OK) goto NoMatch;
            Serial.print("Welcome! Your ID is ");
            Serial.print(finger.fingerID);
            Serial.println(". You are granted access.");
            digitalWrite(locker, HIGH);
            delay(3000);
            digitalWrite(locker, LOW);
            return;
        }
        NoMatch: {
            Serial.println("Access  Denied");

            delay(1500);
            return;
        }
        NoFinger: {
            Serial.println("No finger detected");

        }

        dataMillis = millis();

        if (!taskCompleted) {
            taskCompleted = true;

            /*content.set("fields/Japan/mapValue/fields/population/integerValue", "125570000");

             content.set("fields/Belgium/mapValue/fields/time_zone/integerValue", "1");
             content.set("fields/Belgium/mapValue/fields/population/integerValue", "11492641");

             content.set("fields/Singapore/mapValue/fields/time_zone/integerValue", "8");
             content.set("fields/Singapore/mapValue/fields/population/integerValue", "5703600");

             // info is the collection id, countries is the document id in collection info.
            */
        }

        String documentPath = "lock/XhCV3TOYYNEt8aR3jmzj";
        String mask = "lock";

        if (Firebase.Firestore.getDocument( & fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), mask.c_str())) {
            //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());

            // Parse JSON payload
            DynamicJsonDocument jsonDoc(1024); // Adjust the size if necessary
            DeserializationError error = deserializeJson(jsonDoc, fbdo.payload().c_str());

            // Check for parsing errors
            if (error) {
                Serial.print("JSON parsing error: ");
                Serial.println(error.c_str());
                return;
            }

            // Extract the population value
            bool lock = jsonDoc["fields"]["lock"]["booleanValue"].as < bool > ();
            if (lock == 1) {
                digitalWrite(locker, HIGH);

            } else {
                digitalWrite(locker, LOW);

            }
            Serial.println(lock);

        } else {
            Serial.println(fbdo.errorReason());
        }

        documentPath = "lock/XhCV3TOYYNEt8aR3jmzj";
        mask = "newFinger";

        if (Firebase.Firestore.getDocument( & fbdo, FIREBASE_PROJECT_ID, "", documentPath.c_str(), mask.c_str())) {
            //Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());

            // Parse JSON payload
            DynamicJsonDocument jsonDoc(1024); // Adjust the size if necessary
            DeserializationError error = deserializeJson(jsonDoc, fbdo.payload().c_str());

            // Check for parsing errors
            if (error) {
                Serial.print("JSON parsing error: ");
                Serial.println(error.c_str());
                return;
            }

            // Extract the population value
            bool newFinger = jsonDoc["fields"]["newFinger"]["booleanValue"].as < bool > ();
            if (newFinger == 1) {
                //digitalWrite(locker, HIGH);
                String documentPath = "lock/XhCV3TOYYNEt8aR3jmzj";
                FirebaseJson content;

                // For the usage of FirebaseJson, see examples/FirebaseJson/BasicUsage/Create.ino
                content.set("fields/newFinger/booleanValue", 0);

                Serial.print("Update a document... ");

                /** if updateMask contains the field name that exists in the remote document and
                 * this field name does not exist in the document (content), that field will be deleted from remote document
                 */

                if (Firebase.Firestore.patchDocument( & fbdo, FIREBASE_PROJECT_ID, "" /* databaseId can be (default) or empty */ , documentPath.c_str(), content.raw(), "newFinger" /* updateMask */ ))
                    Serial.printf("ok\n%s\n\n", fbdo.payload().c_str());
                else
                    Serial.println(fbdo.errorReason());
                    DynamicJsonDocument jsonDoc(1024); // Adjust the size if necessary
                DeserializationError error = deserializeJson(jsonDoc, fbdo.payload().c_str());
                int lockId = jsonDoc["fields"]["lock_id"]["integerValue"].as < int > ();
                Serial.println("Ready to enroll a fingerprint!");
                Serial.println("Please type in the ID # (from 1 to 127) you want to save this finger as... and the lock id is " + lockId);
                Serial.println(lockId);
                id = lockId;
                if (id == 0) { // ID #0 not allowed, try again!
                    return;
                }
                Serial.print("Enrolling ID #");
                Serial.println(id);

                while (!getFingerprintEnroll());

            } else {
                //digitalWrite(locker, LOW);

            }
            Serial.println(newFinger);

        } else {
            Serial.println(fbdo.errorReason());
        }

    }
}

// NewFinger
uint8_t readnumber(void) {
    uint8_t num = 0;

    while (num == 0) {
        while (!Serial.available());
        num = Serial.parseInt();
    }
    return num;
}

uint8_t getFingerprintEnroll() {

    int p = -1;
    Serial.print("Waiting for valid finger to enroll as #");
    Serial.println(id);
    while (p != FINGERPRINT_OK) {
        p = finger.getImage();
        switch (p) {
        case FINGERPRINT_OK:
            Serial.println("Image taken");
            break;
        case FINGERPRINT_NOFINGER:
            Serial.println(".");
            break;
        case FINGERPRINT_PACKETRECIEVEERR:
            Serial.println("Communication error");
            break;
        case FINGERPRINT_IMAGEFAIL:
            Serial.println("Imaging error");
            break;
        default:
            Serial.println("Unknown error");
            break;
        }
    }

    // OK success!

    p = finger.image2Tz(1);
    switch (p) {
    case FINGERPRINT_OK:
        Serial.println("Image converted");
        break;
    case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
        return p;
    case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return p;
    case FINGERPRINT_FEATUREFAIL:
        Serial.println("Could not find fingerprint features");
        return p;
    case FINGERPRINT_INVALIDIMAGE:
        Serial.println("Could not find fingerprint features");
        return p;
    default:
        Serial.println("Unknown error");
        return p;
    }

    Serial.println("Remove finger");
    delay(2000);
    p = 0;
    while (p != FINGERPRINT_NOFINGER) {
        p = finger.getImage();
    }
    Serial.print("ID ");
    Serial.println(id);
    p = -1;
    Serial.println("Place same finger again");
    while (p != FINGERPRINT_OK) {
        p = finger.getImage();
        switch (p) {
        case FINGERPRINT_OK:
            Serial.println("Image taken");
            break;
        case FINGERPRINT_NOFINGER:
            Serial.print(".");
            break;
        case FINGERPRINT_PACKETRECIEVEERR:
            Serial.println("Communication error");
            break;
        case FINGERPRINT_IMAGEFAIL:
            Serial.println("Imaging error");
            break;
        default:
            Serial.println("Unknown error");
            break;
        }
    }

    // OK success!

    p = finger.image2Tz(2);
    switch (p) {
    case FINGERPRINT_OK:
        Serial.println("Image converted");
        break;
    case FINGERPRINT_IMAGEMESS:
        Serial.println("Image too messy");
        return p;
    case FINGERPRINT_PACKETRECIEVEERR:
        Serial.println("Communication error");
        return p;
    case FINGERPRINT_FEATUREFAIL:
        Serial.println("Could not find fingerprint features");
        return p;
    case FINGERPRINT_INVALIDIMAGE:
        Serial.println("Could not find fingerprint features");
        return p;
    default:
        Serial.println("Unknown error");
        return p;
    }

    // OK converted!
    Serial.print("Creating model for #");
    Serial.println(id);

    p = finger.createModel();
    if (p == FINGERPRINT_OK) {
        Serial.println("Prints matched!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        Serial.println("Communication error");
        return p;
    } else if (p == FINGERPRINT_ENROLLMISMATCH) {
        Serial.println("Fingerprints did not match");
        return p;
    } else {
        Serial.println("Unknown error");
        return p;
    }

    Serial.print("ID ");
    Serial.println(id);
    p = finger.storeModel(id);
    if (p == FINGERPRINT_OK) {
        Serial.println("Stored!");
    } else if (p == FINGERPRINT_PACKETRECIEVEERR) {
        Serial.println("Communication error");
        return p;
    } else if (p == FINGERPRINT_BADLOCATION) {
        Serial.println("Could not store in that location");
        return p;
    } else if (p == FINGERPRINT_FLASHERR) {
        Serial.println("Error writing to flash");
        return p;
    } else {
        Serial.println("Unknown error");
        return p;
    }

    return true;
}