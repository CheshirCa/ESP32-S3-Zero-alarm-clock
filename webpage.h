#ifndef WEBPAGE_H
#define WEBPAGE_H

#include <Arduino.h>

// ESP32-S3: Части HTML доступны напрямую для chunked transfer
extern const char html_part1[] PROGMEM;
extern const char html_part2[] PROGMEM;
extern const char html_part3[] PROGMEM;
extern const char html_part4[] PROGMEM;

// getWebPage() больше не используется - используем chunked transfer
// String getWebPage();  // ЗАКОММЕНТИРОВАНО

#endif
