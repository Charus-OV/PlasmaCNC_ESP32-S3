#include "WebInterface.h"
#include "Config.h"
#include "StepperControl.h"
#include "PlasmaControl.h"
#include "THC_System.h"
#include "FileManager.h"

WebServer WebInterface::server(80);
WebSocketsServer WebInterface::webSocket(81);

void WebInterface::init() {
    // Запуск WiFi
    WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);
    Serial.printf("📡 WiFi AP started: %s\n", WiFi.softAPIP().toString().c_str());
    
    // Настройка HTTP маршрутов
    server.on("/", handleRoot);
    server.on("/api/status", handleAPIStatus);
    server.on("/api/files", handleAPIFiles);
    server.on("/api/upload", HTTP_POST, []() {
        server.send(200, "text/plain", "Upload complete");
    }, handleFileUpload);
    
    server.begin();
    
    // Запуск WebSocket
    webSocket.begin();
    webSocket.onEvent(handleWebSocket);
    
    Serial.println("✅ Web Interface initialized");
}

void WebInterface::handleClient() {
    server.handleClient();
    webSocket.loop();
}

void WebInterface::handleRoot() {
    server.send(200, "text/html", getMainPage());
}

void WebInterface::handleAPIStatus() {
    DynamicJsonDocument doc(1024);
    doc["status"] = "ok";
    doc["version"] = "1.0.0";
    doc["uptime"] = millis() / 1000;
    doc["freeHeap"] = ESP.getFreeHeap();
    doc["wifiClients"] = WiFi.softAPgetStationNum();
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebInterface::handleAPIFiles() {
    auto files = FileManager::listFiles();
    DynamicJsonDocument doc(2048);
    JsonArray filesArray = doc.to<JsonArray>();
    
    for (const String& file : files) {
        filesArray.add(file);
    }
    
    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}

void WebInterface::handleFileUpload() {
    // Заглушка для загрузки файлов
    server.send(200, "text/plain", "File upload endpoint ready");
}

void WebInterface::handleWebSocket(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
    switch (type) {
        case WStype_DISCONNECTED:
            Serial.printf("[%u] WebSocket disconnected\n", num);
            break;
        case WStype_CONNECTED:
            {
                IPAddress ip = webSocket.remoteIP(num);
                Serial.printf("[%u] WebSocket connected from %d.%d.%d.%d\n", num, ip[0], ip[1], ip[2], ip[3]);
                sendSystemState();
            }
            break;
        case WStype_TEXT:
            {
                String message = String((char*)payload);
                processWebSocketCommand(message);
            }
            break;
    }
}

void WebInterface::processWebSocketCommand(String message) {
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, message);
    
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return;
    }
    
    String command = doc["command"];
    Serial.println("Received command: " + command);
    
    if (command == "plasma_on") {
        PlasmaControl::startPlasma();
    } else if (command == "plasma_off") {
        PlasmaControl::stopPlasma();
    } else if (command == "jog") {
        String direction = doc["direction"];
        float distance = doc["distance"];
        StepperControl::jog(direction, distance);
    } else if (command == "home_all") {
        StepperControl::homeAll();
    } else if (command == "emergency_stop") {
        // Emergency stop logic here
        Serial.println("🛑 EMERGENCY STOP");
    }
    
    sendSystemState();
}

void WebInterface::sendSystemState() {
    DynamicJsonDocument doc(1024);
    
    doc["type"] = "status";
    doc["arcVoltage"] = PlasmaControl::getArcVoltage();
    doc["arcOK"] = PlasmaControl::isArcOK();
    doc["plasmaActive"] = PlasmaControl::isActive();
    doc["currentHeight"] = StepperControl::getCurrentZ();
    doc["targetVoltage"] = THC_System::getTargetVoltage();
    doc["thcEnabled"] = THC_System::isEnabled();
    doc["thcActive"] = THC_System::isActive();
    doc["machineState"] = "Ready";
    doc["x"] = StepperControl::getCurrentX();
    doc["y"] = StepperControl::getCurrentY();
    doc["z"] = StepperControl::getCurrentZ();
    
    String jsonString;
    serializeJson(doc, jsonString);
    webSocket.broadcastTXT(jsonString);
}

