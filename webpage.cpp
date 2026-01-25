#include "webpage.h"

// HTML разбит на части по ЛОГИЧЕСКИМ ГРАНИЦАМ для предотвращения разрыва JavaScript
// Часть 1: HTML до <script>
// Часть 2: JavaScript код (весь <script> блок)
// Часть 3: HTML после </script>

const char html_part1[] PROGMEM = R"rawliteral(

<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 Clock</title>
  <link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='0.9em' font-size='90'>⏰</text></svg>">
  <style>
    * {
      margin: 0;
      padding: 0;
      box-sizing: border-box;
    }
    
    :root {
      /* Light theme colors */
      --bg-gradient-start: #667eea;
      --bg-gradient-end: #764ba2;
      --card-bg: white;
      --card-bg-alpha: rgba(255, 255, 255, 0.95);
      --text-primary: #333;
      --text-secondary: #666;
      --text-tertiary: #999;
      --border-color: #e5e7eb;
      --sensor-bg: #f7f7f7;
      --weather-bg: #f0f9ff;
      --accent-color: #667eea;  
      --success-color: #10b981;
      --warning-color: #f59e0b;
      --error-color: #ef4444;
      --inactive-bg: #e5e7eb;
      --inactive-text: #9ca3af;
      --shadow: rgba(0,0,0,0.1);
      --modal-overlay: rgba(0,0,0,0.5);
    }
    
    body.dark-theme {
      /* Dark theme colors */
      --bg-gradient-start: #1e1b4b;
      --bg-gradient-end: #312e81;
      --card-bg: #1f2937;
      --card-bg-alpha: rgba(31, 41, 55, 0.95);
      --text-primary: #f3f4f6;
      --text-secondary: #d1d5db;
      --text-tertiary: #9ca3af;
      --border-color: #374151;
      --sensor-bg: #374151;
      --weather-bg: #1e3a5f;
      --accent-color: #818cf8;
      --success-color: #34d399;
      --warning-color: #fbbf24;
      --error-color: #f87171;
      --inactive-bg: #4b5563;
      --inactive-text: #6b7280;
      --shadow: rgba(0,0,0,0.3);
      --modal-overlay: rgba(0,0,0,0.7);
    }
    
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, Arial, sans-serif;
      background: linear-gradient(135deg, var(--bg-gradient-start) 0%, var(--bg-gradient-end) 100%);
      min-height: 100vh;
      padding: 20px;
      color: var(--text-primary);
      transition: background 0.3s, color 0.3s;
    }
    
    .container {
      max-width: 1200px;
      margin: 0 auto;
    }
    
    .header {
      background: var(--card-bg-alpha);
      border-radius: 12px;
      padding: 12px 20px;
      margin-bottom: 20px;
      display: flex;
      justify-content: space-between;
      align-items: center;
      box-shadow: 0 4px 6px var(--shadow);
    }
    
    .header-title {
      font-size: 1.2em;
      font-weight: 600;
      color: var(--accent-color);
    }
    
    .wifi-status {
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 0.9em;
      color: var(--text-secondary);
    }
    
    .wifi-icon {
      font-size: 1.2em;
    }
    
    .main-content {
      display: grid;
      grid-template-columns: 1fr 200px;
      gap: 20px;
    }
    
    .dashboard {
      background: var(--card-bg);
      border-radius: 12px;
      padding: 40px;
      box-shadow: 0 4px 6px var(--shadow);
    }
    
    .clock {
      text-align: center;
      margin-bottom: 30px;
    }
    
    .clock-time {
      font-size: 7em;
      font-weight: 700;
      color: var(--accent-color);
      line-height: 1;
      margin-bottom: 10px;
    }
    
    .clock-date {
      font-size: 1.5em;
      color: var(--text-secondary);
      margin-bottom: 5px;
    }
    
    .clock-weekday {
      font-size: 1.2em;
      color: var(--text-tertiary);
    }
    
    .sensors {
      display: grid;
      grid-template-columns: repeat(3, 1fr);
      gap: 15px;
      margin-bottom: 30px;
    }
    
    .sensor-card {
      background: var(--sensor-bg);
      border-radius: 8px;
      padding: 20px;
      text-align: center;
      transition: background-color 0.3s;
    }
    
    .sensor-card.temp-cold {
      background: linear-gradient(135deg, #a8d8ea 0%, #89c4d4 100%);
      color: #1e3a8a;
    }
    
    .sensor-card.temp-normal {
      background: linear-gradient(135deg, #86efac 0%, #4ade80 100%);
      color: #14532d;
    }
    
    .sensor-card.temp-warm {
      background: linear-gradient(135deg, #fef08a 0%, #fde047 100%);
      color: #713f12;
    }
    
    .sensor-card.temp-hot {
      background: linear-gradient(135deg, #fed7aa 0%, #fb923c 100%);
      color: #7c2d12;
    }
    
    .sensor-card.humidity-low {
      background: linear-gradient(135deg, #fca5a5 0%, #ef4444 100%);
      color: #7f1d1d;
    }
    
    .sensor-card.humidity-normal {
      background: linear-gradient(135deg, #86efac 0%, #4ade80 100%);
      color: #14532d;
    }
    
    .sensor-card.humidity-high {
      background: linear-gradient(135deg, #fde047 0%, #fb923c 100%);
      color: #7c2d12;
    }
    
    .sensor-card.pressure-high {
      background: url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 200 200"><circle cx="100" cy="100" r="40" fill="%23fbbf24"/><circle cx="100" cy="100" r="30" fill="%23fcd34d"/><path d="M100,70 L105,90 L95,90 Z" fill="%23fcd34d"/><path d="M100,130 L105,110 L95,110 Z" fill="%23fcd34d"/><path d="M70,100 L90,105 L90,95 Z" fill="%23fcd34d"/><path d="M130,100 L110,105 L110,95 Z" fill="%23fcd34d"/></svg>') no-repeat center center;
      background-size: 60px 60px;
      background-color: #fef3c7;
      color: #78350f;
    }
    
    .sensor-card.pressure-normal {
      background: url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 200 200"><ellipse cx="100" cy="90" rx="50" ry="30" fill="%23d1d5db"/><ellipse cx="130" cy="110" rx="40" ry="25" fill="%23e5e7eb"/><ellipse cx="70" cy="105" rx="35" ry="20" fill="%23e5e7eb"/></svg>') no-repeat center center;
      background-size: 80px 60px;
      background-color: #f3f4f6;
      color: #374151;
    }
    
    .sensor-card.pressure-low {
      background: url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 200 200"><ellipse cx="100" cy="80" rx="55" ry="30" fill="%236b7280"/><path d="M70,120 L65,140 M80,115 L77,135 M90,115 L88,135 M100,115 L100,140 M110,115 L112,135 M120,115 L123,135 M130,120 L135,140" stroke="%233b82f6" stroke-width="2" fill="none"/></svg>') no-repeat center center;
      background-size: 80px 70px;
      background-color: #dbeafe;
      color: #1e3a8a;
    }
    
    /* Dark theme overrides for sensor cards */
    body.dark-theme .sensor-card.temp-cold {
      background: linear-gradient(135deg, #164e63 0%, #155e75 100%);
      color: #e0f2fe;
    }
    
    body.dark-theme .sensor-card.temp-normal {
      background: linear-gradient(135deg, #065f46 0%, #047857 100%);
      color: #d1fae5;
    }
    
    body.dark-theme .sensor-card.temp-warm {
      background: linear-gradient(135deg, #78350f 0%, #92400e 100%);
      color: #fef3c7;
    }
    
    body.dark-theme .sensor-card.temp-hot {
      background: linear-gradient(135deg, #7c2d12 0%, #9a3412 100%);
      color: #fed7aa;
    }
    
    body.dark-theme .sensor-card.humidity-low {
      background: linear-gradient(135deg, #7f1d1d 0%, #991b1b 100%);
      color: #fecaca;
    }
    
    body.dark-theme .sensor-card.humidity-normal {
      background: linear-gradient(135deg, #065f46 0%, #047857 100%);
      color: #d1fae5;
    }
    
    body.dark-theme .sensor-card.humidity-high {
      background: linear-gradient(135deg, #78350f 0%, #92400e 100%);
      color: #fef3c7;
    }
    
    body.dark-theme .sensor-card.pressure-high {
      background: url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 200 200"><circle cx="100" cy="100" r="40" fill="%23fbbf24"/><circle cx="100" cy="100" r="30" fill="%23fcd34d"/><path d="M100,70 L105,90 L95,90 Z" fill="%23fcd34d"/><path d="M100,130 L105,110 L95,110 Z" fill="%23fcd34d"/><path d="M70,100 L90,105 L90,95 Z" fill="%23fcd34d"/><path d="M130,100 L110,105 L110,95 Z" fill="%23fcd34d"/></svg>') no-repeat center center;
      background-size: 60px 60px;
      background-color: #78350f;
      color: #fef3c7;
    }
    
    body.dark-theme .sensor-card.pressure-normal {
      background: url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 200 200"><ellipse cx="100" cy="90" rx="50" ry="30" fill="%23d1d5db"/><ellipse cx="130" cy="110" rx="40" ry="25" fill="%23e5e7eb"/><ellipse cx="70" cy="105" rx="35" ry="20" fill="%23e5e7eb"/></svg>') no-repeat center center;
      background-size: 80px 60px;
      background-color: #4b5563;
      color: #f3f4f6;
    }
    
    body.dark-theme .sensor-card.pressure-low {
      background: url('data:image/svg+xml;utf8,<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 200 200"><ellipse cx="100" cy="80" rx="55" ry="30" fill="%236b7280"/><path d="M70,120 L65,140 M80,115 L77,135 M90,115 L88,135 M100,115 L100,140 M110,115 L112,135 M120,115 L123,135 M130,120 L135,140" stroke="%233b82f6" stroke-width="2" fill="none"/></svg>') no-repeat center center;
      background-size: 80px 70px;
      background-color: #1e3a8a;
      color: #dbeafe;
    }
    
    .sensor-icon {
      font-size: 2em;
      margin-bottom: 8px;
    }
    
    .sensor-value {
      font-size: 1.8em;
      font-weight: 600;
      color: inherit;
      margin-bottom: 4px;
    }
    
    .sensor-label {
      font-size: 0.9em;
      color: inherit;
      opacity: 0.8;
    }
    
    .weather {
      background: var(--weather-bg);
      border-radius: 8px;
      padding: 20px;
      border-left: 4px solid var(--accent-color);
    }
    
    .weather-title {
      font-weight: 600;
      margin-bottom: 10px;
      color: var(--text-primary);
    }
    
    .weather-text {
      color: var(--text-secondary);
      font-size: 0.95em;
    }
    
    .weather-city {
      color: var(--accent-color);
      font-weight: 600;
    }
    
    .sidebar {
      display: flex;
      flex-direction: column;
      gap: 8px;
    }
    
    .alarm-btn, .timer-btn, .settings-btn {
      background: var(--card-bg);
      border: 2px solid var(--border-color);
      border-radius: 8px;
      padding: 15px;
      font-size: 0.9em;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.2s;
      color: var(--text-primary);
      text-align: center;
    }
    
    .alarm-btn:hover, .timer-btn:hover, .settings-btn:hover {
      transform: translateY(-2px);
      box-shadow: 0 4px 8px var(--shadow);
    }
    
    .alarm-btn.active, .timer-btn.active {
      background: var(--success-color);
      color: white;
      border-color: var(--success-color);
    }
    
    .alarm-btn.ringing, .timer-btn.ringing {
      background: var(--error-color);
      color: white;
      border-color: var(--error-color);
      animation: pulse 1s infinite;
    }
    
    .alarm-btn.inactive {
      background: var(--inactive-bg);
      color: var(--inactive-text);
      border-color: var(--inactive-bg);
    }
    
    @keyframes pulse {
      0%, 100% { opacity: 1; }
      50% { opacity: 0.6; }
    }
    
    /* Modals */
    .modal {
      display: none;
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: var(--modal-overlay);
      z-index: 1000;
      overflow-y: auto;
      padding: 20px;
    }
    
    .modal.show {
      display: flex;
      align-items: flex-start;
      justify-content: center;
    }
    
    .modal-content {
      background: var(--card-bg);
      border-radius: 12px;
      padding: 30px;
      max-width: 500px;
      width: 100%;
      margin: 40px auto;
      box-shadow: 0 10px 25px var(--shadow);
    }
    
    .modal-header {
      font-size: 1.5em;
      font-weight: 700;
      margin-bottom: 20px;
      color: var(--accent-color);
    }
    
    .form-group {
      margin-bottom: 20px;
    }
    
    .form-label {
      display: block;
      margin-bottom: 8px;
      font-weight: 600;
      color: var(--text-primary);
    }
    
    .form-input, .form-select, .form-textarea {
      width: 100%;
      padding: 10px;
      border: 2px solid var(--border-color);
      border-radius: 6px;
      font-size: 1em;
      background: var(--card-bg);
      color: var(--text-primary);
      transition: border-color 0.2s;
    }
    
    .form-input:focus, .form-select:focus, .form-textarea:focus {
      outline: none;
      border-color: var(--accent-color);
    }
    
    .checkbox-group {
      display: flex;
      align-items: center;
      gap: 10px;
      cursor: pointer;
    }
    
    .checkbox-group input[type="checkbox"] {
      width: 20px;
      height: 20px;
      cursor: pointer;
    }
    
    .weekdays {
      display: flex;
      gap: 8px;
      flex-wrap: wrap;
    }
    
    .weekday-btn {
      flex: 1;
      min-width: 45px;
      padding: 10px;
      border: 2px solid var(--border-color);
      border-radius: 6px;
      background: var(--card-bg);
      color: var(--text-primary);
      cursor: pointer;
      transition: all 0.2s;
      font-weight: 600;
    }
    
    .weekday-btn:hover {
      background: var(--accent-color);
      color: white;
      border-color: var(--accent-color);
    }
    
    .weekday-btn.selected {
      background: var(--accent-color);
      color: white;
      border-color: var(--accent-color);
    }
    
    .btn {
      padding: 12px 24px;
      border: none;
      border-radius: 6px;
      font-size: 1em;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.2s;
    }
    
    .btn:hover {
      transform: translateY(-2px);
      box-shadow: 0 4px 8px var(--shadow);
    }
    
    .btn-primary {
      background: var(--accent-color);
      color: white;
    }
    
    .btn-secondary {
      background: var(--text-secondary);
      color: white;
    }
    
    .btn-success {
      background: var(--success-color);
      color: white;
    }
    
    .btn-danger {
      background: var(--error-color);
      color: white;
    }
    
    .btn-group {
      display: flex;
      gap: 10px;
      margin-top: 20px;
      flex-wrap: wrap;
    }
    
    .btn-group .btn {
      flex: 1;
    }
    
    /* Alert Overlay */
    .alert-overlay {
      display: none;
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: var(--modal-overlay);
      z-index: 2000;
    }
    
    .alert-overlay.show {
      display: flex;
      align-items: center;
      justify-content: center;
    }
    
    .alert-content {
      background: white;
      border-radius: 12px;
      padding: 40px;
      max-width: 500px;
      text-align: center;
      box-shadow: 0 10px 25px rgba(0,0,0,0.3);
      animation: alertPulse 1s infinite;
    }
    
    @keyframes alertPulse {
      0%, 100% { transform: scale(1); }
      50% { transform: scale(1.05); }
    }
    
    .alert-icon {
      font-size: 4em;
      margin-bottom: 20px;
    }
    
    .alert-title {
      font-size: 2em;
      font-weight: 700;
      color: var(--error-color);
      margin-bottom: 20px;
    }
    
    .alert-items {
      font-size: 1.2em;
      margin-bottom: 30px;
      color: #333;
    }
    
    .alert-item {
      padding: 10px;
      margin: 10px 0;
      background: #f3f4f6;
      border-radius: 6px;
    }
    
    .alert-btn {
      padding: 15px 30px;
      font-size: 1.2em;
      font-weight: 700;
      background: var(--error-color);
      color: white;
      border: none;
      border-radius: 8px;
      cursor: pointer;
      transition: all 0.2s;
    }
    
    .alert-btn:hover {
      background: #dc2626;
      transform: scale(1.05);
    }
    
    /* Settings Page */
    .settings-page {
      display: none;
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: linear-gradient(135deg, var(--bg-gradient-start) 0%, var(--bg-gradient-end) 100%);
      z-index: 1500;
      overflow-y: auto;
    }
    
    .settings-page.show {
      display: block;
    }
    
    .settings-header {
      background: var(--card-bg-alpha);
      padding: 20px;
      margin-bottom: 20px;
      position: sticky;
      top: 0;
      z-index: 10;
      box-shadow: 0 2px 10px var(--shadow);
    }
    
    .settings-back {
      background: var(--accent-color);
      color: white;
      border: none;
      padding: 10px 20px;
      border-radius: 6px;
      cursor: pointer;
      font-size: 1em;
      margin-bottom: 10px;
    }
    
    .settings-title {
      font-size: 1.8em;
      font-weight: 700;
      color: var(--accent-color);
    }
    
    .settings-content {
      max-width: 800px;
      margin: 0 auto;
      padding: 0 20px 40px;
    }
    
    .settings-section {
      background: var(--card-bg);
      border-radius: 12px;
      padding: 30px;
      margin-bottom: 20px;
      box-shadow: 0 4px 6px var(--shadow);
    }
    
    .settings-section-title {
      font-size: 1.3em;
      font-weight: 700;
      color: var(--accent-color);
      margin-bottom: 20px;
      padding-bottom: 10px;
      border-bottom: 2px solid var(--border-color);
    }
    
    /* Weather Forecast Modal */
    .weather-forecast-modal {
      display: none;
      position: fixed;
      top: 0;
      left: 0;
      width: 100%;
      height: 100%;
      background: var(--modal-overlay);
      z-index: 1000;
      overflow-y: auto;
      padding: 20px;
    }
    
    .weather-forecast-modal.show {
      display: flex;
      align-items: flex-start;
      justify-content: center;
    }
    
    .weather-forecast-content {
      background: var(--card-bg);
      border-radius: 12px;
      padding: 30px;
      max-width: 900px;
      width: 100%;
      margin: 40px auto;
      box-shadow: 0 10px 25px var(--shadow);
      position: relative;
    }
    
    .weather-forecast-close {
      position: absolute;
      top: 15px;
      right: 15px;
      background: var(--error-color);
      color: white;
      border: none;
      width: 40px;
      height: 40px;
      border-radius: 50%;
      font-size: 1.5em;
      cursor: pointer;
      transition: all 0.2s;
    }
    
    .weather-forecast-close:hover {
      transform: scale(1.1);
    }
    
    /* Slider */
    .slider {
      -webkit-appearance: none;
      width: 100%;
      height: 6px;
      border-radius: 3px;
      background: var(--border-color);
      outline: none;
      transition: background 0.2s;
    }
    
    .slider:hover {
      background: var(--text-tertiary);
    }
    
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--accent-color);
      cursor: pointer;
    }
    
    .slider::-moz-range-thumb {
      width: 20px;
      height: 20px;
      border-radius: 50%;
      background: var(--accent-color);
      cursor: pointer;
      border: none;
    }
    
    .slider-value {
      display: inline-block;
      margin-left: 10px;
      font-weight: 600;
      color: var(--accent-color);
    }
    
    /* Responsive */
    @media (max-width: 768px) {
      .main-content {
        grid-template-columns: 1fr;
      }
      
      .sidebar {
        flex-direction: row;
        flex-wrap: wrap;
      }
      
      .alarm-btn {
        flex: 1;
        min-width: calc(50% - 4px);
      }
      
      .timer-btn, .settings-btn {
        flex: 1;
        min-width: calc(50% - 4px);
      }
      
      .clock-time {
        font-size: 3.5em;
      }
      
      .sensors {
        grid-template-columns: 1fr;
      }
    }
  </style>
</head>
<body>
  <div class="container">
    <!-- Header -->
    <div class="header">
      <div class="header-title">ESP32 Alarm Clock</div>
      <div class="wifi-status">
        <span class="wifi-icon" id="wifiIcon">📶</span>
        <span id="nightModeIndicator" style="display:none; margin-left: 8px;">🌙</span>
        <span id="ipAddress">--</span>
      </div>
    </div>

    <div class="main-content">
      <!-- Dashboard -->
      <div class="dashboard">
        <!-- Clock -->
        <div class="clock">
          <div class="clock-time" id="clockTime">--:--</div>
          <div class="clock-date" id="clockDate">Loading...</div>
          <div class="clock-weekday" id="clockWeekday">---</div>
        </div>

        <!-- Sensors -->
        <div class="sensors">
          <div class="sensor-card">
            <div class="sensor-icon">🌡️</div>
            <div class="sensor-value" id="temperature">--</div>
            <div class="sensor-label" data-translate="temperature">Temperature</div>
          </div>
          <div class="sensor-card">
            <div class="sensor-icon">💧</div>
            <div class="sensor-value" id="humidity">--</div>
            <div class="sensor-label" data-translate="humidity">Humidity</div>
          </div>
          <div class="sensor-card">
            <div class="sensor-icon">🔽</div>
            <div class="sensor-value" id="pressure">--</div>
            <div class="sensor-label" data-translate="pressure">Pressure</div>
          </div>
        </div>

        <!-- Weather -->
        <div class="weather" id="weatherCard" style="display: none; cursor: pointer;" onclick="showWeatherForecast()">
          <div class="weather-title">
            🌤️ <span data-translate="weatherForecast">Weather Forecast</span> (<span class="weather-city" id="weatherCity">--</span>)
          </div>
          <div class="weather-text" id="weatherText">--</div>
        </div>
      </div>

      <!-- Sidebar -->
      <div class="sidebar" id="sidebar">
        <!-- Alarms (generated by JS) -->
        <!-- Timer button -->
        <button class="timer-btn" id="timerBtn" onclick="openTimerModal()">
          <span id="timerLabel">⏱️ <span data-translate="timer">Timer</span></span>
        </button>
        <!-- Settings button -->
        <button class="settings-btn" onclick="openSettings()">⚙️ <span data-translate="settings">Settings</span></button>
      </div>
    </div>
  </div>
  </div> <!-- Close container -->

  <!-- Alarm Modal -->
  <div class="modal" id="alarmModal">
    <div class="modal-content">
      <div class="modal-header" id="alarmModalTitle"><span data-translate="setAlarm">Set Alarm</span></div>
      
      <div class="form-group">
        <label class="form-label" data-translate="time">Time</label>
        <input type="time" class="form-input" id="alarmTime">
      </div>
      
      <div class="form-group">
        <label class="form-label" data-translate="type">Type</label>
        <select class="form-select" id="alarmType" onchange="updateAlarmTypeFields()">
          <option value="daily" data-translate="daily">Daily</option>
          <option value="weekdays" data-translate="weekdays">Weekdays</option>
          <option value="date" data-translate="specificDate">Specific Date</option>
        </select>
      </div>
      
      <div class="form-group" id="alarmWeekdaysGroup" style="display: none;">
        <label class="form-label" data-translate="selectDays">Select Days</label>
        <div class="weekdays" id="alarmWeekdays">
          <button type="button" class="weekday-btn" data-value="1"><span data-translate="mon">Mon</span></button>
          <button type="button" class="weekday-btn" data-value="2"><span data-translate="tue">Tue</span></button>
          <button type="button" class="weekday-btn" data-value="4"><span data-translate="wed">Wed</span></button>
          <button type="button" class="weekday-btn" data-value="8"><span data-translate="thu">Thu</span></button>
          <button type="button" class="weekday-btn" data-value="16"><span data-translate="fri">Fri</span></button>
          <button type="button" class="weekday-btn" data-value="32"><span data-translate="sat">Sat</span></button>
          <button type="button" class="weekday-btn" data-value="64"><span data-translate="sun">Sun</span></button>
        </div>
      </div>
      
      <div class="form-group" id="alarmDateGroup" style="display: none;">
        <label class="form-label" data-translate="date">Date</label>
        <input type="date" class="form-input" id="alarmDate">
      </div>
      
      <div class="form-group">
        <label class="form-label"><span data-translate="text">Text</span> (<span data-translate="optional">optional</span>, <span data-translate="maxChars">max 30 chars</span>)</label>
        <input type="text" class="form-input" id="alarmText" maxlength="30" data-translate-placeholder="wakeUp" placeholder="Wake up!">
      </div>
      
      <div class="form-group">
        <label class="checkbox-group">
          <input type="checkbox" id="alarmRepeat">
          <span data-translate="repeatAfterTrigger">Repeat after trigger (every 10 min)</span>
        </label>
      </div>
      
      <div class="form-group">
        <label class="checkbox-group">
          <input type="checkbox" id="alarmSave">
          <span data-translate="saveToNVS">Save to NVS (survive reboot)</span>
        </label>
      </div>
      
      <div class="btn-group">
        <button class="btn btn-primary" onclick="saveAlarm()"><span data-translate="save">Save</span></button>
        <button class="btn btn-secondary" onclick="closeAlarmModal()"><span data-translate="cancel">Cancel</span></button>
      </div>
      
      <div class="btn-group" id="alarmEditButtons" style="display: none;">
        <button class="btn btn-danger" onclick="deleteAlarm()"><span data-translate="delete">Delete</span></button>
        <button class="btn btn-secondary" id="toggleAlarmBtn" onclick="toggleAlarm()"><span data-translate="disable">Disable</span></button>
      </div>
    </div>
  </div>

  <!-- Timer Modal -->
  <div class="modal" id="timerModal">
    <div class="modal-content">
      <div class="modal-header"><span data-translate="timer">Timer</span></div>
      
      <div class="form-group">
        <label class="form-label" data-translate="duration">Duration</label>
        <input type="time" class="form-input" id="timerDuration" value="00:01:00" step="1">
      </div>
      
      <div class="form-group">
        <label class="form-label"><span data-translate="text">Text</span> (<span data-translate="optional">optional</span>)</label>
        <input type="text" class="form-input" id="timerText" maxlength="30" placeholder="Timer">
      </div>
      
      <div class="btn-group">
        <button class="btn btn-success" onclick="startTimer()"><span data-translate="start">Start</span></button>
        <button class="btn btn-primary" onclick="addTimerMinute()"><span data-translate="addMinute">+1 min</span></button>
        <button class="btn btn-danger" onclick="stopTimer()"><span data-translate="stop">Stop</span></button>
        <button class="btn btn-secondary" onclick="closeTimerModal()"><span data-translate="close">Close</span></button>
      </div>
    </div>
  </div>

  <!-- Alert Overlay for Ringing Alarms/Timer -->
  <div class="alert-overlay" id="alertOverlay">
    <div class="alert-content">
      <div class="alert-icon">🔔</div>
      <div class="alert-title" id="alertTitle">ALARM RINGING!</div>
      <div class="alert-items" id="alertItems"></div>
      <button class="alert-btn" onclick="stopAllAlerts()">STOP ALL</button>
    </div>
  </div>

  <!-- Settings Page -->
  <div class="settings-page" id="settingsPage">
    <div class="settings-header">
      <button class="settings-back" onclick="closeSettings()">← <span data-translate="backToDashboard">Back to Dashboard</span></button>
      <div class="settings-title" data-translate="settingsTitle">Settings</div>
    </div>
    
    <div class="settings-content">
      <!-- Time Settings -->
      <div class="settings-section">
        <div class="settings-section-title" data-translate="timeSettings">Time Settings</div>
        
        <div class="form-group">
          <label class="form-label" data-translate="ntpServer">NTP Server</label>
          <input type="text" class="form-input" id="settingsNtp" placeholder="pool.ntp.org">
        </div>
        
        <div class="form-group">
          <label class="form-label" data-translate="timezone">Timezone (GMT offset in hours)</label>
          <input type="number" class="form-input" id="settingsTimezone" min="-12" max="14" step="1" placeholder="3">
        </div>
        
        <div class="form-group">
          <label class="form-label" data-translate="dstOffset">DST Offset (hours)</label>
          <input type="number" class="form-input" id="settingsDst" min="-2" max="2" step="1" placeholder="0">
        </div>
        
        <div class="form-group">
          <label class="form-label" data-translate="manualTime">Manual Time (YYYY-MM-DD HH:MM:SS)</label>
          <input type="text" class="form-input" id="settingsManualTime" placeholder="2026-01-24 14:30:00">
        </div>
        
        <div class="btn-group">
          <button class="btn btn-primary" onclick="syncTimeNow()"><span data-translate="syncTimeNow">Sync Time Now</span></button>
          <button class="btn btn-secondary" onclick="setManualTime()"><span data-translate="setManualTime">Set Manual Time</span></button>
        </div>
      </div>

      <!-- Display Settings -->
      <div class="settings-section">
        <div class="settings-section-title" data-translate="displaySettings">Display Settings</div>
        
        <div class="form-group">
          <label class="form-label">
            <span data-translate="displayBrightness">Display Brightness</span>
            <span class="slider-value" id="brightnessValue">255</span>
          </label>
          <input type="range" class="slider" id="settingsBrightness" min="0" max="255" step="5" value="255" oninput="updateBrightnessValue(this.value)">
        </div>
      </div>

      <!-- Night Mode Settings -->
      <div class="settings-section">
        <div class="settings-section-title" data-translate="nightModeSettings">Night Mode</div>
        
        <div class="form-group">
          <label class="checkbox-group">
            <input type="checkbox" id="nightModeEnabled">
            <span data-translate="enableNightMode">Enable Night Mode</span>
          </label>
        </div>
        
        <div class="form-group">
          <label class="form-label" data-translate="startTime">Start Time</label>
          <input type="time" class="form-input" id="nightStartTime" value="23:00">
        </div>
        
        <div class="form-group">
          <label class="form-label" data-translate="endTime">End Time</label>
          <input type="time" class="form-input" id="nightEndTime" value="07:00">
        </div>
        
        <div class="form-group">
          <label class="form-label">
            <span data-translate="nightBrightness">Night Brightness</span>
            <span class="slider-value" id="nightBrightnessValue">10</span>
          </label>
          <input type="range" class="slider" id="nightBrightness" min="0" max="255" step="5" value="10" oninput="document.getElementById('nightBrightnessValue').textContent = this.value">
        </div>
        
        <button class="btn btn-primary" onclick="saveNightMode()"><span data-translate="saveNightMode">Save Night Mode</span></button>
      </div>

      <!-- Weather Settings -->
      <div class="settings-section">
        <div class="settings-section-title" data-translate="weatherSettings">Weather Settings</div>
        
        <div class="form-group">
          <label class="form-label" data-translate="city">City</label>
          <input type="text" class="form-input" id="settingsCity" placeholder="Moscow">
        </div>
        
        <button class="btn btn-primary" onclick="updateWeatherNow()"><span data-translate="updateWeatherNow">Update Weather Now</span></button>
      </div>

      <!-- Theme & Language Settings -->
      <div class="settings-section">
        <div class="settings-section-title" data-translate="themeLanguage">Theme & Language</div>
        
        <div class="form-group">
          <label class="form-label" data-translate="theme">Theme</label>
          <select class="form-select" id="themeSelect" onchange="changeTheme(this.value)">
            <option value="light" data-translate="lightTheme">Light</option>
            <option value="dark" data-translate="darkTheme">Dark</option>
          </select>
        </div>
        
        <div class="form-group">
          <label class="form-label" data-translate="language">Language</label>
          <select class="form-select" id="languageSelect" onchange="changeLanguage(this.value)">
            <option value="en">English</option>
            <option value="ru">Русский</option>
          </select>
        </div>
        
        <button class="btn btn-primary" onclick="saveThemeLanguage()"><span data-translate="saveSettings">Save Settings</span></button>
      </div>

      <!-- Melody Settings -->
      <div class="settings-section" id="melodySettings" style="display: none;">
        <div class="settings-section-title" data-translate="melodySettings">Melodies (Passive Buzzer)</div>
        
        <div class="form-group">
          <label class="form-label" data-translate="alarmMelody">Alarm Melody (RTTTL format)</label>
          <textarea class="form-textarea" id="settingsAlarmMelody" rows="3" placeholder="C5 Q D5 Q E5 H..."></textarea>
          <small style="color: var(--text-secondary);"><span data-translate="melodyFormatInfo">Format: Note+Octave Duration (e.g., C5 Q = C octave 5 quarter note)</span></small><br>
          <small style="color: var(--text-secondary);"><span data-translate="melodyNotesInfo">Notes: C C# D D# E F F# G G# A A# B P(pause) | Octaves: 3-7</span></small><br>
          <small style="color: var(--text-secondary);"><span data-translate="melodyDurationsInfo">Durations: W(whole) H(half) Q(quarter) E(eighth) S(sixteenth)</span></small>
        </div>
        
        <button class="btn btn-secondary" onclick="testMelody('alarm')" style="width: 100%; margin-bottom: 10px;"><span data-translate="testAlarmMelody">Test Alarm Melody</span></button>
        
        <div class="form-group">
          <label class="form-label" data-translate="timerMelody">Timer Melody (RTTTL format)</label>
          <textarea class="form-textarea" id="settingsTimerMelody" rows="3" placeholder="C5 E C5 E P E..."></textarea>
        </div>
        
        <button class="btn btn-secondary" onclick="testMelody('timer')" style="width: 100%; margin-bottom: 10px;"><span data-translate="testTimerMelody">Test Timer Melody</span></button>
        
        <button class="btn btn-success" onclick="saveMelodies()" style="width: 100%;"><span data-translate="saveBothMelodies">Save Both Melodies</span></button>
      </div>

      <!-- WiFi Mode Settings -->
      <div class="settings-section">
        <div class="settings-section-title" data-translate="wifiPowerSaving">WiFi Power Saving</div>
        
        <div class="form-group">
          <label class="form-label" data-translate="wifiMode">WiFi Mode</label>
          <select class="form-select" id="wifiModeSelect">
            <option value="0" data-translate="alwaysOn">Always On (no power saving)</option>
            <option value="1" data-translate="smartWifi">Smart WiFi (recommended)</option>
          </select>
        </div>
        
        <div id="smartWifiSettings">
          <div class="form-group">
            <label class="form-label" data-translate="autoOffAfter">Auto-off after (minutes)</label>
            <input type="number" class="form-input" id="wifiAutoOff" min="1" max="60" value="10">
            <small style="color: var(--text-secondary);"><span data-translate="wifiAutoOffInfo">WiFi turns off after this many minutes of inactivity</span></small>
          </div>
          
          <div class="form-group">
            <label class="form-label" data-translate="ntpSyncInterval">NTP Sync interval (minutes)</label>
            <input type="number" class="form-input" id="ntpSyncInterval" min="15" max="1440" value="60">
            <small style="color: var(--text-secondary);"><span data-translate="ntpSyncInfo">How often to sync time from NTP server</span></small>
          </div>
          
          <div class="form-group">
            <label class="form-label" data-translate="weatherSyncInterval">Weather update interval (minutes)</label>
            <input type="number" class="form-input" id="weatherSyncInterval" min="15" max="1440" value="30">
            <small style="color: var(--text-secondary);"><span data-translate="weatherSyncInfo">How often to update weather forecast</span></small>
          </div>
          
          <div style="background: #fef3c7; padding: 12px; border-radius: 6px; margin-top: 12px;">
            <strong>💡 <span data-translate="tipLabel">Tip:</span></strong> <span data-translate="bootButtonTip">Hold BOOT button for 3 seconds to manually enable WiFi for 10 minutes</span>
          </div>
        </div>
        
        <button class="btn btn-primary" onclick="saveWiFiMode()" style="margin-top: 12px;"><span data-translate="saveWifiSettings">Save WiFi Settings</span></button>
      </div>

      <!-- System Actions -->
      <div class="settings-section">
        <div class="settings-section-title" data-translate="systemSettings">System Settings</div>
        
        <div class="btn-group">
          <button class="btn btn-primary" onclick="saveAllSettings()"><span data-translate="saveAllSettings">Save All Settings</span></button>
          <button class="btn btn-secondary" onclick="restoreSettings()"><span data-translate="restoreSettings">Restore from NVS</span></button>
        </div>
        
        <div class="btn-group">
          <button class="btn btn-danger" onclick="eraseNVS()"><span data-translate="eraseNVS">Erase NVS</span></button>
          <button class="btn btn-danger" onclick="rebootDevice()"><span data-translate="rebootDevice">Reboot Device</span></button>
        </div>
      </div>
    </div>
  </div>

)rawliteral";

const char html_part2[] PROGMEM = R"rawliteral(
  <script>
    let currentAlarmIndex = -1;
    let statusData = {};
    let currentLanguage = 'en';
    let currentTheme = 'light';
    
    // Translations object
    const translations = {
      en: {
        // Dashboard
        clockTitle: 'ESP32 Clock',
        connected: 'Connected',
        disconnected: 'Disconnected',
        nightMode: 'Night Mode',
        
        // Sensors
        temperature: 'Temperature',
        humidity: 'Humidity',
        pressure: 'Pressure',
        
        // Weather
        weatherIn: 'Weather in',
        weatherForecast: 'Weather Forecast for',
        loading: 'Loading...',
        errorLoading: 'Failed to load forecast',
        tryUpdating: 'Try updating your weather location in settings.',
        currentWeather: 'Current Weather',
        feelsLike: 'Feels like',
        wind: 'Wind',
        precipitation: 'Precipitation',
        visibility: 'Visibility',
        max: 'Max',
        min: 'Min',
        location: 'Location',
        lastUpdated: 'Last updated',
        dataSource: 'Data source',
        
        // Alarms & Timer
        alarm: 'Alarm',
        timer: 'Timer',
        settings: 'Settings',
        alarmRinging: 'ALARM RINGING!',
        timerFinished: 'TIMER FINISHED!',
        stopAll: 'STOP ALL',
        
        // Alarm modal
        newAlarm: 'New Alarm',
        editAlarm: 'Edit Alarm',
        setAlarm: 'Set Alarm',
        time: 'Time',
        type: 'Type',
        daily: 'Daily',
        weekdays: 'Weekdays',
        specificDate: 'Specific Date',
        selectDays: 'Select Days',
        date: 'Date',
        repeat: 'Repeat',
        repeatAfterTrigger: 'Repeat after trigger (every 10 min)',
        saveToNVS: 'Save to NVS (survive reboot)',
        text: 'Text',
        optional: 'optional',
        maxChars: 'max 30 chars',
        save: 'Save',
        cancel: 'Cancel',
        delete: 'Delete',
        disable: 'Disable',
        enable: 'Enable',
        wakeUp: 'Wake up!',
        
        // Days of week
        monday: 'Monday',
        tuesday: 'Tuesday',
        wednesday: 'Wednesday',
        thursday: 'Thursday',
        friday: 'Friday',
        saturday: 'Saturday',
        sunday: 'Sunday',
        
        // Days short
        mon: 'Mon',
        tue: 'Tue',
        wed: 'Wed',
        thu: 'Thu',
        fri: 'Fri',
        sat: 'Sat',
        sun: 'Sun',
        
        // Alarm modal
        newAlarm: 'New Alarm',
        editAlarm: 'Edit Alarm',
        setAlarm: 'Set Alarm',
        time: 'Time',
        type: 'Type',
        daily: 'Daily',
        weekdays: 'Weekdays',
        specificDate: 'Specific Date',
        selectDays: 'Select Days',
        date: 'Date',
        repeat: 'Repeat',
        repeatAfterTrigger: 'Repeat after trigger (every 10 min)',
        saveToNVS: 'Save to NVS (survive reboot)',
        text: 'Text',
        optional: 'optional',
        maxChars: 'max 30 chars',
        save: 'Save',
        cancel: 'Cancel',
        delete: 'Delete',
        disable: 'Disable',
        enable: 'Enable',
        wakeUp: 'Wake up!',
        
        // Timer modal
        duration: 'Duration',
        start: 'Start',
        stop: 'Stop',
        close: 'Close',
        addMinute: '+1 min',
        
        // Settings
        settingsTitle: 'Settings',
        backToDashboard: 'Back to Dashboard',
        timeSettings: 'Time Settings',
        ntpServer: 'NTP Server',
        timezone: 'Timezone (GMT offset in hours)',
        dstOffset: 'DST Offset (hours)',
        manualTime: 'Manual Time (YYYY-MM-DD HH:MM:SS)',
        syncTimeNow: 'Sync Time Now',
        setManualTime: 'Set Manual Time',
        
        displaySettings: 'Display Settings',
        displayBrightness: 'Display Brightness',
        
        nightModeSettings: 'Night Mode',
        enableNightMode: 'Enable Night Mode',
        startTime: 'Start Time',
        endTime: 'End Time',
        nightBrightness: 'Night Brightness',
        saveNightMode: 'Save Night Mode',
        
        weatherSettings: 'Weather Settings',
        city: 'City',
        updateWeatherNow: 'Update Weather Now',
        
        themeLanguage: 'Theme & Language',
        theme: 'Theme',
        lightTheme: 'Light',
        darkTheme: 'Dark',
        language: 'Language',
        saveSettings: 'Save Settings',
        
        melodySettings: 'Melodies (Passive Buzzer)',
        alarmMelody: 'Alarm Melody (RTTTL format)',
        timerMelody: 'Timer Melody (RTTTL format)',
        testAlarmMelody: 'Test Alarm Melody',
        testTimerMelody: 'Test Timer Melody',
        saveBothMelodies: 'Save Both Melodies',
        melodyFormatInfo: 'Format: Note+Octave Duration (e.g., C5 Q = C octave 5 quarter note)',
        melodyNotesInfo: 'Notes: C C# D D# E F F# G G# A A# B P(pause) | Octaves: 3-7',
        melodyDurationsInfo: 'Durations: W(whole) H(half) Q(quarter) E(eighth) S(sixteenth)',
        
        wifiPowerSaving: 'WiFi Power Saving',
        wifiMode: 'WiFi Mode',
        alwaysOn: 'Always On (no power saving)',
        smartWifi: 'Smart WiFi (recommended)',
        autoOffAfter: 'Auto-off after (minutes)',
        ntpSyncInterval: 'NTP Sync interval (minutes)',
        weatherSyncInterval: 'Weather update interval (minutes)',
        saveWifiSettings: 'Save WiFi Settings',
        wifiAutoOffInfo: 'WiFi turns off after this many minutes of inactivity',
        ntpSyncInfo: 'How often to sync time from NTP server',
        weatherSyncInfo: 'How often to update weather forecast',
        tipLabel: 'Tip:',
        bootButtonTip: 'Hold BOOT button for 3 seconds to manually enable WiFi for 10 minutes',
        
        systemSettings: 'System Settings',
        saveAllSettings: 'Save All Settings',
        restoreSettings: 'Restore from NVS',
        eraseNVS: 'Erase NVS',
        rebootDevice: 'Reboot Device'
      },
      
      ru: {
        // Панель управления
        clockTitle: 'Часы ESP32',
        connected: 'Подключено',
        disconnected: 'Отключено',
        nightMode: 'Ночной режим',
        
        // Датчики
        temperature: 'Температура',
        humidity: 'Влажность',
        pressure: 'Давление',
        
        // Погода
        weatherIn: 'Погода в',
        weatherForecast: 'Прогноз погоды для',
        loading: 'Загрузка...',
        errorLoading: 'Не удалось загрузить прогноз',
        tryUpdating: 'Попробуйте обновить местоположение в настройках.',
        currentWeather: 'Текущая погода',
        feelsLike: 'Ощущается как',
        wind: 'Ветер',
        precipitation: 'Осадки',
        visibility: 'Видимость',
        max: 'Макс',
        min: 'Мин',
        location: 'Местоположение',
        lastUpdated: 'Обновлено',
        dataSource: 'Источник данных',
        
        // Будильники и таймер
        alarm: 'Будильник',
        timer: 'Таймер',
        settings: 'Настройки',
        alarmRinging: 'ЗВОНИТ БУДИЛЬНИК!',
        timerFinished: 'ТАЙМЕР ЗАВЕРШЁН!',
        stopAll: 'ОСТАНОВИТЬ ВСЁ',
        
        // Дни недели
        monday: 'Понедельник',
        tuesday: 'Вторник',
        wednesday: 'Среда',
        thursday: 'Четверг',
        friday: 'Пятница',
        saturday: 'Суббота',
        sunday: 'Воскресенье',
        
        // Дни короткие
        mon: 'Пн',
        tue: 'Вт',
        wed: 'Ср',
        thu: 'Чт',
        fri: 'Пт',
        sat: 'Сб',
        sun: 'Вс',
        
        // Модальное окно будильника
        newAlarm: 'Новый будильник',
        editAlarm: 'Редактировать будильник',
        setAlarm: 'Установить будильник',
        time: 'Время',
        type: 'Тип',
        daily: 'Ежедневно',
        weekdays: 'По будням',
        specificDate: 'Конкретная дата',
        selectDays: 'Выберите дни',
        date: 'Дата',
        repeat: 'Повтор',
        repeatAfterTrigger: 'Повторять после срабатывания (каждые 10 мин)',
        saveToNVS: 'Сохранить в NVS (сохранится после перезагрузки)',
        text: 'Текст',
        optional: 'опционально',
        maxChars: 'макс 30 символов',
        save: 'Сохранить',
        cancel: 'Отмена',
        delete: 'Удалить',
        disable: 'Отключить',
        enable: 'Включить',
        wakeUp: 'Просыпайся!',
        
        // Модальное окно таймера
        duration: 'Длительность',
        start: 'Запустить',
        stop: 'Остановить',
        close: 'Закрыть',
        addMinute: '+1 мин',
        
        // Настройки
        settingsTitle: 'Настройки',
        backToDashboard: 'Назад к панели',
        timeSettings: 'Настройки времени',
        ntpServer: 'NTP сервер',
        timezone: 'Часовой пояс (смещение GMT в часах)',
        dstOffset: 'Летнее время (часы)',
        manualTime: 'Ручная установка времени (ГГГГ-ММ-ДД ЧЧ:ММ:СС)',
        syncTimeNow: 'Синхронизировать время',
        setManualTime: 'Установить время вручную',
        
        displaySettings: 'Настройки дисплея',
        displayBrightness: 'Яркость дисплея',
        
        nightModeSettings: 'Ночной режим',
        enableNightMode: 'Включить ночной режим',
        startTime: 'Время начала',
        endTime: 'Время окончания',
        nightBrightness: 'Яркость ночью',
        saveNightMode: 'Сохранить ночной режим',
        
        weatherSettings: 'Настройки погоды',
        city: 'Город',
        updateWeatherNow: 'Обновить погоду сейчас',
        
        themeLanguage: 'Тема и язык',
        theme: 'Тема',
        lightTheme: 'Светлая',
        darkTheme: 'Тёмная',
        language: 'Язык',
        saveSettings: 'Сохранить настройки',
        
        melodySettings: 'Мелодии (пассивный зуммер)',
        alarmMelody: 'Мелодия будильника (формат RTTTL)',
        timerMelody: 'Мелодия таймера (формат RTTTL)',
        testAlarmMelody: 'Тест мелодии будильника',
        testTimerMelody: 'Тест мелодии таймера',
        saveBothMelodies: 'Сохранить обе мелодии',
        melodyFormatInfo: 'Формат: Нота+Октава Длительность (напр., C5 Q = До 5-й октавы четвертная нота)',
        melodyNotesInfo: 'Ноты: C C# D D# E F F# G G# A A# B P(пауза) | Октавы: 3-7',
        melodyDurationsInfo: 'Длительности: W(целая) H(половинная) Q(четвертная) E(восьмая) S(шестнадцатая)',
        
        wifiPowerSaving: 'Энергосбережение WiFi',
        wifiMode: 'Режим WiFi',
        alwaysOn: 'Всегда включен',
        smartWifi: 'Умный WiFi (рекомендуется)',
        autoOffAfter: 'Автовыключение через (минуты)',
        ntpSyncInterval: 'Интервал синхронизации NTP (минуты)',
        weatherSyncInterval: 'Интервал обновления погоды (минуты)',
        saveWifiSettings: 'Сохранить настройки WiFi',
        wifiAutoOffInfo: 'WiFi отключается после этого количества минут бездействия',
        ntpSyncInfo: 'Как часто синхронизировать время с NTP сервером',
        weatherSyncInfo: 'Как часто обновлять прогноз погоды',
        tipLabel: 'Совет:',
        bootButtonTip: 'Удерживайте кнопку BOOT 3 секунды для включения WiFi на 10 минут',
        
        systemSettings: 'Системные настройки',
        saveAllSettings: 'Сохранить все настройки',
        restoreSettings: 'Восстановить из NVS',
        eraseNVS: 'Очистить NVS',
        rebootDevice: 'Перезагрузить устройство'
      }
    };
    
    // Load saved preferences
    function loadPreferences() {
      const savedTheme = localStorage.getItem('theme') || 'light';
      const savedLang = localStorage.getItem('language') || 'en';
      
      currentTheme = savedTheme;
      currentLanguage = savedLang;
      
      applyTheme(savedTheme);
      applyLanguage(savedLang);
    }
    
    // Apply theme
    function applyTheme(theme) {
      if (theme === 'dark') {
        document.body.classList.add('dark-theme');
      } else {
        document.body.classList.remove('dark-theme');
      }
      
      const themeSelect = document.getElementById('themeSelect');
      if (themeSelect) {
        themeSelect.value = theme;
      }
    }
    
    // Apply language
    function applyLanguage(lang) {
      // Update all elements with data-translate attribute
      document.querySelectorAll('[data-translate]').forEach(element => {
        const key = element.getAttribute('data-translate');
        if (translations[lang] && translations[lang][key]) {
          if (element.tagName === 'INPUT' && element.type === 'text') {
            element.placeholder = translations[lang][key];
          } else if (element.tagName === 'OPTION') {
            element.textContent = translations[lang][key];
          } else {
            element.textContent = translations[lang][key];
          }
        }
      });
      
      // Update placeholders with data-translate-placeholder attribute
      document.querySelectorAll('[data-translate-placeholder]').forEach(element => {
        const key = element.getAttribute('data-translate-placeholder');
        if (translations[lang] && translations[lang][key]) {
          element.placeholder = translations[lang][key];
        }
      });
      
      const langSelect = document.getElementById('languageSelect');
      if (langSelect) {
        langSelect.value = lang;
      }
      
      // Update toggle button text based on alarm state
      updateToggleAlarmButton();
    }
    
    // Change theme
    function changeTheme(theme) {
      currentTheme = theme;
      applyTheme(theme);
    }
    
    // Change language
    function changeLanguage(lang) {
      currentLanguage = lang;
      applyLanguage(lang);
    }
    
    function saveThemeLanguage() {
      localStorage.setItem('theme', currentTheme);
      localStorage.setItem('language', currentLanguage);
      
      fetch('/preferences/set?theme=' + encodeURIComponent(currentTheme) + 
            '&language=' + encodeURIComponent(currentLanguage))
        .then(r => r.text())
        .then(msg => alert(msg))
        .catch(err => alert('Error: ' + err));
    }
    
    function updateStatus() {
      fetch('/status')
        .then(r => r.json())
        .then(data => {
          statusData = data;
          
          // ИСПРАВЛЕНИЕ: Безопасный доступ к данным
          document.getElementById('clockTime').textContent = data.time || '--:--';
          document.getElementById('clockDate').textContent = data.date || 'Loading...';
          document.getElementById('clockWeekday').textContent = data.weekday || '---';
          
          // WiFi status
          const wifiIcon = document.getElementById('wifiIcon');
          const ipAddress = document.getElementById('ipAddress');
          if (data.wifi === 'Connected') {
            wifiIcon.textContent = '📶';
            ipAddress.textContent = data.ip || '--';
          } else {
            wifiIcon.textContent = '📵';
            ipAddress.textContent = data.wifi || 'Disconnected';
          }
          
          // Night mode indicator
          const nightIndicator = document.getElementById('nightModeIndicator');
          if (data.nightMode && data.nightMode.active) {
            nightIndicator.style.display = 'inline';
          } else {
            nightIndicator.style.display = 'none';
          }
          
          // Sensors with null checks
          if (data.sensors) {
            const tempEl = document.getElementById('temperature');
            if (tempEl && typeof data.sensors.temperature === 'number') {
              tempEl.textContent = data.sensors.temperature.toFixed(1) + '°C';
              
              // Apply temperature color classes
              const tempCard = tempEl.closest('.sensor-card');
              tempCard.classList.remove('temp-cold', 'temp-normal', 'temp-warm', 'temp-hot');
              if (data.sensors.temperature < 15) {
                tempCard.classList.add('temp-cold');
              } else if (data.sensors.temperature < 22) {
                tempCard.classList.add('temp-normal');
              } else if (data.sensors.temperature < 28) {
                tempCard.classList.add('temp-warm');
              } else {
                tempCard.classList.add('temp-hot');
              }
            }
            
            const humEl = document.getElementById('humidity');
            if (humEl && typeof data.sensors.humidity === 'number') {
              humEl.textContent = data.sensors.humidity.toFixed(1) + '%';
              
              // Apply humidity color classes
              const humCard = humEl.closest('.sensor-card');
              humCard.classList.remove('humidity-low', 'humidity-normal', 'humidity-high');
              if (data.sensors.humidity < 30) {
                humCard.classList.add('humidity-low');
              } else if (data.sensors.humidity < 60) {
                humCard.classList.add('humidity-normal');
              } else {
                humCard.classList.add('humidity-high');
              }
            }
            
            const pressEl = document.getElementById('pressure');
            if (pressEl && typeof data.sensors.pressure === 'number') {
              pressEl.textContent = data.sensors.pressure.toFixed(1) + ' hPa';
              
              // Apply pressure color classes
              const pressCard = pressEl.closest('.sensor-card');
              pressCard.classList.remove('pressure-high', 'pressure-normal', 'pressure-low');
              if (data.sensors.pressure > 1020) {
                pressCard.classList.add('pressure-high');
              } else if (data.sensors.pressure > 1000) {
                pressCard.classList.add('pressure-normal');
              } else {
                pressCard.classList.add('pressure-low');
              }
            }
          }
          
          // Weather with null check
          if (data.weather && data.city) {
            const weatherCard = document.getElementById('weatherCard');
            const cityEl = document.getElementById('weatherCity');
            const textEl = document.getElementById('weatherText');
            
            if (weatherCard && cityEl && textEl) {
              weatherCard.style.display = 'block';
              if (cityEl) cityEl.textContent = data.city;
              if (textEl) textEl.textContent = data.weather;
            } else {
              weatherCard.style.display = 'none';
            }
          }
          
          // Update alarms - с проверкой на массив
          if (Array.isArray(data.alarms)) {
            updateAlarmButtons(data.alarms);
          } else {
            updateAlarmButtons([]);
          }
          
          // Update timer - с проверкой на объект
          updateTimerButton(data.timer || {});
          
          // Check for ringing alarms/timer
          checkRingingAlerts(data);
        })
        .catch(err => {
          // ИСПРАВЛЕНИЕ: Улучшенная обработка ошибок сети
          console.error('Status update error:', err);
          // Не показываем alert, чтобы не раздражать пользователя
          // при временных проблемах с сетью
        });
    }

    function updateAlarmButtons(alarms) {
      const sidebar = document.getElementById('sidebar');
      
      // Remove old alarm buttons
      const oldButtons = sidebar.querySelectorAll('.alarm-btn');
      oldButtons.forEach(btn => btn.remove());
      
      // Add alarm buttons
      for (let i = 0; i < 10; i++) {
        const alarm = alarms[i] || { active: false, ringing: false };
        const btn = document.createElement('button');
        btn.className = 'alarm-btn';
        btn.onclick = () => openAlarmModal(i);
        
        if (alarm.ringing) {
          btn.classList.add('ringing');
          btn.innerHTML = `⏰ ${i + 1} 🔔<br>${formatTime(alarm.hour, alarm.minute)}`;
        } else if (alarm.active) {
          btn.classList.add('active');
          btn.innerHTML = `⏰ ${i + 1}<br>${formatTime(alarm.hour, alarm.minute)}`;
        } else {
          btn.classList.add('inactive');
          btn.innerHTML = `⏰ ${i + 1}`;
        }
        
        sidebar.insertBefore(btn, document.getElementById('timerBtn'));
      }
    }

    function updateTimerButton(timer) {
      const btn = document.getElementById('timerBtn');
      const label = document.getElementById('timerLabel');
      
      if (timer.ringing) {
        btn.className = 'timer-btn ringing';
        label.textContent = '⏱️ 🔔';
      } else if (timer.active) {
        btn.className = 'timer-btn active';
        const remaining = timer.remaining || 0;
        const hours = Math.floor(remaining / 3600);
        const minutes = Math.floor((remaining % 3600) / 60);
        const seconds = remaining % 60;
        if (hours > 0) {
          label.textContent = `⏱️ ${hours}:${pad(minutes)}:${pad(seconds)}`;
        } else {
          label.textContent = `⏱️ ${minutes}:${pad(seconds)}`;
        }
)rawliteral";

const char html_part3[] PROGMEM = R"rawliteral(
      } else {
        btn.className = 'timer-btn';
        label.innerHTML = '⏱️ <span data-translate="timer">' + (translations[currentLanguage]?.timer || 'Timer') + '</span>';
      }
    }

    function checkRingingAlerts(data) {
      const alarms = data.alarms || [];
      const timer = data.timer || {};
      
      const ringingAlarms = alarms.filter(a => a.ringing);
      const timerRinging = timer.ringing;
      
      if (timerRinging || ringingAlarms.length > 0) {
        showAlertOverlay(ringingAlarms, timerRinging, timer);
      } else {
        hideAlertOverlay();
      }
    }

    function showAlertOverlay(alarms, timerRinging, timer) {
      const overlay = document.getElementById('alertOverlay');
      const title = document.getElementById('alertTitle');
      const items = document.getElementById('alertItems');
      
      if (timerRinging) {
        title.textContent = 'TIMER FINISHED!';
        items.innerHTML = `<div class="alert-item">${timer.text || 'Timer'}</div>`;
      } else {
        title.textContent = 'ALARM RINGING!';
        items.innerHTML = alarms.map(a => 
          `<div class="alert-item">⏰ ${a.index + 1}: ${formatTime(a.hour, a.minute)} ${a.text ? '"' + a.text + '"' : ''}</div>`
        ).join('');
      }
      
      overlay.classList.add('show');
    }

    function hideAlertOverlay() {
      document.getElementById('alertOverlay').classList.remove('show');
    }

    function stopAllAlerts() {
      // Check if timer is ringing first (priority)
      if (statusData.timer && statusData.timer.ringing) {
        fetch('/timer/clear')
          .then(() => {
            // After stopping timer, check if there are ringing alarms
            if (statusData.alarms && statusData.alarms.some(a => a.ringing)) {
              fetch('/alarm/stop').then(() => updateStatus());
            } else {
              updateStatus();
            }
          });
      } else {
        // Just stop alarms
        fetch('/alarm/stop').then(() => updateStatus());
      }
    }

    function updateSettingsFields(data) {
      document.getElementById('settingsNtp').value = data.ntpServer || 'pool.ntp.org';
      document.getElementById('settingsTimezone').value = data.timezone || 3;
      document.getElementById('settingsDst').value = data.dstOffset || 0;
      document.getElementById('settingsCity').value = data.city || 'Moscow';
      document.getElementById('settingsBrightness').value = data.brightness || 255;
      updateBrightnessValue(data.brightness || 255);
      
      // Show melody settings if passive buzzer
      if (data.buzzerType === 'passive') {
        document.getElementById('melodySettings').style.display = 'block';
        document.getElementById('settingsAlarmMelody').value = data.alarmMelody || '';
        document.getElementById('settingsTimerMelody').value = data.timerMelody || '';
      }
      
      // Night mode settings
      if (data.nightMode) {
        document.getElementById('nightModeEnabled').checked = data.nightMode.enabled || false;
        document.getElementById('nightStartTime').value = 
          `${pad(data.nightMode.startHour || 23)}:${pad(data.nightMode.startMinute || 0)}`;
        document.getElementById('nightEndTime').value = 
          `${pad(data.nightMode.endHour || 7)}:${pad(data.nightMode.endMinute || 0)}`;
        document.getElementById('nightBrightness').value = data.nightMode.brightness || 10;
        document.getElementById('nightBrightnessValue').textContent = data.nightMode.brightness || 10;
      }
      
      // WiFi mode settings
      if (data.wifiMode) {
        document.getElementById('wifiModeSelect').value = data.wifiMode.mode || 0;
        document.getElementById('wifiAutoOff').value = (data.wifiMode.autoOffTimeout || 600) / 60;
        document.getElementById('ntpSyncInterval').value = (data.wifiMode.ntpSyncInterval || 3600) / 60;
        document.getElementById('weatherSyncInterval').value = (data.wifiMode.weatherSyncInterval || 1800) / 60;
      }
      
      // Theme and language
      if (data.theme) {
        currentTheme = data.theme;
        applyTheme(data.theme);
      }
      if (data.language) {
        currentLanguage = data.language;
        applyLanguage(data.language);
      }
    }

    // Alarm functions
    function openAlarmModal(index) {
      currentAlarmIndex = index;
      const modal = document.getElementById('alarmModal');
      const alarm = statusData.alarms ? statusData.alarms[index] : null;
      
      if (alarm && alarm.active) {
        document.getElementById('alarmModalTitle').innerHTML = '<span data-translate="editAlarm">' + (translations[currentLanguage]?.editAlarm || 'Edit Alarm') + '</span> ' + (index + 1);
        document.getElementById('alarmTime').value = formatTime(alarm.hour, alarm.minute);
        document.getElementById('alarmText').value = alarm.text || '';
        document.getElementById('alarmRepeat').checked = alarm.repeat || false;
        document.getElementById('alarmSave').checked = alarm.saved || false;
        
        if (alarm.type === 'date') {
          document.getElementById('alarmType').value = 'date';
          document.getElementById('alarmDate').value = alarm.date || '';
        } else if (alarm.type === 'weekdays') {
          document.getElementById('alarmType').value = 'weekdays';
          updateWeekdaySelection(alarm.weekdays || 0);
        } else {
          document.getElementById('alarmType').value = 'daily';
        }
        
        document.getElementById('alarmEditButtons').style.display = 'flex';
      } else {
        document.getElementById('alarmModalTitle').innerHTML = '<span data-translate="newAlarm">' + (translations[currentLanguage]?.newAlarm || 'New Alarm') + '</span> ' + (index + 1);
        document.getElementById('alarmTime').value = '07:00';
        document.getElementById('alarmText').value = '';
        document.getElementById('alarmRepeat').checked = false;
        document.getElementById('alarmSave').checked = false;
        document.getElementById('alarmType').value = 'daily';
        document.getElementById('alarmEditButtons').style.display = 'none';
      }
      
      updateAlarmTypeFields();
      updateToggleAlarmButton();
      modal.classList.add('show');
    }

    function closeAlarmModal() {
      document.getElementById('alarmModal').classList.remove('show');
    }

    function updateAlarmTypeFields() {
      const type = document.getElementById('alarmType').value;
      document.getElementById('alarmWeekdaysGroup').style.display = type === 'weekdays' ? 'block' : 'none';
      document.getElementById('alarmDateGroup').style.display = type === 'date' ? 'block' : 'none';
    }

    function updateWeekdaySelection(weekdayMask) {
      const buttons = document.querySelectorAll('#alarmWeekdays .weekday-btn');
      buttons.forEach(btn => {
        const value = parseInt(btn.getAttribute('data-value'));
        if (weekdayMask & value) {
          btn.classList.add('selected');
        } else {
          btn.classList.remove('selected');
        }
      });
    }

    function updateToggleAlarmButton() {
      const btn = document.getElementById('toggleAlarmBtn');
      if (!btn) return;
      
      const alarm = statusData.alarms ? statusData.alarms[currentAlarmIndex] : null;
      if (alarm && alarm.active) {
        btn.innerHTML = '<span data-translate="disable">' + (translations[currentLanguage]?.disable || 'Disable') + '</span>';
      } else {
        btn.innerHTML = '<span data-translate="enable">' + (translations[currentLanguage]?.enable || 'Enable') + '</span>';
      }
    }

    // Toggle weekday selection
    document.addEventListener('click', e => {
      if (e.target.classList.contains('weekday-btn')) {
        e.target.classList.toggle('selected');
      }
    });

    function saveAlarm() {
      const time = document.getElementById('alarmTime').value.split(':');
      const hour = parseInt(time[0]);
      const minute = parseInt(time[1]);
      const type = document.getElementById('alarmType').value;
      const text = document.getElementById('alarmText').value;
      const repeat = document.getElementById('alarmRepeat').checked ? 1 : 0;
      const save = document.getElementById('alarmSave').checked ? 1 : 0;
      
      const timeStr = `${hour.toString().padStart(2, '0')}:${minute.toString().padStart(2, '0')}`;
      let url = `/alarm/set?index=${currentAlarmIndex}&time=${timeStr}&text=${encodeURIComponent(text)}&repeat=${repeat}&save=${save}`;
      
      if (type === 'date') {
        const date = document.getElementById('alarmDate').value;
        url += `&date=${date}`;
      } else if (type === 'weekdays') {
        const weekdays = Array.from(document.querySelectorAll('#alarmWeekdays .weekday-btn.selected'))
          .reduce((sum, btn) => sum + parseInt(btn.getAttribute('data-value')), 0);
        url += `&weekdays=${weekdays}`;
      }
      
      fetch(url)
        .then(r => r.text())
        .then(msg => {
          alert(msg);
          closeAlarmModal();
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }

    function deleteAlarm() {
      if (confirm('Delete this alarm?')) {
        fetch(`/alarm/clear?index=${currentAlarmIndex}`)
          .then(r => r.text())
          .then(msg => {
            alert(msg);
            closeAlarmModal();
            updateStatus();
          })
          .catch(err => alert('Error: ' + err));
      }
    }

    function toggleAlarm() {
      const alarm = statusData.alarms[currentAlarmIndex];
      const enable = !alarm.active;
      
      fetch(`/alarm/toggle?index=${currentAlarmIndex}&enable=${enable ? 1 : 0}`)
        .then(r => r.text())
        .then(msg => {
          alert(msg);
          closeAlarmModal();
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }

    // Timer functions
    function openTimerModal() {
      document.getElementById('timerModal').classList.add('show');
    }

    function closeTimerModal() {
      document.getElementById('timerModal').classList.remove('show');
    }

    function startTimer() {
      const duration = document.getElementById('timerDuration').value.split(':');
      const seconds = parseInt(duration[0]) * 3600 + parseInt(duration[1]) * 60 + parseInt(duration[2]);
      const text = document.getElementById('timerText').value;
      
      fetch(`/timer?duration=${seconds}&text=${encodeURIComponent(text)}`)
        .then(r => r.text())
        .then(msg => {
          alert(msg);
          closeTimerModal();
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }

    function stopTimer() {
      fetch('/timer/clear')
        .then(r => r.text())
        .then(msg => {
          alert(msg);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }

    function addTimerMinute() {
      fetch('/timer/add?time=60')
        .then(r => r.text())
        .then(msg => {
          alert(msg);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }

    // Settings functions
    function openSettings() {
      document.getElementById('settingsPage').classList.add('show');
      updateSettingsFields(statusData);
    }

    function closeSettings() {
      document.getElementById('settingsPage').classList.remove('show');
    }

    function syncTimeNow() {
      fetch('/time/sync')
        .then(r => r.text())
        .then(msg => {
          alert(msg);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }

    function setManualTime() {
      const timeStr = document.getElementById('settingsManualTime').value;
      fetch('/time/set?time=' + encodeURIComponent(timeStr))
        .then(r => r.text())
        .then(msg => {
          alert(msg);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }

    function updateBrightnessValue(value) {
      document.getElementById('brightnessValue').textContent = value;
      fetch('/brightness/set?value=' + value);
    }

    function saveNightMode() {
      const enabled = document.getElementById('nightModeEnabled').checked ? 1 : 0;
      const start = document.getElementById('nightStartTime').value.split(':');
      const end = document.getElementById('nightEndTime').value.split(':');
      const brightness = document.getElementById('nightBrightness').value;
      
      fetch(`/nightmode/set?enabled=${enabled}&startHour=${start[0]}&startMinute=${start[1]}&endHour=${end[0]}&endMinute=${end[1]}&brightness=${brightness}&save=1`)
        .then(r => r.text())
        .then(msg => {
          alert(msg);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }

    function saveWiFiMode() {
      const mode = document.getElementById('wifiModeSelect').value;
      const autoOff = document.getElementById('wifiAutoOff').value;
      const ntpSync = document.getElementById('ntpSyncInterval').value;
      const weatherSync = document.getElementById('weatherSyncInterval').value;
      
      fetch(`/wifimode/set?mode=${mode}&autoOff=${autoOff}&ntpSync=${ntpSync}&weatherSync=${weatherSync}&save=1`)
        .then(r => r.text())
        .then(msg => {
          alert(msg);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }

    function updateWeatherNow() {
      const city = document.getElementById('settingsCity').value;
      fetch('/weather/update?city=' + encodeURIComponent(city))
        .then(r => r.text())
        .then(msg => {
          alert(msg);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }

    function testMelody(type) {
      const melody = type === 'alarm' 
        ? document.getElementById('settingsAlarmMelody').value
        : document.getElementById('settingsTimerMelody').value;
      
      fetch('/melody/test?melody=' + encodeURIComponent(melody))
        .then(r => r.text())
        .then(msg => alert(msg))
        .catch(err => alert('Error: ' + err));
    }

    function saveMelodies() {
      const alarm = document.getElementById('settingsAlarmMelody').value;
      const timer = document.getElementById('settingsTimerMelody').value;
      
      fetch('/melody/save?alarm=' + encodeURIComponent(alarm) + '&timer=' + encodeURIComponent(timer))
        .then(r => r.text())
        .then(msg => {
          alert(msg);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }

    function saveAllSettings() {
      const ntp = document.getElementById('settingsNtp').value;
      const tz = document.getElementById('settingsTimezone').value;
      const dst = document.getElementById('settingsDst').value;
      const city = document.getElementById('settingsCity').value;
      const bright = document.getElementById('settingsBrightness').value;
      
      fetch(`/settings/save?ntp=${encodeURIComponent(ntp)}&timezone=${tz}&dst=${dst}&city=${encodeURIComponent(city)}&brightness=${bright}`)
        .then(r => r.text())
        .then(msg => {
          alert(msg);
          updateStatus();
        })
        .catch(err => alert('Error: ' + err));
    }

    function restoreSettings() {
      if (confirm('Restore all settings from NVS?')) {
        fetch('/restore')
          .then(r => r.text())
          .then(msg => {
            alert(msg);
            setTimeout(() => location.reload(), 1000);
          })
          .catch(err => alert('Error: ' + err));
      }
    }

    function eraseNVS() {
      if (confirm('WARNING: This will erase ALL settings from NVS! Continue?')) {
        fetch('/erase')
          .then(r => r.text())
          .then(msg => alert(msg))
          .catch(err => alert('Error: ' + err));
      }
    }

    function rebootDevice() {
      if (confirm('Reboot the device?')) {
        fetch('/reboot')
          .then(r => r.text())
          .then(msg => {
            alert('Device is rebooting... Please wait 10 seconds.');
            setTimeout(() => location.reload(), 10000);
          })
          .catch(err => alert('Error: ' + err));
      }
    }

    // Utility functions
    function formatTime(hour, minute) {
      return `${pad(hour)}:${pad(minute)}`;
    }

    function pad(num) {
      return num < 10 ? '0' + num : num;
    }

    // Weather forecast functions
function showWeatherForecast() {
  const city = statusData?.city || 'Moscow';
  const modal = document.getElementById('weatherForecastModal');
  const content = document.getElementById('weatherForecastContent');
  const lang = currentLanguage || 'en';
  
  if (!modal || !content) {
    return;
  }
  
  // Показать загрузку
  content.innerHTML = `<p style="text-align: center; padding: 40px;">${translations[lang]?.loading || 'Loading...'}</p>`;
  modal.classList.add('show');
  
  // Прямой запрос к wttr.in из браузера с параметром языка
  const wttrUrl = `https://wttr.in/${encodeURIComponent(city)}?format=j1&lang=${lang}`;
  
  fetch(wttrUrl)
    .then(response => {
      if (!response.ok) {
        throw new Error(`HTTP ${response.status}: ${response.statusText}`);
      }
      return response.json();
    })
    .then(data => {
      if (!data || !data.current_condition) {
        throw new Error('Invalid weather data format');
      }
      
      // Форматируем и показываем
      const formatted = formatWeatherData(data, city, lang);
      content.innerHTML = formatted;
    })
    .catch(error => {
      console.error('[Weather] Error:', error);
      
      const errorMsg = escapeHtml(String(error.message || error));
      
      content.innerHTML = `
        <div style="padding: 20px;">
          <p style="color: var(--error-color); font-weight: bold; margin-bottom: 15px;">
            ⚠️ ${translations[lang]?.errorLoading || 'Failed to load forecast'}
          </p>
          <p style="margin-bottom: 10px;">
            <strong>Error:</strong> ${errorMsg}
          </p>
          <p style="margin-bottom: 20px; color: var(--text-secondary);">
            ${translations[lang]?.tryUpdating || 'Try refreshing the page or checking your internet connection.'}
          </p>
          <button class="btn btn-primary" onclick="retryWeatherForecast()" style="margin-right: 10px;">
            🔄 Retry
          </button>
        </div>
      `;
    });
}

// Функция для повторной попытки
function retryWeatherForecast() {
  showWeatherForecast();
}

// Форматирование JSON данных погоды
function formatWeatherData(weatherJson, city, lang) {
  try {
    // ИСПРАВЛЕНИЕ: Проверяем структуру данных wttr.in JSON
    if (!weatherJson || typeof weatherJson !== 'object') {
      return `<p>No weather data available for ${escapeHtml(city)}</p>`;
    }
    
    if (!weatherJson.current_condition || !Array.isArray(weatherJson.current_condition) || weatherJson.current_condition.length === 0) {
      return `<p>Invalid weather data format for ${escapeHtml(city)}</p>`;
    }
    
    const current = weatherJson.current_condition[0];
    const area = weatherJson.nearest_area && weatherJson.nearest_area[0];
    const forecast = weatherJson.weather;
    
    // ИСПРАВЛЕНИЕ: Проверка что current содержит необходимые данные
    if (!current) {
      return `<p>No current weather data available for ${escapeHtml(city)}</p>`;
    }
    
    let html = `
      <div style="max-height: 70vh; overflow-y: auto;">
        <h3 style="margin-top: 0; margin-bottom: 15px;">
          ${escapeHtml(translations[lang]?.weatherForecast || 'Weather Forecast for')} ${escapeHtml(city)}
        </h3>
        
        <div style="background: var(--weather-bg); padding: 15px; border-radius: 8px; margin-bottom: 20px;">
          <h4>🌤️ ${translations[lang]?.currentWeather || 'Current Weather'}</h4>
          <table style="width: 100%;">
            <tr><td>${translations[lang]?.temperature || 'Temperature'}:</td><td><strong>${escapeHtml(String(current.temp_C || '--'))}°C (${escapeHtml(String(current.temp_F || '--'))}°F)</strong></td></tr>
            <tr><td>${translations[lang]?.feelsLike || 'Feels like'}:</td><td>${escapeHtml(String(current.FeelsLikeC || '--'))}°C</td></tr>
            <tr><td>${translations[lang]?.humidity || 'Humidity'}:</td><td>${escapeHtml(String(current.humidity || '--'))}%</td></tr>
            <tr><td>${translations[lang]?.wind || 'Wind'}:</td><td>${escapeHtml(String(current.windspeedKmph || '--'))} km/h (${escapeHtml(String(current.winddir16Point || '--'))})</td></tr>
            <tr><td>${translations[lang]?.pressure || 'Pressure'}:</td><td>${escapeHtml(String(current.pressure || '--'))} hPa</td></tr>
            <tr><td>${translations[lang]?.precipitation || 'Precipitation'}:</td><td>${escapeHtml(String(current.precipMM || '--'))} mm</td></tr>
            <tr><td>${translations[lang]?.visibility || 'Visibility'}:</td><td>${escapeHtml(String(current.visibility || '--'))} km</td></tr>
          </table>
        </div>
        
        <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 15px;">
    `;
    
    // 3-дневный прогноз с проверками
    if (forecast && Array.isArray(forecast)) {
      for (let i = 0; i < Math.min(3, forecast.length); i++) {
        const day = forecast[i];
        if (!day) continue;
        
        const date = new Date(day.date);
        const dayNames = lang === 'ru' 
          ? ['Вс', 'Пн', 'Вт', 'Ср', 'Чт', 'Пт', 'Сб']
          : ['Sun', 'Mon', 'Tue', 'Wed', 'Thu', 'Fri', 'Sat'];
        const dayName = dayNames[date.getDay()] || '---';
        
        // Безопасный доступ к данным
        const maxTemp = day.maxtempC || '--';
        const minTemp = day.mintempC || '--';
        const hourlyData = day.hourly && day.hourly[0] ? day.hourly[0] : {};
        
        html += `
          <div style="background: var(--card-bg); padding: 15px; border-radius: 8px; border: 1px solid var(--border-color);">
            <h5 style="margin-top: 0;">${escapeHtml(dayName)} (${escapeHtml(String(day.date || ''))})</h5>
            <div>🌅 ${translations[lang]?.max || 'Max'}: ${escapeHtml(String(maxTemp))}°C</div>
            <div>🌇 ${translations[lang]?.min || 'Min'}: ${escapeHtml(String(minTemp))}°C</div>
            <div>💧 ${translations[lang]?.humidity || 'Humidity'}: ${escapeHtml(String(hourlyData.humidity || '--'))}%</div>
            <div>💨 ${translations[lang]?.wind || 'Wind'}: ${escapeHtml(String(hourlyData.windspeedKmph || '--'))} km/h</div>
            <div>🌧️ ${translations[lang]?.precipitation || 'Precipitation'}: ${escapeHtml(String(hourlyData.precipMM || '--'))} mm</div>
          </div>
        `;
      }
    }
    
    html += `
        </div>
        
        <div style="margin-top: 20px; font-size: 0.9em; color: var(--text-tertiary);">
    `;
    
    // Добавляем информацию о местоположении с проверками
    if (area) {
      const areaName = area.areaName && area.areaName[0] ? area.areaName[0].value : '';
      const country = area.country && area.country[0] ? area.country[0].value : '';
      html += `<p>📍 ${translations[lang]?.location || 'Location'}: ${escapeHtml(areaName)}, ${escapeHtml(country)}</p>`;
    }
    
    html += `
          <p>📅 ${translations[lang]?.lastUpdated || 'Last updated'}: ${escapeHtml(String(current.observation_time || ''))}</p>
          <p>🌍 ${translations[lang]?.dataSource || 'Data source'}: <a href="https://wttr.in/${encodeURIComponent(city)}" target="_blank" rel="noopener noreferrer">wttr.in</a></p>
        </div>
      </div>
    `;
    
    return html;
    
  } catch (error) {
    console.error('Error formatting weather:', error);
    // ИСПРАВЛЕНИЕ: Безопасный fallback с XSS защитой
    return `
      <h3>Weather Data</h3>
      <p style="color: var(--error-color);">Failed to format weather data: ${escapeHtml(String(error.message || 'Unknown error'))}</p>
      <pre style="background: var(--sensor-bg); color: var(--text-primary); padding: 15px; border-radius: 8px; overflow-x: auto; max-height: 400px; font-size: 12px;">
${escapeHtml(JSON.stringify(weatherJson, null, 2))}
      </pre>
    `;
  }
}
    
    function escapeHtml(text) {
      const div = document.createElement('div');
      div.textContent = text;
      return div.innerHTML;
    }
    
    function closeWeatherForecast() {
      document.getElementById('weatherForecastModal').classList.remove('show');
    }
    
    // Initialize
    loadPreferences();
    updateStatus();
    
    // ИСПРАВЛЕНИЕ: Перезапускаем applyLanguage после загрузки DOM
    // чтобы убедиться что все элементы обновились
    setTimeout(() => {
      applyLanguage(currentLanguage);
    }, 100);
    
    // ИСПРАВЛЕНИЕ: Сохраняем ссылку на интервал для предотвращения утечки памяти
    let statusUpdateInterval = setInterval(updateStatus, 2000);
    
    // Функция для остановки обновлений при необходимости
    window.stopStatusUpdates = function() {
      if (statusUpdateInterval) {
        clearInterval(statusUpdateInterval);
        statusUpdateInterval = null;
      }
    };
  </script>
)rawliteral";

const char html_part4[] PROGMEM = R"rawliteral(
  <!-- Weather Forecast Modal -->
  <div class="weather-forecast-modal" id="weatherForecastModal">
    <div class="weather-forecast-content">
      <button class="weather-forecast-close" onclick="closeWeatherForecast()">×</button>
      <div id="weatherForecastContent">Loading...</div>
    </div>
  </div>

  <style>
    /* Добавьте эти стили в основной CSS блок */
    .weather-forecast-modal table {
      width: 100%;
      border-collapse: collapse;
      margin: 10px 0;
    }
    
    .weather-forecast-modal td {
      padding: 8px 0;
      border-bottom: 1px solid var(--border-color);
    }
    
    .weather-forecast-modal td:first-child {
      color: var(--text-secondary);
      width: 40%;
    }
    
    .weather-forecast-modal td:last-child {
      text-align: right;
    }
    
    .weather-forecast-modal pre {
      background: var(--sensor-bg);
      color: var(--text-primary);
      padding: 15px;
      border-radius: 8px;
      overflow-x: auto;
      font-family: 'Courier New', monospace;
      font-size: 12px;
      line-height: 1.4;
      white-space: pre-wrap;
      word-wrap: break-word;
    }
  </style>

</body>
</html>
)rawliteral";

/*
 * getWebPage() больше не используется
 * 
 * Для ESP32-S3 используем chunked transfer в setupWebServer():
 * 
 * server.on("/", HTTP_GET, [&]() {
 *   server.setContentLength(CONTENT_LENGTH_UNKNOWN);
 *   server.send(200, "text/html; charset=utf-8", "");
 *   server.sendContent(FPSTR(html_part1));
 *   server.sendContent(FPSTR(html_part2));
 *   server.sendContent(FPSTR(html_part3));
 *   server.sendContent(FPSTR(html_part4));
 *   server.sendContent("");
 * });
 */