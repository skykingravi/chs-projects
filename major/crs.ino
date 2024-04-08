// libraries
#include <ESP8266WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <--- MODEL HEADER FILE LOCATION ---->

// helper functions
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

// global variables
#define WIFI_SSID "<--- WIFI NAME --->"
#define WIFI_PASSWORD "<--- WIFI PASSWORD --->"
#define API_KEY "<--- FIREBASE API KEY --->"
#define DATABASE_URL "<--- FIREBASE DATABASE URL --->"

#define DHTPIN D2
#define DHTTYPE DHT11
#define SOIL_MOISTURE_PIN A0

DHT dht(DHTPIN, DHTTYPE);

unsigned long sendDataPrevMillis = 0;
bool signupOK = false;
bool predicting = false;

// firebase objects
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// labels
const String LABELS[22] = {
    "rice",
    "maize",
    "chickpea",
    "kidneybeans",
    "pigeonpeas",
    "mothbeans",
    "mungbean",
    "blackgram",
    "lentil",
    "pomegranate",
    "banana",
    "mango",
    "grapes",
    "watermelon",
    "muskmelon",
    "apple",
    "orange",
    "papaya",
    "coconut",
    "cotton",
    "jute",
    "coffee"};

// softmax function (e^x-xm/sum(e^x-xm))
void softmax(float *x)
{
    float max_val = x[0];
    for (int i = 1; i < 22; i++)
    {
        if (x[i] > max_val)
        {
            max_val = x[i];
        }
    }
    float exp_sum = 0.0;
    for (int i = 0; i < 22; i++)
    {
        exp_sum += exp(x[i] - max_val);
    }
    for (int i = 0; i < 22; i++)
    {
        x[i] = exp(x[i] - max_val) / exp_sum;
    }
}

// prediction function
String prediction(float inp[])
{
    // normalization
    for (int i = 0; i < 7; i++)
    {
        inp[i] = (inp[i] - M[i]) / S[i];
    }

    // 7x18 hidden layer
    float o0[18];
    for (int i = 0; i < 18; i++)
    {
        o0[i] = b0[i];
        for (int j = 0; j < 7; j++)
        {
            o0[i] += w0[i + j * 18] * inp[j];
        }
        if (o0[i] < 0)
            o0[i] = 0;
    }

    // 18x9 hidden layer
    float o1[9];
    for (int i = 0; i < 9; i++)
    {
        o1[i] = b1[i];
        for (int j = 0; j < 18; j++)
        {
            o1[i] += w1[i + j * 9] * o0[j];
        }
        if (o1[i] < 0)
            o1[i] = 0;
    }

    // 9x22 output layer
    float o2[22];
    for (int i = 0; i < 22; i++)
    {
        o2[i] = b2[i];
        for (int j = 0; j < 9; j++)
        {
            o2[i] += w2[i + j * 22] * o1[j];
        }
    }
    softmax(o2);

    int maxi = 0;
    for (int i = 1; i < 22; i++)
    {
        if (o2[i] > o2[maxi])
        {
            maxi = i;
        }
    }
    return LABELS[maxi];
}

void setup()
{
    Serial.begin(115200);
    dht.begin();

    // connect to WiFi
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("Connecting to Wi-Fi");
    while (WiFi.status() != WL_CONNECTED)
    {
        Serial.print(".");
        delay(300);
    }
    Serial.println();
    Serial.print("Connected with IP: ");
    Serial.println(WiFi.localIP());
    Serial.println();

    // signup to firebase
    config.api_key = API_KEY;
    config.database_url = DATABASE_URL;
    if (Firebase.signUp(&config, &auth, "", ""))
    {
        Serial.println("Successfully Signed Up to the Firebase.");
        signupOK = true;
    }
    else
    {
        Serial.printf("%s\n", config.signer.signupError.message.c_str());
    }

    // callback function for token generation
    config.token_status_callback = tokenStatusCallback;

    // start firebase
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
}

void loop()
{
    // run each second only if signed up to firebase and is ready
    if (Firebase.ready() && signupOK && (millis() - sendDataPrevMillis > 1000 || sendDataPrevMillis == 0))
    {
        sendDataPrevMillis = millis();

        // check the predicting variable
        if (Firebase.RTDB.getBool(&fbdo, "predicting"))
        {
            if (fbdo.dataType() == "boolean")
            {
                predicting = fbdo.boolData();

                // if it is true then do the prediction
                if (predicting)
                {
                    Serial.println("Predicting...");

                    // fetch sensor data
                    float temperature = dht.readTemperature();
                    float humidity = dht.readHumidity();
                    float moisture = 100.0 - ((analogRead(SOIL_MOISTURE_PIN) / 1024.0) * 100.0);

                    // N,P,K,temperature,humidity,ph,moisture
                    float inp[7] = {0, 0, 0, temperature, humidity, 0, moisture};

                    // fetch parameters from the firebase
                    Firebase.RTDB.getFloat(&fbdo, "parameters/N");
                    Serial.print("N: ");
                    Serial.println(fbdo.floatData());
                    inp[0] = fbdo.floatData();

                    Firebase.RTDB.getFloat(&fbdo, "parameters/P");
                    Serial.print("P: ");
                    Serial.println(fbdo.floatData());
                    inp[1] = fbdo.floatData();

                    Firebase.RTDB.getFloat(&fbdo, "parameters/K");
                    Serial.print("K: ");
                    Serial.println(fbdo.floatData());
                    inp[2] = fbdo.floatData();

                    Serial.print("temperature: ");
                    Serial.println(temperature);

                    Serial.print("humidity: ");
                    Serial.println(humidity);

                    Firebase.RTDB.getFloat(&fbdo, "parameters/ph");
                    Serial.print("ph: ");
                    Serial.println(fbdo.floatData());
                    inp[5] = fbdo.floatData();

                    Serial.print("moisture: ");
                    Serial.println(moisture);

                    String result = prediction(inp);
                    Serial.print("Predicted Crop: ");
                    Serial.println(result);

                    // save the result and values to firebase
                    Firebase.RTDB.setString(&fbdo, "prediction/crop", result);
                    Firebase.RTDB.setFloat(&fbdo, "parameters/temperature", temperature);
                    Firebase.RTDB.setFloat(&fbdo, "parameters/humidity", humidity);
                    Firebase.RTDB.setFloat(&fbdo, "parameters/moisture", moisture);

                    predicting = false;
                    Firebase.RTDB.setBool(&fbdo, "predicting", predicting);
                }
            }
        }
        else
        {
            Serial.println("Failed to get data from Firebase.");
            Serial.println("Reason: " + fbdo.errorReason());
        }
    }
}
