#include "ClockWebServer.h"
#include "webpage.h"
#include <HTTPClient.h>
#include <WiFi.h>
#include <esp_timer.h>

// Rate limiting для DoS защиты
unsigned long lastRequestTime = 0;
const unsigned long MIN_REQUEST_INTERVAL = 50; // 50ms между запросами

// Helper function: escape JSON special characters with proper UTF-8 support
String jsonEscape(String str) {
  String escaped = "";
  escaped.reserve(str.length() + 20);
  
  for (unsigned int i = 0; i < str.length(); ) {
    unsigned char c = (unsigned char)str[i];
    
    // Handle control characters
    if (c < 0x20) {
      switch (c) {
        case '"': escaped += "\\\""; i++; break;
        case '\\': escaped += "\\\\"; i++; break;
        case '\b': escaped += "\\b"; i++; break;
        case '\f': escaped += "\\f"; i++; break;
        case '\n': escaped += "\\n"; i++; break;
        case '\r': escaped += "\\r"; i++; break;
        case '\t': escaped += "\\t"; i++; break;
        default:
          char buf[7];
          sprintf(buf, "\\u%04x", c);
          escaped += buf;
          i++;
          break;
      }
    }
    // Handle UTF-8 multi-byte sequences
    else if (c < 0x80) {
      // ASCII character
      if (c == '"') {
        escaped += "\\\"";
      } else if (c == '\\') {
        escaped += "\\\\";
      } else {
        escaped += (char)c;
      }
      i++;
    }
    else {
      // UTF-8 multi-byte character - preserve as-is
      escaped += (char)c;
      i++;
    }
  }
  return escaped;
}

