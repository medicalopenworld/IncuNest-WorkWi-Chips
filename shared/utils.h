#ifndef INCUNEST_SHARED_UTILS_H
#define INCUNEST_SHARED_UTILS_H

static inline float clampf(float v, float min_v, float max_v) {
  if (v < min_v) return min_v;
  if (v > max_v) return max_v;
  return v;
}

static inline float mapf(float x, float in_min, float in_max, float out_min, float out_max) {
  if (in_max == in_min) return out_min;
  float n = (x - in_min) / (in_max - in_min);
  return out_min + (n * (out_max - out_min));
}

static inline unsigned short crc8_sensirion(const unsigned char *data, int len) {
  unsigned short crc = 0xFF;
  for (int i = 0; i < len; i++) {
    crc ^= data[i];
    for (int b = 0; b < 8; b++) {
      if (crc & 0x80) {
        crc = (unsigned short)((crc << 1) ^ 0x31);
      } else {
        crc <<= 1;
      }
    }
  }
  return (unsigned short)(crc & 0xFF);
}

#endif
