#include "esphome.h"
#include <math.h>

class ADS1115RMS : public PollingComponent, public Sensor {
 public:
  ADS1115RMS(int channel, float v_mid, float burden, float ct_ratio)
  : PollingComponent(1000), channel_(channel),
    v_mid_(v_mid), burden_(burden), ct_ratio_(ct_ratio) {}

  sensor::ADS1115Sensor *ads_sensor_;

  void setup() override {}

  void update() override {
    const int samples = 200;
    float sum_sq = 0.0;

    for (int i = 0; i < samples; i++) {
      float v = ads_sensor_->get_raw_value(channel_) - v_mid_;
      sum_sq += v * v;
      delayMicroseconds(1000);
    }

    float rms_voltage = sqrt(sum_sq / samples);
    float secondary_current = rms_voltage / burden_;
    float primary_current = secondary_current * ct_ratio_;

    publish_state(primary_current);
  }

 protected:
  int channel_;
  float v_mid_;
  float burden_;
  float ct_ratio_;
};
