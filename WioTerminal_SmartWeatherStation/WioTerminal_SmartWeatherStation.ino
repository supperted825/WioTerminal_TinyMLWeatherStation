// Wio Terminal Smart Weather Station
// Author: Jonathan Tan
// Feb 2021
// Written for Seeed Wio Terminal

// Include TensorFlow Libraries
#include "TensorFlowLite.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

// Include Remaining Libraries
#include "DHT.h"
#include "TFT_eSPI.h"

TFT_eSPI tft;

// Our model
#include "model.h"

// DHT Sensor Variables
#define DHTTYPE DHT11
#define DHTPIN PIN_WIRE_SCL
#define DEBUG Serial
DHT dht(DHTPIN, DHTTYPE);

// TF Model Outputs
const char* OUTPUTS[] = {
    "No Rain",
    "Might Rain",
    "Rain"
};
int NUM_OUTPUTS = 3;
int array_count = 0;
float probability, reading[2];
float temp_hum_val[14];
long displayMarker, dataMarker;
String prediction; 

// Tensor Flow Variables

tflite::MicroErrorReporter tflErrorReporter;
tflite::AllOpsResolver tflOpsResolver;
const tflite::Model* tflModel = nullptr;
tflite::MicroInterpreter* tflInterpreter = nullptr;
TfLiteTensor* tflInputTensor = nullptr;
TfLiteTensor* tflOutputTensor = nullptr;

// Create an area of memory to use for input, output, and other TensorFlow
// arrays. You'll need to adjust this by combiling, running, and looking for errors.

constexpr int tensorArenaSize = 8 * 1024;
byte tensorArena[tensorArenaSize];

void millisDelay(int duration) {
  long mark = millis();
  while (millis() - mark < duration) {}
}

void setup() {
    // Wait for Serial to connect
    Serial.begin(115200);

    // Initialise LCD
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);
    
    /// Get the TFL representation of the model byte array
    tflModel = tflite::GetModel(model);
    if (tflModel->version() != TFLITE_SCHEMA_VERSION) {
      Serial.println(F("Model schema mismatch!"));
      while (1);
    }
    
    // Create an interpreter to run the model
    tflInterpreter = new tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);
    
    // Allocate memory for the model's input and output tensors
    tflInterpreter->AllocateTensors();
    
    // Get pointers for the model's input and output tensors
    tflInputTensor = tflInterpreter->input(0);
    tflOutputTensor = tflInterpreter->output(0);

    dht.begin();
    dataMarker = millis();
    displayMarker = millis();
    tft.setTextDatum(TC_DATUM);
    tft.setTextSize(2);
    tft.drawString(F("Smart Weather Station"), 160, 20);
    tft.drawFastVLine(160,80,60, TFT_WHITE);
}

void loop() {

    if (array_count == 0 | millis() - displayMarker > 2000) {
      displayMarker = millis();
      //float reading[2];
      if (!dht.readTempAndHumidity(reading)) {
        tft.setTextPadding(0);
        tft.setTextSize(2);
        tft.drawString(F("Temp (C)"), 80, 140);
        tft.drawString(F("R.H."), 240, 140);
        tft.setTextSize(4);
        tft.drawString(String(round(reading[1]*10)/10), 80, 90);
        tft.drawString(String(round(reading[0])), 240, 90);
      } else {
          //tft.drawString(F("Failed to get temprature and humidity value."),0,0);
      }
    }
 
    if (array_count == 0 | millis() - dataMarker > 4000) {
      dataMarker = millis();
      tft.setTextSize(2);
      tft.setTextPadding(320);

      for (int i=0; i<14; i++) {
        temp_hum_val[i] = temp_hum_val[i+2];
      }

      temp_hum_val[12] = reading[1] + 273.15;
      temp_hum_val[13] = reading[0];
      array_count ++;
      if (array_count > 7) array_count = 7;
      
      if (array_count == 7) {
        
        // Copy array into tensor inputs
        for (int i=0; i<14; i++) {
          tflInputTensor->data.f[i] = temp_hum_val[i];
          Serial.print(String(temp_hum_val[i]) + " ");
        }
        
        // Run inference on data
        TfLiteStatus invoke_status = tflInterpreter->Invoke();
        if (invoke_status != kTfLiteOk) {
            Serial.println(F("Invoke Failed!"));
            while(1);
            return;
        }
        
        // Read predicted y value from output, get max probability
        
        probability = 0;
        for (int i = 0; i < NUM_OUTPUTS; i++) {
            if (tflOutputTensor->data.f[i] > probability) {
              prediction = OUTPUTS[i];
              probability = tflOutputTensor->data.f[i];
            }
        }
        
        tft.drawString(prediction + " (" + String(probability) + ")", 160 ,200);
      } else {
        tft.drawString("Need " + String(7-array_count) + " more readings.", 160, 200);
      }
    }
}
