#ifndef INCUNEST_THERMAL_MODEL_H
#define INCUNEST_THERMAL_MODEL_H

typedef struct {
  float chamber_temp_c;
  float ambient_temp_c;
  float humidity_pct;
  float thermal_mass;
  float wall_loss_factor;
} thermal_state_t;

static inline void thermal_init(thermal_state_t *s, float ambient_temp_c) {
  s->ambient_temp_c = ambient_temp_c;
  s->chamber_temp_c = ambient_temp_c;
  s->humidity_pct = 50.0f;
  s->thermal_mass = 2.0f;
  s->wall_loss_factor = 0.002f;
}

static inline void thermal_step(thermal_state_t *s, float heater_power_norm,
                                float fan_power_norm, float door_open_norm,
                                float dt_seconds) {
  float heater_gain = 12.0f * heater_power_norm;
  float fan_mix = 1.5f * fan_power_norm;
  float door_loss = 8.0f * door_open_norm;
  float wall_loss = s->wall_loss_factor * (s->chamber_temp_c - s->ambient_temp_c) * 1000.0f;
  float net = heater_gain - wall_loss - door_loss + fan_mix;
  float d_temp = (net / (s->thermal_mass + 0.1f)) * dt_seconds;
  s->chamber_temp_c += d_temp;
}

#endif
