#ifndef INCUNEST_NTC_TABLES_H
#define INCUNEST_NTC_TABLES_H

static inline float ntc_beta_resistance(float temp_c, float r0, float beta) {
  const float t0 = 298.15f; /* 25C */
  const float tk = temp_c + 273.15f;
  float exp_term = beta * ((1.0f / tk) - (1.0f / t0));
  extern float expf(float x);
  return r0 * expf(exp_term);
}

static inline float ntc_divider_voltage(float temp_c, float vcc, float r_fixed,
                                        float r0, float beta) {
  float r_ntc = ntc_beta_resistance(temp_c, r0, beta);
  return vcc * (r_fixed / (r_fixed + r_ntc));
}

#endif
