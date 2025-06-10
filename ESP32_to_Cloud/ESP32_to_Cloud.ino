#include <Magic_Wand_inferencing.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <Adafruit_NeoPixel.h>
#include <math.h> // for sin()
#include <HTTPClient.h>
#include <ArduinoJson.h>

// === NeoPixel ===
#define NEOPIXEL_PIN D3       // GPIO4
#define NUM_PIXELS 1

Adafruit_NeoPixel pixels(NUM_PIXELS, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);

// === Button ===
#define BUTTON_PIN D2       // GPIO20 (D7)

// === Constants ===
#define FADE_STEPS 50
#define FADE_DELAY 20  // ms per step

// Add these definitions
#define CONFIDENCE_THRESHOLD 95.0
const char* serverUrl = "http://10.19.66.79:5001/";

Adafruit_MPU6050 mpu;

// Sampling
#define SAMPLE_RATE_MS 10
#define CAPTURE_DURATION_MS 1000
#define FEATURE_SIZE EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE

bool capturing = false;
unsigned long last_sample_time = 0;
unsigned long capture_start_time = 0;
int sample_count = 0;
float features[FEATURE_SIZE];

int raw_feature_get_data(size_t offset, size_t length, float *out_ptr) {
    memcpy(out_ptr, features + offset, length * sizeof(float));
    return 0;
}

void setup() {
    Serial.begin(115200);

    // NeoPixel setup
    pixels.begin();
    pixels.clear();
    pixels.show();

    // Button
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // MPU6050
    Serial.println("Initializing MPU6050...");
    if (!mpu.begin()) {
        Serial.println("Failed to find MPU6050 chip");
        while (1) delay(10);
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    Serial.println("MPU6050 initialized. Press button to start gesture recognition.");
}

void loop() {
    if (digitalRead(BUTTON_PIN) == LOW && !capturing) {
        Serial.println("Button pressed! Starting gesture capture...");
        sample_count = 0;
        capturing = true;
        capture_start_time = millis();
        last_sample_time = millis();
        delay(200);  // debounce
    }

    if (capturing) {
        capture_accelerometer_data();
    }
}

void capture_accelerometer_data() {
    if (millis() - last_sample_time >= SAMPLE_RATE_MS) {
        last_sample_time = millis();

        sensors_event_t a, g, temp;
        mpu.getEvent(&a, &g, &temp);

        if (sample_count < FEATURE_SIZE / 3) {
            int idx = sample_count * 3;
            features[idx] = a.acceleration.x;
            features[idx + 1] = a.acceleration.y;
            features[idx + 2] = a.acceleration.z;
            sample_count++;
        }

        if (sample_count >= FEATURE_SIZE / 3) {
            capturing = false;
            Serial.println("Capture complete");
            run_inference();
        }
    }
}

void run_inference() {
    if (sample_count * 3 < EI_CLASSIFIER_DSP_INPUT_FRAME_SIZE) {
        Serial.println("ERROR: Not enough data for inference");
        return;
    }

    ei_impulse_result_t result = { 0 };
    signal_t features_signal;
    features_signal.total_length = FEATURE_SIZE;
    features_signal.get_data = &raw_feature_get_data;

    EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false);
    if (res != EI_IMPULSE_OK) {
        Serial.print("Classifier failed: "); Serial.println(res);
        return;
    }

    float max_val = 0;
    int max_idx = -1;
    for (uint16_t i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
        if (result.classification[i].value > max_val) {
            max_val = result.classification[i].value;
            max_idx = i;
        }
    }

    if (max_idx != -1) {
        String gesture = ei_classifier_inferencing_categories[max_idx];
        Serial.print("Prediction: "); Serial.print(gesture);
        Serial.print(" ("); Serial.print(max_val * 100); Serial.println("%)");

        float confidence = max_val * 100;

        if (confidence < CONFIDENCE_THRESHOLD) {
            Serial.println("Low confidence - sending raw data to server...");
            Serial.println("Edge - Offloading Transmitting to cloud - Using cloud compute");
            sendRawDataToServer();
        } else {
            if (gesture == "Z") {
                fadeNeoPixel(255, 0, 0);   // Red
            } else if (gesture == "O") {
                fadeNeoPixel(0, 0, 255);   // Blue
            } else if (gesture == "V") {
                fadeNeoPixel(0, 255, 0);   // Green
            } else {
                pixels.clear(); pixels.show(); // No color
            }
        }
    }
}

// === NeoPixel Fading Function ===
void fadeNeoPixel(int r, int g, int b) {
    for (int i = 0; i <= FADE_STEPS; i++) {
        float bell = sin(PI * i / FADE_STEPS);
        pixels.setPixelColor(0, pixels.Color(r * bell, g * bell, b * bell));
        pixels.show();
        delay(FADE_DELAY);
    }

    delay(200);  // Hold at peak

    for (int i = FADE_STEPS; i >= 0; i--) {
        float bell = sin(PI * i / FADE_STEPS);
        pixels.setPixelColor(0, pixels.Color(r * bell, g * bell, b * bell));
        pixels.show();
        delay(FADE_DELAY);
    }

    pixels.clear();
    pixels.show();
}

void sendRawDataToServer() {
    HTTPClient http;
    http.begin(serverUrl);
    http.addHeader("Content-Type", "application/json");

    // Build JSON array from features[]
    DynamicJsonDocument doc(FEATURE_SIZE * 6); // Adjust size as needed, e.g., 6 bytes per float + overhead
    JsonArray data = doc.createNestedArray("data");
    for (int i = 0; i < FEATURE_SIZE; i++) {
        data.add(features[i]);
    }

    String jsonPayload;
    serializeJson(doc, jsonPayload);

    int httpResponseCode = http.POST(jsonPayload);
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println("Server response: " + response);

        // Parse the JSON response
        DynamicJsonDocument docResponse(256); // Adjust size as needed
        DeserializationError error = deserializeJson(docResponse, response);
        if (!error) {
            const char* gesture = docResponse["gesture"];
            float confidence = docResponse["confidence"];

            Serial.println("Server Inference Result:");
            Serial.print("Gesture: ");
            Serial.println(gesture);
            Serial.print("Confidence: ");
            Serial.print(confidence);
            Serial.println("%");
            // Your code to actuate LED
            if (strcmp(gesture, "Z") == 0) {
                fadeNeoPixel(255, 0, 0);   // Red
            } else if (strcmp(gesture, "O") == 0) {
                fadeNeoPixel(0, 0, 255);   // Blue
            } else if (strcmp(gesture, "V") == 0) {
                fadeNeoPixel(0, 255, 0);   // Green
            } else {
                pixels.clear(); pixels.show(); // No color
            }
        } else {
            Serial.print("Failed to parse server response: ");
            Serial.println(error.c_str());
        }

    } else {
        Serial.printf("Error sending POST: %s\n", http.errorToString(httpResponseCode).c_str());
    }

    http.end();
}
