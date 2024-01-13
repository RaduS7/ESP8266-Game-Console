#include <ESP8266WebServer.h>

const char * ssid = "esprs";
const char * password = "12345678";

IPAddress player1IP;
IPAddress player2IP;
bool player1Connected = false;
bool player2Connected = false;
bool gameStarted = false;

ESP8266WebServer server(80);

const int led1 = D3;
const int led2 = D2;
const int buzzer = D5;
const int gameModeButtonPin = D7;
const int resetGameButtonPin = D6;
bool gameMode = false;
unsigned long lastDebounceTime1 = 0;
unsigned long lastDebounceTime2 = 0;
unsigned long debounceDelay = 250;

void htmlIndex() {
  IPAddress clientIP = server.client().remoteIP();
  
  if (!player1Connected || clientIP == player1IP) {
    player1IP = clientIP;
    player1Connected = true;    
  } else if (!player2Connected || clientIP == player2IP) {
    player2IP = clientIP;
    player2Connected = true;    
  } else {    
    server.send(403, "text/plain", "Game is already in progress.");
    return;
  }

  String player = (clientIP == player1IP) ? "Player 1" : "Player 2";
  String sliderPlayer = ((clientIP == player1IP) ? "P1" : "P2");

  String htmlContent;

  if (gameMode) {    
    htmlContent =
      "<html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>"
    "html, body { height: 100%; overflow: hidden; margin: 0; }" // Disable scrolling and reset margins
    ".disable-dbl-tap-zoom { touch-action: manipulation; }" // Disable double-tap zoom
    "body { font-family: Arial, sans-serif; text-align: center; }"
    "h1 { color: #333; margin-top: 20px; }"
    "button { background-color: #4CAF50; color: white; padding: 20px 40px;" 
    "text-align: center; text-decoration: none; display: inline-block;"
    "font-size: 20px; margin: 10px 2px; cursor: pointer; border: none; border-radius: 12px; }"
    // Slider Styles
    ".slider-container { display: flex; justify-content: center; align-items: flex-end; height: 80%; }" // Container for slider
    "#slider { -webkit-appearance: none; appearance: none; width: 90%; height: 50px; background: #d3d3d3; outline: none; opacity: 0.7; -webkit-transition: .2s; transition: opacity .2s; }"
    "#slider:hover { opacity: 1; }"
    "#slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 50px; height: 50px; background: #4CAF50; cursor: pointer; }"
    "</style></head><body>"
    "<h1>Hello " + player + "!</h1>"
    // Slider in a container
    "<div class='slider-container'>"
    "<input type='range' id='slider' min='1' max='127' value='64' oninput='sendSliderValue()'>"
    "</div>"
    "<script>"
    "function sendSliderValue() {"
    "    var sliderValue = document.getElementById('slider').value;"
    "    var xhr = new XMLHttpRequest();"
    "    xhr.open('GET', '/sliderMove?player=" + sliderPlayer + "&value=' + sliderValue, true);"
    "    xhr.send();"
    "}"
    "</script>"
    "</body></html>";
  } else {
    htmlContent =
      "<html><head>"
    "<meta name='viewport' content='width=device-width, initial-scale=1'>"
    "<style>"
    "html, body { height: 100%; overflow: hidden; margin: 0; }" // Disable scrolling and reset margins
    ".disable-dbl-tap-zoom { touch-action: manipulation; }" // Disable double-tap zoom
    "body { font-family: Arial, sans-serif; text-align: center; }"
    "h1 { color: #333; margin-top: 20px; }"
    "button { background-color: #4CAF50; color: white; padding: 20px 40px;" 
    "text-align: center; text-decoration: none; display: inline-block;"
    "font-size: 20px; margin: 10px 2px; cursor: pointer; border: none; border-radius: 12px; }" 
    "</style></head><body>"
    "<h1>Hello " + player + "!</h1>"
    "<button class='disable-dbl-tap-zoom' onclick='sendButtonPress()'>Press me</button>" // Button with disabled double-tap zoom
    "<script>"
    "function sendButtonPress() {"
    "    var xhr = new XMLHttpRequest();"
    "    xhr.open('GET', '/buttonPress?player=" + player + "', true);"
    "    xhr.send();"
    "}"
    "</script>"
    "</body></html>";
  }

  server.send(200, "text/html", htmlContent);
}

void handleButtonPress() {
  String player = server.arg("player");
  if (gameMode) {
    Serial.println(player + " pressed the button");
  } else {
    Serial.println("Pressed");
  }

  server.send(200, "text/plain", "Button press registered");
}

void handleSliderMove() {
  String player = server.arg("player");
  int value = server.arg("value").toInt();
  Serial.println(player + " " + value);

  server.send(200, "text/plain", "Slider move registered");
}

void setupServer() {
  server.on("/", htmlIndex);
  server.on("/buttonPress", handleButtonPress);
  server.on("/sliderMove", handleSliderMove);
  server.begin();
  Serial.println("HTTP server started");
}

void createNetwork() {
  Serial.println("Setting up as an Access Point");
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid, password);

  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);
}

void setup() {
  Serial.begin(115200);
  pinMode(resetGameButtonPin, INPUT_PULLUP);
  pinMode(gameModeButtonPin, INPUT_PULLUP);
  pinMode(led1, OUTPUT);
  pinMode(led2, OUTPUT);
  delay(1000);
  createNetwork();
  setupServer();
}

void loop() {
  server.handleClient();

  if (!gameStarted) {
    if ((gameMode && player1Connected && player2Connected) ||
      (!gameMode && player1Connected)) {
      gameStarted = true;
      Serial.println("Game Start");
    }
  }

  static bool lastGameButtonState = HIGH;
  bool currentButtonState1 = digitalRead(gameModeButtonPin);

  if (lastGameButtonState == HIGH && currentButtonState1 == LOW && millis() - lastDebounceTime1 > debounceDelay) {
    gameMode = !gameMode;
    Serial.println(gameMode ? "Multiplayer" : "Single Player");

    lastDebounceTime1 = millis();
  }
  lastGameButtonState = currentButtonState1;

  static bool lastResetButtonState = HIGH;
  bool currentButtonState2 = digitalRead(resetGameButtonPin);

  if (lastResetButtonState == HIGH && currentButtonState2 == LOW && millis() - lastDebounceTime2 > debounceDelay) {
    Serial.println("Reset");

    player1Connected = false;
    player2Connected = false;
    gameStarted = false;

    lastDebounceTime2 = millis();
  }
  lastResetButtonState = currentButtonState2;

  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');

    if (command.indexOf("Game Over") >= 0) {
      tone(buzzer, 300);
      delay(1000);
      noTone(buzzer);
    } else if (command.indexOf("P1 won") >= 0) {
      tone(buzzer, 300);
      digitalWrite(led1, HIGH);
      delay(1000);
      noTone(buzzer);
      digitalWrite(led1, LOW);
    } else if (command.indexOf("P2 won") >= 0) {
      tone(buzzer, 300);
      digitalWrite(led2, HIGH);
      delay(1000);
      noTone(buzzer);
      digitalWrite(led2, LOW);
    }
  }
}