String WebInterface::getMainPage() {
    return R"rawliteral(
<!DOCTYPE html>
<html lang="ru">
<head>
    <meta charset="UTF-8">
    <title>Плазменный ЧПУ - ESP32-S3</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        :root {
            --primary: #2196F3;
            --secondary: #FF9800;
            --success: #4CAF50;
            --danger: #f44336;
            --dark: #1a1a1a;
            --darker: #2a2a2a;
            --text: #ffffff;
        }
        
        * { margin: 0; padding: 0; box-sizing: border-box; }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: var(--dark);
            color: var(--text);
            overflow-x: hidden;
        }
        
        .app-container {
            display: grid;
            grid-template-rows: auto 1fr auto;
            height: 100vh;
            gap: 10px;
            padding: 10px;
        }
        
        /* Header */
        .header {
            background: var(--darker);
            padding: 15px 20px;
            border-radius: 10px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            box-shadow: 0 2px 10px rgba(0,0,0,0.3);
        }
        
        .logo { display: flex; align-items: center; gap: 10px; }
        .logo h1 { color: var(--success); font-size: 1.5em; }
        
        .status-bar {
            display: flex;
            gap: 15px;
            align-items: center;
        }
        
        .status-item {
            padding: 5px 10px;
            border-radius: 15px;
            background: #333;
            font-size: 0.9em;
        }
        
        /* Main Content */
        .main-content {
            display: grid;
            grid-template-columns: 300px 1fr 300px;
            gap: 10px;
            height: 100%;
        }
        
        .panel {
            background: var(--darker);
            border-radius: 10px;
            padding: 15px;
            border: 1px solid #333;
            box-shadow: 0 2px 5px rgba(0,0,0,0.2);
        }
        
        .panel h3 {
            color: var(--primary);
            margin-bottom: 15px;
            border-bottom: 1px solid #333;
            padding-bottom: 5px;
        }
        
        /* Monitoring Panel */
        .monitor-grid {
            display: grid;
            grid-template-columns: 1fr 1fr;
            gap: 8px;
        }
        
        .monitor-item {
            background: #333;
            padding: 10px;
            border-radius: 5px;
            text-align: center;
        }
        
        .monitor-label {
            font-size: 0.8em;
            color: #aaa;
            margin-bottom: 5px;
        }
        
        .monitor-value {
            font-size: 1.2em;
            font-weight: bold;
            color: var(--success);
        }
        
        .position-display {
            margin-top: 15px;
            background: #333;
            padding: 10px;
            border-radius: 5px;
        }
        
        /* Jog Control - НОВОЕ РАСПОЛОЖЕНИЕ */
        .jog-container {
            display: flex;
            gap: 20px;
            align-items: center;
            justify-content: center;
        }
        
        .xy-jog-grid {
            display: grid;
            grid-template-areas: 
                ". y-plus ."
                "x-minus center x-plus"
                ". y-minus .";
            grid-template-columns: 80px 80px 80px;
            grid-template-rows: 80px 80px 80px;
            gap: 5px;
        }
        
        .z-jog-grid {
            display: grid;
            grid-template-rows: 80px 80px;
            gap: 5px;
        }
        
        .jog-btn {
            background: var(--primary);
            color: white;
            border: none;
            border-radius: 10px;
            font-size: 1.2em;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.2s;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        
        .jog-btn:hover {
            transform: scale(1.05);
            box-shadow: 0 4px 8px rgba(0,0,0,0.3);
        }
        
        .jog-btn:active {
            transform: scale(0.95);
        }
        
        /* Расположение кнопок XY */
        .btn-y-plus { grid-area: y-plus; background: #FF9800; }
        .btn-y-minus { grid-area: y-minus; background: #FF9800; }
        .btn-x-plus { grid-area: x-plus; background: #2196F3; }
        .btn-x-minus { grid-area: x-minus; background: #2196F3; }
        .btn-center { 
            grid-area: center; 
            background: #666; 
            font-size: 0.8em;
            cursor: default;
        }
        .btn-center:hover { transform: none; box-shadow: none; }
        
        /* Кнопки Z */
        .btn-z-plus { background: #4CAF50; }
        .btn-z-minus { background: #4CAF50; }
        
        /* Control Buttons */
        .control-buttons {
            display: flex;
            flex-direction: column;
            gap: 10px;
            margin-top: 20px;
        }
        
        .btn {
            background: var(--primary);
            color: white;
            border: none;
            padding: 12px 15px;
            border-radius: 5px;
            cursor: pointer;
            transition: all 0.3s;
            font-size: 0.9em;
        }
        
        .btn:hover {
            transform: translateY(-1px);
            box-shadow: 0 3px 8px rgba(0,0,0,0.3);
        }
        
        .btn-success { background: var(--success); }
        .btn-danger { background: var(--danger); }
        .btn-warning { background: var(--secondary); }
        
        /* THC Controls */
        .slider-group {
            margin: 15px 0;
        }
        
        .slider-group label {
            display: block;
            margin-bottom: 5px;
            color: #ccc;
        }
        
        .slider-container {
            display: flex;
            align-items: center;
            gap: 10px;
        }
        
        input[type="range"] {
            flex: 1;
            height: 6px;
            background: #333;
            border-radius: 3px;
            outline: none;
        }
        
        .slider-value {
            min-width: 60px;
            text-align: right;
            font-weight: bold;
        }
        
        /* Connection Status */
        .connection-status {
            text-align: center;
            padding: 10px;
            border-radius: 5px;
            margin: 10px 0;
            background: #333;
        }
        
        /* Responsive */
        @media (max-width: 1200px) {
            .main-content {
                grid-template-columns: 1fr;
                grid-template-rows: auto auto auto;
            }
        }
    </style>
</head>
<body>
    <div class="app-container">
        <!-- Header -->
        <header class="header">
            <div class="logo">
                <h1>⚡ Плазменный ЧПУ Контроллер</h1>
                <span>ESP32-S3</span>
            </div>
            <div class="status-bar">
                <div class="status-item" id="connectionStatus">🟢 Подключено</div>
                <div class="status-item" id="machineStatus">Готов</div>
                <div class="status-item" id="thcStatus">THC: Выкл</div>
                <div class="status-item" id="plasmaStatus">Плазма: Выкл</div>
            </div>
        </header>

        <!-- Main Content -->
        <div class="main-content">
            <!-- Monitoring Panel -->
            <div class="panel monitoring-panel">
                <h3>📊 Мониторинг системы</h3>
                <div class="monitor-grid">
                    <div class="monitor-item">
                        <div class="monitor-label">Напряжение дуги</div>
                        <div class="monitor-value" id="arcVoltage">0.0 V</div>
                    </div>
                    <div class="monitor-item">
                        <div class="monitor-label">Текущая высота</div>
                        <div class="monitor-value" id="currentHeight">0.00 mm</div>
                    </div>
                    <div class="monitor-item">
                        <div class="monitor-label">Целевое напряжение</div>
                        <div class="monitor-value" id="targetVoltage">140 V</div>
                    </div>
                    <div class="monitor-item">
                        <div class="monitor-label">Статус дуги</div>
                        <div class="monitor-value" id="arcStatus">НЕТ</div>
                    </div>
                </div>
                
                <div class="position-display">
                    <div style="margin-bottom: 10px; color: #aaa;">Позиция:</div>
                    <div style="display: flex; justify-content: space-between;">
                        <span>X: <span id="posX">0.0</span> mm</span>
                        <span>Y: <span id="posY">0.0</span> mm</span>
                        <span>Z: <span id="posZ">0.0</span> mm</span>
                    </div>
                </div>
            </div>

            <!-- Jog Control Panel -->
            <div class="panel jog-panel">
                <h3>🎮 Ручное управление</h3>
                
                <div class="jog-container">
                    <!-- XY Jog Grid -->
                    <div class="xy-jog-grid">
                        <button class="jog-btn btn-y-plus" onclick="jog('Y+')">Y+</button>
                        <button class="jog-btn btn-y-minus" onclick="jog('Y-')">Y-</button>
                        <button class="jog-btn btn-x-minus" onclick="jog('X-')">X-</button>
                        <div class="jog-btn btn-center">XY</div>
                        <button class="jog-btn btn-x-plus" onclick="jog('X+')">X+</button>
                    </div>
                    
                    <!-- Z Jog Grid -->
                    <div class="z-jog-grid">
                        <button class="jog-btn btn-z-plus" onclick="jog('Z+')">Z+</button>
                        <button class="jog-btn btn-z-minus" onclick="jog('Z-')">Z-</button>
                    </div>
                </div>
                
                <div class="control-buttons">
                    <button class="btn btn-success" onclick="plasmaOn()">M03 - Плазма ВКЛ</button>
                    <button class="btn btn-danger" onclick="plasmaOff()">M05 - Плазма ВЫКЛ</button>
                    <button class="btn btn-warning" onclick="homeAll()">🏠 Нулевание осей</button>
                    <button class="btn btn-danger" onclick="emergencyStop()">🛑 Аварийный Стоп</button>
                </div>
            </div>

            <!-- THC Control Panel -->
            <div class="panel thc-panel">
                <h3>🎛 Управление THC</h3>
                
                <div class="slider-group">
                    <label>Целевое напряжение: <span class="slider-value" id="voltageValue">140</span> V</label>
                    <div class="slider-container">
                        <input type="range" id="voltageSlider" min="80" max="200" value="140" step="1">
                    </div>
                </div>
                
                <div class="slider-group">
                    <label>Мертвая зона: ±<span class="slider-value" id="deadZoneValue">5</span> V</label>
                    <div class="slider-container">
                        <input type="range" id="deadZoneSlider" min="1" max="20" value="5" step="1">
                    </div>
                </div>
                
                <div class="slider-group">
                    <label>Скорость коррекции: <span class="slider-value" id="correctionValue">0.10</span> mm/s</label>
                    <div class="slider-container">
                        <input type="range" id="correctionSlider" min="0.01" max="0.5" value="0.1" step="0.01">
                    </div>
                </div>
                
                <div style="display: flex; gap: 10px; margin: 15px 0;">
                    <button class="btn btn-success" id="thcToggle" onclick="toggleTHC()">THC Вкл</button>
                    <button class="btn" onclick="saveTHCSettings()">💾 Сохранить</button>
                </div>
                
                <div style="background: #333; padding: 10px; border-radius: 5px; margin-top: 10px;">
                    <div style="font-size: 0.9em; color: #aaa;">Статус THC:</div>
                    <div id="thcActiveStatus" style="color: #f44336;">Не активно</div>
                </div>
            </div>
        </div>

        <!-- Connection Status -->
        <div class="connection-status" id="connectionInfo">
            🔴 Ожидание подключения WebSocket...
        </div>
    </div>

    <script>
        let ws = new WebSocket('ws://' + window.location.hostname + ':81/');
        let reconnectInterval;
        
        // Инициализация
        document.addEventListener('DOMContentLoaded', function() {
            setupEventListeners();
        });
        
        function setupEventListeners() {
            // Слайдеры THC
            document.getElementById('voltageSlider').addEventListener('input', function(e) {
                document.getElementById('voltageValue').textContent = e.target.value;
            });
            
            document.getElementById('deadZoneSlider').addEventListener('input', function(e) {
                document.getElementById('deadZoneValue').textContent = e.target.value;
            });
            
            document.getElementById('correctionSlider').addEventListener('input', function(e) {
                document.getElementById('correctionValue').textContent = parseFloat(e.target.value).toFixed(2);
            });
        }
        
        // WebSocket handlers
        ws.onopen = function() {
            console.log('WebSocket connected');
            updateConnectionStatus(true);
            clearInterval(reconnectInterval);
        };
        
        ws.onclose = function() {
            console.log('WebSocket disconnected');
            updateConnectionStatus(false);
            // Попытка переподключения
            reconnectInterval = setInterval(() => {
                console.log('Attempting to reconnect...');
                ws = new WebSocket('ws://' + window.location.hostname + ':81/');
            }, 3000);
        };
        
        ws.onmessage = function(event) {
            const data = JSON.parse(event.data);
            updateUI(data);
        };
        
        function updateUI(data) {
            // Обновление мониторинга
            document.getElementById('arcVoltage').textContent = data.arcVoltage.toFixed(1) + ' V';
            document.getElementById('currentHeight').textContent = data.currentHeight.toFixed(2) + ' mm';
            document.getElementById('targetVoltage').textContent = data.targetVoltage + ' V';
            document.getElementById('arcStatus').textContent = data.arcOK ? 'ДА' : 'НЕТ';
            document.getElementById('arcStatus').style.color = data.arcOK ? '#4CAF50' : '#f44336';
            
            // Обновление позиции
            document.getElementById('posX').textContent = data.x.toFixed(1);
            document.getElementById('posY').textContent = data.y.toFixed(1);
            document.getElementById('posZ').textContent = data.z.toFixed(1);
            
            // Обновление статусов
            document.getElementById('machineStatus').textContent = data.machineState;
            document.getElementById('thcStatus').textContent = 'THC: ' + (data.thcEnabled ? 'Вкл' : 'Выкл');
            document.getElementById('plasmaStatus').textContent = 'Плазма: ' + (data.plasmaActive ? 'Вкл' : 'Выкл');
            document.getElementById('thcActiveStatus').textContent = data.thcActive ? 'Активно' : 'Не активно';
            document.getElementById('thcActiveStatus').style.color = data.thcActive ? '#4CAF50' : '#f44336';
            
            // Обновление кнопки THC
            const thcToggle = document.getElementById('thcToggle');
            thcToggle.textContent = data.thcEnabled ? 'THC Выкл' : 'THC Вкл';
            thcToggle.className = data.thcEnabled ? 'btn btn-danger' : 'btn btn-success';
        }
        
        function updateConnectionStatus(connected) {
            const status = document.getElementById('connectionInfo');
            status.textContent = connected ? '🟢 WebSocket подключен - Система онлайн' : '🔴 WebSocket отключен - Переподключение...';
            status.style.background = connected ? '#4CAF50' : '#f44336';
        }
        
        // Управление системой
        function plasmaOn() {
            sendCommand('plasma_on');
        }
        
        function plasmaOff() {
            sendCommand('plasma_off');
        }
        
        function toggleTHC() {
            sendCommand('toggle_thc');
        }
        
        function homeAll() {
            sendCommand('home_all');
        }
        
        function emergencyStop() {
            if(confirm('!!! ВНИМАНИЕ !!!\n\nАварийная остановка системы. Продолжить?')) {
                sendCommand('emergency_stop');
            }
        }
        
        function jog(direction) {
            sendCommand('jog', { direction: direction, distance: 10 });
        }
        
        function saveTHCSettings() {
            const settings = {
                voltage: parseInt(document.getElementById('voltageSlider').value),
                deadZone: parseInt(document.getElementById('deadZoneSlider').value),
                correctionRate: parseFloat(document.getElementById('correctionSlider').value)
            };
            sendCommand('thc_settings', settings);
            alert('Настройки THC сохранены!');
        }
        
        // Вспомогательные функции
        function sendCommand(command, data = {}) {
            const message = { command, ...data };
            if(ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify(message));
            } else {
                alert('WebSocket не подключен. Команда не отправлена.');
            }
        }
    </script>
</body>
</html>
)rawliteral";
}