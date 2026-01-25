// Setup page for Captive Portal
// Dynamic language switching via JavaScript

const char SETUP_PAGE_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32 Clock Setup</title>
  <link rel="icon" href="data:image/svg+xml,<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 100 100'><text y='0.9em' font-size='90'>⏰</text></svg>">
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      display: flex;
      align-items: center;
      justify-content: center;
      padding: 20px;
    }
    .container {
      background: white;
      border-radius: 20px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
      max-width: 500px;
      width: 100%;
      padding: 40px 30px;
    }
    h1 {
      text-align: center;
      color: #667eea;
      margin-bottom: 10px;
      font-size: 28px;
    }
    .subtitle {
      text-align: center;
      color: #666;
      margin-bottom: 30px;
      font-size: 14px;
    }
    .section {
      margin-bottom: 25px;
      padding-bottom: 25px;
      border-bottom: 1px solid #eee;
    }
    .section:last-child {
      border-bottom: none;
      margin-bottom: 0;
      padding-bottom: 0;
    }
    .section-title {
      font-size: 16px;
      font-weight: 600;
      color: #333;
      margin-bottom: 15px;
      display: flex;
      align-items: center;
    }
    .section-title::before {
      content: '';
      width: 4px;
      height: 20px;
      background: #667eea;
      margin-right: 10px;
      border-radius: 2px;
    }
    .form-group {
      margin-bottom: 15px;
    }
    label {
      display: block;
      margin-bottom: 5px;
      color: #555;
      font-size: 14px;
      font-weight: 500;
    }
    input[type="text"],
    input[type="password"],
    input[type="number"],
    input[type="datetime-local"],
    select {
      width: 100%;
      padding: 12px 15px;
      border: 2px solid #e0e0e0;
      border-radius: 10px;
      font-size: 14px;
      transition: all 0.3s;
      background: #fafafa;
    }
    input:focus,
    select:focus {
      outline: none;
      border-color: #667eea;
      background: white;
    }
    .inline-group {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 10px;
    }
    button {
      width: 100%;
      padding: 15px;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      border: none;
      border-radius: 10px;
      font-size: 16px;
      font-weight: 600;
      cursor: pointer;
      transition: transform 0.2s, box-shadow 0.2s;
      margin-top: 10px;
    }
    button:hover {
      transform: translateY(-2px);
      box-shadow: 0 10px 25px rgba(102, 126, 234, 0.4);
    }
    button:active {
      transform: translateY(0);
    }
    .help-text {
      font-size: 12px;
      color: #999;
      margin-top: 5px;
    }
    .status {
      padding: 15px;
      border-radius: 10px;
      margin-bottom: 20px;
      text-align: center;
      font-size: 14px;
      display: none;
    }
    .status.success {
      background: #d4edda;
      color: #155724;
      border: 1px solid #c3e6cb;
    }
    .status.error {
      background: #f8d7da;
      color: #721c24;
      border: 1px solid #f5c6cb;
    }
    @media (max-width: 500px) {
      .container { padding: 30px 20px; }
      h1 { font-size: 24px; }
      .inline-group { grid-template-columns: 1fr; }
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>⏰ ESP32 Clock</h1>
    <p class="subtitle" data-translate="subtitle">Initial Setup</p>
    
    <div id="status" class="status"></div>
    
    <form id="setupForm">
      <!-- Language & Theme Selection -->
      <div class="section">
        <div class="section-title" data-translate="interface">Interface</div>
        <div class="inline-group">
          <div class="form-group">
            <label data-translate="language">Language</label>
            <select id="language" name="language" onchange="switchLanguage(this.value)">
              <option value="en">English</option>
              <option value="ru">Русский</option>
            </select>
          </div>
          <div class="form-group">
            <label data-translate="theme">Theme</label>
            <select id="theme" name="theme">
              <option value="light" data-translate="light">Light</option>
              <option value="dark" data-translate="dark">Dark</option>
            </select>
          </div>
        </div>
      </div>

      <!-- WiFi Settings -->
      <div class="section">
        <div class="section-title" data-translate="wifi_settings">WiFi Settings</div>
        <div class="form-group">
          <label data-translate="wifi_ssid">WiFi Network (SSID)</label>
          <input type="text" id="ssid" name="ssid" required placeholder="MyWiFi">
        </div>
        <div class="form-group">
          <label data-translate="wifi_password">WiFi Password</label>
          <input type="password" id="password" name="password" placeholder="********">
        </div>
      </div>

      <!-- Time Settings -->
      <div class="section">
        <div class="section-title" data-translate="time_settings">Time Settings</div>
        <div class="form-group">
          <label data-translate="current_time">Current Date & Time</label>
          <input type="datetime-local" id="datetime" name="datetime">
          <p class="help-text" data-translate="time_help">Set current time or leave empty to sync via NTP</p>
        </div>
        <div class="inline-group">
          <div class="form-group">
            <label data-translate="timezone">Timezone (hours)</label>
            <input type="number" id="tz" name="tz" value="3" min="-12" max="14" step="1">
          </div>
          <div class="form-group">
            <label data-translate="dst">DST Offset (hours)</label>
            <input type="number" id="dst" name="dst" value="0" min="-2" max="2" step="1">
          </div>
        </div>
        <div class="form-group">
          <label data-translate="ntp_server">NTP Server</label>
          <input type="text" id="ntp" name="ntp" value="pool.ntp.org">
        </div>
      </div>

      <!-- Weather Settings -->
      <div class="section">
        <div class="section-title" data-translate="weather_settings">Weather Settings</div>
        <div class="form-group">
          <label data-translate="city">City</label>
          <input type="text" id="city" name="city" value="Moscow" placeholder="Moscow">
        </div>
      </div>

      <button type="submit" data-translate="save_and_restart">Save & Restart</button>
    </form>
  </div>

  <script>
    const translations = {
      en: {
        subtitle: "Initial Setup",
        interface: "Interface",
        language: "Language",
        theme: "Theme",
        light: "Light",
        dark: "Dark",
        wifi_settings: "WiFi Settings",
        wifi_ssid: "WiFi Network (SSID)",
        wifi_password: "WiFi Password",
        time_settings: "Time Settings",
        current_time: "Current Date & Time",
        time_help: "Set current time or leave empty to sync via NTP",
        timezone: "Timezone (hours)",
        dst: "DST Offset (hours)",
        ntp_server: "NTP Server",
        weather_settings: "Weather Settings",
        city: "City",
        save_and_restart: "Save & Restart",
        saving: "Saving configuration...",
        success: "Configuration saved! Device will restart in 3 seconds...",
        error: "Error saving configuration. Please try again."
      },
      ru: {
        subtitle: "Начальная настройка",
        interface: "Интерфейс",
        language: "Язык",
        theme: "Тема",
        light: "Светлая",
        dark: "Тёмная",
        wifi_settings: "Настройки WiFi",
        wifi_ssid: "Сеть WiFi (SSID)",
        wifi_password: "Пароль WiFi",
        time_settings: "Настройки времени",
        current_time: "Текущие дата и время",
        time_help: "Установите текущее время или оставьте пустым для синхронизации через NTP",
        timezone: "Часовой пояс (часы)",
        dst: "Летнее время (часы)",
        ntp_server: "NTP сервер",
        weather_settings: "Настройки погоды",
        city: "Город",
        save_and_restart: "Сохранить и перезапустить",
        saving: "Сохранение конфигурации...",
        success: "Настройки сохранены! Устройство перезапустится через 3 секунды...",
        error: "Ошибка сохранения. Попробуйте снова."
      }
    };

    let currentLang = 'en';

    function switchLanguage(lang) {
      currentLang = lang;
      document.querySelectorAll('[data-translate]').forEach(element => {
        const key = element.getAttribute('data-translate');
        if (translations[lang][key]) {
          if (element.tagName === 'INPUT' || element.tagName === 'BUTTON') {
            if (element.placeholder !== undefined && key.includes('placeholder')) {
              element.placeholder = translations[lang][key];
            } else {
              element.textContent = translations[lang][key];
            }
          } else {
            element.textContent = translations[lang][key];
          }
        }
      });
    }

    function showStatus(message, isSuccess) {
      const status = document.getElementById('status');
      status.textContent = message;
      status.className = 'status ' + (isSuccess ? 'success' : 'error');
      status.style.display = 'block';
    }

    document.getElementById('setupForm').addEventListener('submit', async function(e) {
      e.preventDefault();
      
      const formData = new FormData(e.target);
      const data = Object.fromEntries(formData);
      
      showStatus(translations[currentLang].saving, true);
      
      try {
        const response = await fetch('/setup/save', {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify(data)
        });
        
        if (response.ok) {
          showStatus(translations[currentLang].success, true);
          setTimeout(() => {
            window.location.href = '/';
          }, 3000);
        } else {
          showStatus(translations[currentLang].error, false);
        }
      } catch (error) {
        showStatus(translations[currentLang].error, false);
      }
    });

    // Set current datetime
    const now = new Date();
    now.setMinutes(now.getMinutes() - now.getTimezoneOffset());
    document.getElementById('datetime').value = now.toISOString().slice(0, 16);
  </script>
</body>
</html>
)rawliteral";
