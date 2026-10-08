#include "infrastructure/esp32/Esp32AnalogLightSensor.h"

#include <Arduino.h>

namespace coffee {

Esp32AnalogLightSensor::Esp32AnalogLightSensor(uint8_t pin, uint8_t samplesPerRead)
    : pin_(pin), samplesPerRead_(samplesPerRead == 0 ? 1 : samplesPerRead) {}

void Esp32AnalogLightSensor::begin() {
    available_ = digitalPinToAnalogChannel(pin_) >= 0;
    if (!available_) {
        return;
    }
    pinMode(pin_, INPUT);
    analogReadResolution(12);                 // 0..4095
    analogSetPinAttenuation(pin_, ADC_11db);  // full 0..3.3 V input range
}

LightReading Esp32AnalogLightSensor::read() {
    if (!available_) {
        return LightReading::invalid();
    }
    uint32_t sum = 0;
    for (uint8_t i = 0; i < samplesPerRead_; ++i) {
        sum += analogRead(pin_);
    }
    return LightReading::of(static_cast<uint16_t>(sum / samplesPerRead_));
}

}  // namespace coffee