String getStatusJSON() {
  if (getLocalTime(&timeinfo) && timeinfo.tm_year > 120) {
    timeValid = true;
    updateClockStrings();
  }
  String json = "{";
  json.reserve(2048);
  json += "\"time\":\"" + String(hhStr) + ":" + String(mmStr) + "\",";
  json += "\"date\":\"" + String(dateStr) + "\",";
  json += "\"weekday\":\"" + String(weekdayStr) + "\",";
  json += "\"ssid\":\"" + jsonEscape(wifiSSID) + "\",";
  json += "\"wifi\":\"" + String(WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected") + "\",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
  json += "\"timeSync\":" + String(timeValid ? "true" : "false") + ",";
  json += "\"ntpServer\":\"" + jsonEscape(ntpServer) + "\",";
  json += "\"timezone\":" + String(gmtOffset_sec / 3600) + ",";
  json += "\"dstOffset\":" + String(daylightOffset_sec / 3600) + ",";
  json += "\"city\":\"" + jsonEscape(cityName) + "\",";
  json += "\"brightness\":" + String(displayBrightness) + ",";
  #if BUZZER_TYPE == BUZZER_PASSIVE
  json += "\"buzzerType\":\"passive\",";
  json += "\"alarmMelody\":\"" + jsonEscape(alarmMelody) + "\",";
  json += "\"timerMelody\":\"" + jsonEscape(timerMelody) + "\",";
  #else
  json += "\"buzzerType\":\"active\",";
  #endif
  if (timeValid) {
    char fullTime[20];
    sprintf(fullTime, "%04d-%02d-%02d %02d:%02d:%02d",
            timeinfo.tm_year + 1900, timeinfo.tm_mon + 1, timeinfo.tm_mday,
            timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    json += "\"fullTime\":\"" + String(fullTime) + "\",";
  } else {
    json += "\"fullTime\":\"\",";
  }
  // Sensor info
  json += "\"sensors\":{";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"humidity\":" + String(humidityVal, 1) + ",";
  json += "\"pressure\":" + String(pressure, 1);
  json += "},";
  json += "\"alarms\":[";
  for (int i = 0; i < MAX_ALARMS; i++) {
    if (i > 0) json += ",";
    json += "{\"index\":" + String(i) + ",";
    json += "\"active\":" + String(alarms[i].active ? "true" : "false") + ",";
    json += "\"ringing\":" + String((alarmTriggered && triggeredAlarmIndex == i) ? "true" : "false");
    if (alarms[i].active) {
      json += ",\"hour\":" + String(alarms[i].hour);
      json += ",\"minute\":" + String(alarms[i].minute);
      json += ",\"text\":\"" + jsonEscape(String(alarms[i].text)) + "\"";
      json += ",\"repeat\":" + String(alarms[i].repeat ? "true" : "false");
      json += ",\"saved\":" + String(alarms[i].saved ? "true" : "false");
      if (alarms[i].year > 0) {
        json += ",\"type\":\"date\"";
        json += ",\"date\":\"" + String(alarms[i].year) + "-";
        if (alarms[i].month < 10) json += "0";
        json += String(alarms[i].month) + "-";
        if (alarms[i].day < 10) json += "0";
        json += String(alarms[i].day) + "\"";
      } else if (alarms[i].weekdays > 0) {
        json += ",\"type\":\"weekdays\"";
        json += ",\"weekdays\":" + String(alarms[i].weekdays);
      } else {
        json += ",\"type\":\"daily\"";
      }
    }
    json += "}";
  }
  json += "],";
  json += "\"timer\":{";
  json += "\"active\":" + String(timerActive ? "true" : "false") + ",";
  json += "\"ringing\":" + String(timerTriggered ? "true" : "false");
  if (timerActive) {
    uint64_t elapsed = esp_timer_get_time() - timerStartUs;
    uint64_t remaining = (elapsed >= timerDurationUs) ? 0 : (timerDurationUs - elapsed);
    int secRemaining = (remaining + 500000) / 1000000;
    json += ",\"remaining\":" + String(secRemaining);
    json += ",\"text\":\"" + jsonEscape(String(timerText)) + "\"";
  }
  json += "},";
  json += "\"weather\":\"" + jsonEscape(weatherData) + "\",";
  // Night mode info
  json += "\"nightMode\":{";
  json += "\"enabled\":" + String(nightModeEnabled ? "true" : "false") + ",";
  json += "\"active\":" + String(nightModeActive ? "true" : "false") + ",";
  json += "\"startHour\":" + String(nightStartHour) + ",";
  json += "\"startMinute\":" + String(nightStartMinute) + ",";
  json += "\"endHour\":" + String(nightEndHour) + ",";
  json += "\"endMinute\":" + String(nightEndMinute) + ",";
  json += "\"brightness\":" + String(nightBrightness);
  json += "},";
  // WiFi mode info
  json += "\"wifiMode\":{";
  json += "\"mode\":" + String((int)wifiMode) + ",";
  json += "\"autoOffTimeout\":" + String(wifiAutoOffTimeout / 1000) + ",";
  json += "\"ntpSyncInterval\":" + String(ntpSyncInterval / 1000) + ",";
  json += "\"weatherSyncInterval\":" + String(weatherSyncInterval / 1000);
  if (wifiManuallyEnabled) {
    unsigned long remaining = (wifiManualTimeout > millis()) ? (wifiManualTimeout - millis()) / 1000 : 0;
    json += ",\"manuallyEnabled\":true";
    json += ",\"manualTimeoutRemaining\":" + String(remaining);
  } else {
    json += ",\"manuallyEnabled\":false";
  }
  json += "},";
  // Theme and Language
  json += "\"theme\":\"" + jsonEscape(currentTheme) + "\",";
  json += "\"language\":\"" + jsonEscape(currentLanguage) + "\"";
  json += "}";
  return json;
}

String urlDecode(String str) {
  str.replace("+", " ");
  String decoded = "";
  // Резервируем больше места для безопасности
  decoded.reserve(str.length() * 2);
  
  for (unsigned int i = 0; i < str.length(); i++) {
    if (str[i] == '%' && i + 2 < str.length()) {
      String hex = str.substring(i + 1, i + 3);
      char c = strtol(hex.c_str(), NULL, 16);
      decoded += c;
      i += 2;
    } else {
      decoded += str[i];
    }
  }
  return decoded;
}

// Безопасное копирование текста с проверкой границ
void safeTextCopy(char* dest, int destSize, const String& src) {
  int byteCount = 0;
  for (unsigned int i = 0; i < src.length() && byteCount < destSize - 1; i++) {
    dest[byteCount++] = src[i];
  }
  // Гарантируем null-terminator
  if (byteCount < destSize) {
    dest[byteCount] = '\0';
  } else {
    dest[destSize - 1] = '\0';
  }
}

// Rate limiting check
bool checkRateLimit() {
  unsigned long now = millis();
  if (now - lastRequestTime < MIN_REQUEST_INTERVAL) {
    return false;
  }
  lastRequestTime = now;
  return true;
}


void setupWebServer() {
  server.on("/", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    lastWebRequest = millis();
    
    // CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Access-Control-Allow-Methods", "GET, POST");
    
    // ESP32-S3 FIX: Chunked transfer - отправляем HTML по частям
    // Это обходит ограничение String в ~50KB
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html; charset=utf-8", "");
    
    server.sendContent(FPSTR(html_part1));
    server.sendContent(FPSTR(html_part2));
    server.sendContent(FPSTR(html_part3));
    server.sendContent(FPSTR(html_part4));
    
    server.sendContent("");
  });

  server.on("/preferences/set", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    lastWebRequest = millis();
    
    // CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    bool changed = false;
    
    if (server.hasArg("theme")) {
      String theme = server.arg("theme");
      // Используем === для строгого сравнения
      if (theme.equals("light") || theme.equals("dark")) {
        currentTheme = theme;
        changed = true;
      }
    }
    
    if (server.hasArg("language")) {
      String language = server.arg("language");
      if (language.equals("en") || language.equals("ru")) {
        currentLanguage = language;
        changed = true;
      }
    }
    
    
    if (changed) {
      saveThemeLanguageSettings();
      server.send(200, "text/plain; charset=utf-8", "Settings saved successfully");
    } else {
      server.send(400, "text/plain; charset=utf-8", "Invalid parameters");
    }
  });
  
  server.on("/status", HTTP_GET, [&]() {
    lastWebRequest = millis();
    
    // CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    server.send(200, "application/json; charset=utf-8", getStatusJSON());
  });
  
  server.on("/sensor/update", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "application/json", "{\"error\":\"Too many requests\"}");
      return;
    }
    lastWebRequest = millis();
    
    // CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    String json = "{";
    json += "\"status\":\"ok\",";
    json += "\"temperature\":" + String(temperature, 1) + ",";
    json += "\"humidity\":" + String(humidityVal, 1) + ",";
    json += "\"pressure\":" + String(pressure, 1);
    json += "}";
    server.send(200, "application/json; charset=utf-8", json);
  });
  
  server.on("/weather", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    lastWebRequest = millis();
    
    if (server.hasArg("city")) {
      cityName = server.arg("city");
    }
    
  
    needWeatherUpdate = true;
    
    server.send(200, "text/plain; charset=utf-8", "Weather update scheduled for " + cityName);
  });
  
  server.on("/alarm", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    lastWebRequest = millis();
    
    if (!timeValid) {
      server.send(400, "text/plain; charset=utf-8", "Wait for time sync!");
      return;
    }
    
    String timeStr = server.arg("time");
    if (timeStr.length() == 0) {
      server.send(400, "text/plain; charset=utf-8", "Missing time parameter");
      return;
    }
    
    int hour = timeStr.substring(0, 2).toInt();
    int minute = timeStr.substring(3, 5).toInt();
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
      server.send(400, "text/plain; charset=utf-8", "Invalid time");
      return;
    }
    
   
    int idx = 0;
    alarms[idx].active = true;
    alarms[idx].hour = hour;
    alarms[idx].minute = minute;
    alarms[idx].repeat = server.hasArg("repeat");
    
    String text = server.arg("text");
    if (text.length() > 0) {
      String decoded = urlDecode(text);
      safeTextCopy(alarms[idx].text, sizeof(alarms[idx].text), decoded);
    } else {
      alarms[idx].text[0] = '\0';
    }
    
    String type = server.arg("type");
    if (type == "date") {
      String dateStr = server.arg("date");
      alarms[idx].year = dateStr.substring(0, 4).toInt();
      alarms[idx].month = dateStr.substring(5, 7).toInt();
      alarms[idx].day = dateStr.substring(8, 10).toInt();
      alarms[idx].weekdays = 0;
    } else if (type == "weekdays") {
      alarms[idx].year = 0;
      alarms[idx].month = 0;
      alarms[idx].day = 0;
      alarms[idx].weekdays = server.arg("weekdays").toInt();
    } else {
      alarms[idx].year = 0;
      alarms[idx].month = 0;
      alarms[idx].day = 0;
      alarms[idx].weekdays = 0;
    }
    
    saveAlarmToNVS(idx);
    server.send(200, "text/plain; charset=utf-8", "Alarm set successfully");
  });
  
  server.on("/alarm/list", HTTP_GET, [&]() {
    lastWebRequest = millis();
    
    // CORS headers
    server.sendHeader("Access-Control-Allow-Origin", "*");
    
    String json = "{\"alarms\":[";
    for (int i = 0; i < MAX_ALARMS; i++) {
      if (i > 0) json += ",";
      json += "{\"index\":" + String(i) + ",";
      json += "\"active\":" + String(alarms[i].active ? "true" : "false") + ",";
      json += "\"hour\":" + String(alarms[i].hour) + ",";
      json += "\"minute\":" + String(alarms[i].minute) + ",";
      json += "\"repeat\":" + String(alarms[i].repeat ? "true" : "false") + ",";
      json += "\"weekdays\":" + String(alarms[i].weekdays) + ",";
      json += "\"text\":\"" + jsonEscape(String(alarms[i].text)) + "\",";
      json += "\"useDefaultMelody\":" + String(alarms[i].useDefaultMelody ? "true" : "false") + "}";
    }
    json += "]}";
    
   
    server.send(200, "application/json; charset=utf-8", json);
  });
  
  server.on("/alarm/set", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    if (!server.hasArg("index")) {
      server.send(400, "text/plain; charset=utf-8", "Missing index parameter");
      return;
    }
    
    int idx = server.arg("index").toInt();
    if (idx < 0 || idx >= MAX_ALARMS) {
      server.send(400, "text/plain; charset=utf-8", "Invalid index");
      return;
    }
    
    if (!timeValid) {
      server.send(400, "text/plain; charset=utf-8", "Wait for time sync!");
      return;
    }
    
    String timeStr = server.arg("time");
    if (timeStr.length() == 0) {
      server.send(400, "text/plain; charset=utf-8", "Missing time parameter");
      return;
    }
    
    int hour = timeStr.substring(0, 2).toInt();
    int minute = timeStr.substring(3, 5).toInt();
    if (hour < 0 || hour > 23 || minute < 0 || minute > 59) {
      server.send(400, "text/plain; charset=utf-8", "Invalid time");
      return;
    }
    
   
    alarms[idx].active = true;
    alarms[idx].hour = hour;
    alarms[idx].minute = minute;
    alarms[idx].repeat = server.hasArg("repeat");
    
    String text = server.arg("text");
    if (text.length() > 0) {
      String decoded = urlDecode(text);
      safeTextCopy(alarms[idx].text, sizeof(alarms[idx].text), decoded);
    } else {
      alarms[idx].text[0] = '\0';
    }
    
    String type = server.arg("type");
    if (type == "date") {
      String dateStr = server.arg("date");
      alarms[idx].year = dateStr.substring(0, 4).toInt();
      alarms[idx].month = dateStr.substring(5, 7).toInt();
      alarms[idx].day = dateStr.substring(8, 10).toInt();
      alarms[idx].weekdays = 0;
    } else if (type == "weekdays") {
      alarms[idx].year = 0;
      alarms[idx].month = 0;
      alarms[idx].day = 0;
      alarms[idx].weekdays = server.arg("weekdays").toInt();
    } else {
      alarms[idx].year = 0;
      alarms[idx].month = 0;
      alarms[idx].day = 0;
      alarms[idx].weekdays = 0;
    }
    
    if (server.hasArg("melody")) {
      String mel = server.arg("melody");
      String decoded = urlDecode(mel);
      // Безопасное копирование с проверкой границ
      safeTextCopy(alarms[idx].melody, sizeof(alarms[idx].melody), decoded);
      alarms[idx].useDefaultMelody = false;
    }
    
    if (server.hasArg("useDefault")) {
      alarms[idx].useDefaultMelody = (server.arg("useDefault") == "1");
    }
    
    saveAlarmToNVS(idx);
    server.send(200, "text/plain; charset=utf-8", "Alarm set successfully");
  });
  
  server.on("/alarm/clear", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    int idx = 0;
    if (server.hasArg("index")) {
      idx = server.arg("index").toInt();
    }
    
    if (idx < 0 || idx >= MAX_ALARMS) {
      server.send(400, "text/plain; charset=utf-8", "Invalid index");
      return;
    }
    
    
    clearAlarmFromNVS(idx);
    server.send(200, "text/plain; charset=utf-8", "Alarm cleared");
  });
  
  server.on("/alarm/clearall", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
   
    for (int i = 0; i < MAX_ALARMS; i++) {
      clearAlarmFromNVS(i);
    }
    server.send(200, "text/plain; charset=utf-8", "All alarms cleared");
  });
  
  server.on("/alarm/toggle", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    if (!server.hasArg("index")) {
      server.send(400, "text/plain; charset=utf-8", "Missing index parameter");
      return;
    }
    
    int idx = server.arg("index").toInt();
    if (idx < 0 || idx >= MAX_ALARMS) {
      server.send(400, "text/plain; charset=utf-8", "Invalid index");
      return;
    }
    
    alarms[idx].active = !alarms[idx].active;
    
   
    saveAlarmToNVS(idx);
    server.send(200, "text/plain; charset=utf-8", alarms[idx].active ? "Alarm ON" : "Alarm OFF");
  });
  
  server.on("/timer", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    String durationStr = server.arg("duration");
    if (durationStr.length() == 0) {
      server.send(400, "text/plain; charset=utf-8", "Missing duration parameter");
      return;
    }
    
    unsigned long totalSec = durationStr.toInt();
    if (totalSec == 0 || totalSec > 86400) {
      server.send(400, "text/plain; charset=utf-8", "Timer must be 1-86400 seconds");
      return;
    }
    

    
    String text = server.arg("text");
    if (text.length() > 0) {
      String decoded = urlDecode(text);
      safeTextCopy(timerText, sizeof(timerText), decoded);
    } else {
      strcpy(timerText, "TIMER");
    }
    
    timerActive = true;
    timerTriggered = false;
    #if BUZZER_TYPE == BUZZER_PASSIVE
    timerMelodyStarted = false;  // Reset timer melody flag
    #endif
    timerStartUs = esp_timer_get_time();
    timerDurationUs = (uint64_t)totalSec * 1000000;
    server.send(200, "text/plain; charset=utf-8", "Timer started");
  });
  
  server.on("/timer/clear", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    
    timerActive = false;
    timerTriggered = false;
    buzzerActive = false;
    #if BUZZER_TYPE == BUZZER_PASSIVE
    timerMelodyStarted = false;  // Reset timer melody flag
    stopMelody();
    #else
    digitalWrite(BUZZER_PIN, LOW);
    #endif
    server.send(200, "text/plain; charset=utf-8", "Timer cleared");
  });
  
  server.on("/sync", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    if (server.hasArg("ntp")) {
      ntpServer = server.arg("ntp");
    }
    if (server.hasArg("tz")) {
      gmtOffset_sec = server.arg("tz").toInt() * 3600;
    }
    if (server.hasArg("dst")) {
      daylightOffset_sec = server.arg("dst").toInt() * 3600;
    }
    
    
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer.c_str());
    
    // ИСПРАВЛЕНИЕ: Неблокирующая синхронизация
    // Вместо блокирующего while используем отложенную проверку
    unsigned long syncStart = millis();
    bool synced = false;
    
    // Даем 2 секунды на синхронизацию, но не блокируем
    for (int i = 0; i < 20 && !synced; i++) {
      delay(100); // короткие задержки
      if (getLocalTime(&timeinfo) && timeinfo.tm_year > 120) {
        timeValid = true;
        updateClockStrings();
        synced = true;
      }
    }
    
    if (synced) {
      server.send(200, "text/plain; charset=utf-8", "Time synchronized successfully");
    } else {
      server.send(500, "text/plain; charset=utf-8", "Time synchronization failed");
    }
  });
  
  server.on("/time", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    String timeStr = server.arg("value");
    if (timeStr.length() == 0) {
      server.send(400, "text/plain; charset=utf-8", "Missing time parameter");
      return;
    }
    
    
    struct tm t {};
    if (sscanf(timeStr.c_str(), "%d-%d-%d %d:%d:%d",
               &t.tm_year, &t.tm_mon, &t.tm_mday,
               &t.tm_hour, &t.tm_min, &t.tm_sec) == 6) {
      t.tm_year -= 1900;
      t.tm_mon -= 1;
      time_t tt = mktime(&t);
      struct timeval now = { tt, 0 };
      settimeofday(&now, nullptr);
      timeValid = true;
      updateClockStrings();
      server.send(200, "text/plain; charset=utf-8", "Manual time set successfully");
    } else {
      server.send(400, "text/plain; charset=utf-8", "Invalid time format. Use: YYYY-MM-DD HH:MM:SS");
    }
  });
  
  server.on("/save", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    if (server.hasArg("city")) {
      cityName = server.arg("city");
    }
    if (server.hasArg("brightness")) {
      int bright = server.arg("brightness").toInt();
      setDisplayBrightness(bright);
    }
    
    
    saveConfigToNVS();
    saveDisplaySettings();
    server.send(200, "text/plain; charset=utf-8", "Settings saved to NVS");
  });
  
  server.on("/restore", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    
    loadConfigFromNVS();
    loadDisplaySettings();
    setDisplayBrightness(displayBrightness);
    connectWiFi();
    if (WiFi.status() == WL_CONNECTED) {
      syncTime();
    }
    updateClockStrings();
    server.send(200, "text/plain; charset=utf-8", "Settings restored from NVS");
  });
  
  server.on("/erase", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    
    eraseNVS();
    server.send(200, "text/plain; charset=utf-8", "NVS erased successfully");
  });
  
  server.on("/reboot", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    
    server.send(200, "text/plain; charset=utf-8", "Rebooting...");
    
    // ИСПРАВЛЕНИЕ: Неблокирующая задержка
    delay(1000);
    ESP.restart();
  });
  
  #if BUZZER_TYPE == BUZZER_PASSIVE
  server.on("/melody/test", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    String melody = server.arg("melody");
    if (melody.length() == 0) {
      server.send(400, "text/plain; charset=utf-8", "Missing melody parameter");
      return;
    }
    
    
    String decoded = urlDecode(melody);
    Note testNotes[50];
    int testNoteCount = 0;
    parseMelody(decoded, testNotes, testNoteCount, 50);
    
    if (testNoteCount > 0) {
      // ИСПРАВЛЕНИЕ: Неблокирующее проигрывание
      // Вместо блокирующего while используем короткие delay
      for (int i = 0; i < testNoteCount; i++) {
        if (testNotes[i].frequency > 0) {
          buzzerTone(BUZZER_PIN, testNotes[i].frequency);
        } else {
          buzzerNoTone(BUZZER_PIN);
        }
        delay(testNotes[i].duration); // Простая задержка вместо блокирующего цикла
      }
      buzzerNoTone(BUZZER_PIN);
      server.send(200, "text/plain; charset=utf-8", "Melody tested");
    } else {
      server.send(400, "text/plain; charset=utf-8", "Invalid melody format");
    }
  });
  
  server.on("/melody/save", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    String alarm = server.arg("alarm");
    String timer = server.arg("timer");
    
    
    if (alarm.length() > 0) {
      alarmMelody = urlDecode(alarm);
      parseMelody(alarmMelody, alarmNotes, alarmNoteCount, 50);
    }
    if (timer.length() > 0) {
      timerMelody = urlDecode(timer);
      parseMelody(timerMelody, timerNotes, timerNoteCount, 50);
    }
    saveMelodySettings();
    server.send(200, "text/plain; charset=utf-8", "Melodies saved");
  });
  #endif
  
  // Stop all ringing alarms
  server.on("/alarm/stop", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    
    alarmTriggered = false;
    triggeredAlarmIndex = -1;
    buzzerActive = false;
    #if BUZZER_TYPE == BUZZER_PASSIVE
    alarmMelodyStarted = false;  // Reset alarm melody flag
    timerMelodyStarted = false;  // Also reset timer flag
    stopMelody();
    #else
    digitalWrite(BUZZER_PIN, LOW);
    #endif
    server.send(200, "text/plain; charset=utf-8", "All alarms stopped");
  });
  
  // Add time to running timer
  server.on("/timer/add", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    
    if (!timerActive) {
      server.send(400, "text/plain; charset=utf-8", "No timer is running");
      return;
    }
    
    String timeStr = server.arg("time");
    if (timeStr.length() == 0) {
      server.send(400, "text/plain; charset=utf-8", "Missing time parameter");
      return;
    }
    
    int additionalSec = timeStr.toInt();
    if (additionalSec <= 0 || additionalSec > 3600) {
      server.send(400, "text/plain; charset=utf-8", "Time must be 1-3600 seconds");
      return;
    }
    
    
    timerDurationUs += (uint64_t)additionalSec * 1000000;
    server.send(200, "text/plain; charset=utf-8", "Timer extended by " + String(additionalSec) + " seconds");
  });
  
  // Set brightness without saving to NVS
  server.on("/brightness/set", HTTP_GET, [&]() {
    lastWebRequest = millis();
    
    if (!server.hasArg("value")) {
      server.send(400, "text/plain; charset=utf-8", "Missing value parameter");
      return;
    }
    
    int bright = server.arg("value").toInt();
    if (bright < 0 || bright > 255) {
      server.send(400, "text/plain; charset=utf-8", "Brightness must be 0-255");
      return;
    }
    
    
    // If night mode is active, update night brightness instead
    if (nightModeActive) {
      nightBrightness = bright;
    } else {
      displayBrightness = bright;
    }
    setDisplayBrightness(bright);
    server.send(200, "text/plain; charset=utf-8", "Brightness set to " + String(bright));
  });
  
  // Configure night mode
  server.on("/nightmode/set", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    lastWebRequest = millis();
    
    if (server.hasArg("enabled")) {
      nightModeEnabled = (server.arg("enabled") == "1");
    }
    if (server.hasArg("startHour")) {
      nightStartHour = server.arg("startHour").toInt();
    }
    if (server.hasArg("startMinute")) {
      nightStartMinute = server.arg("startMinute").toInt();
    }
    if (server.hasArg("endHour")) {
      nightEndHour = server.arg("endHour").toInt();
    }
    if (server.hasArg("endMinute")) {
      nightEndMinute = server.arg("endMinute").toInt();
    }
    if (server.hasArg("brightness")) {
      nightBrightness = server.arg("brightness").toInt();
    }
    
    
    if (server.hasArg("save") && server.arg("save") == "1") {
      saveNightModeSettings();
    }
    checkNightMode(); // Apply immediately
    server.send(200, "text/plain; charset=utf-8", "Night mode configured");
  });
  
  // Configure WiFi mode
  server.on("/wifimode/set", HTTP_GET, [&]() {
    if (!checkRateLimit()) {
      server.send(429, "text/plain", "Too many requests");
      return;
    }
    lastWebRequest = millis();
    
    if (server.hasArg("mode")) {
      int mode = server.arg("mode").toInt();
      if (mode >= 0 && mode <= 1) {
        wifiMode = (WiFiMode)mode;
      }
    }
    if (server.hasArg("autoOff")) {
      wifiAutoOffTimeout = server.arg("autoOff").toInt() * 60000;
    }
    if (server.hasArg("ntpSync")) {
      ntpSyncInterval = server.arg("ntpSync").toInt() * 60000;
    }
    if (server.hasArg("weatherSync")) {
      weatherSyncInterval = server.arg("weatherSync").toInt() * 60000;
    }
    
    
    if (server.hasArg("save") && server.arg("save") == "1") {
      saveWiFiModeSettings();
    }
    server.send(200, "text/plain; charset=utf-8", "WiFi mode configured");
  });
  
  // Time sync handler
  server.on("/time/sync", HTTP_GET, [&]() {
    lastWebRequest = millis();
    if (WiFi.status() != WL_CONNECTED) {
      server.send(503, "text/plain; charset=utf-8", "WiFi not connected");
      return;
    }
    syncTime();
    if (timeValid) {
      server.send(200, "text/plain; charset=utf-8", "Time synchronized successfully");
    } else {
      server.send(500, "text/plain; charset=utf-8", "Time sync failed");
    }
  });
  
  // Manual time set handler
  server.on("/time/set", HTTP_GET, [&]() {
    lastWebRequest = millis();
    if (server.hasArg("time")) {
      String timeStr = server.arg("time");
      setManualTime(timeStr);
      updateClockStrings();
      server.send(200, "text/plain; charset=utf-8", "Time set successfully");
    } else {
      server.send(400, "text/plain; charset=utf-8", "Missing time parameter");
    }
  });
  
  // Weather update handler
  server.on("/weather/update", HTTP_GET, [&]() {
    lastWebRequest = millis();
    if (WiFi.status() != WL_CONNECTED) {
      server.send(503, "text/plain; charset=utf-8", "WiFi not connected");
      return;
    }
    updateWeather();
    delay(100); // Give time for update to complete
    server.send(200, "text/plain; charset=utf-8", "Weather updated");
  });
  
  server.begin();
}
