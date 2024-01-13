#include <Wire.h>

#include <SPI.h>

#include <Adafruit_GFX.h>

#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// SPI OLED Display pins for ESP8266
#define OLED_MOSI D7
#define OLED_CLK D5
#define OLED_DC D3
#define OLED_RESET D0

// I2C OLED Display address
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 displaySPI(SCREEN_WIDTH, SCREEN_HEIGHT, & SPI, OLED_DC, OLED_RESET, -1);
Adafruit_SSD1306 displayI2C(SCREEN_WIDTH, SCREEN_HEIGHT, & Wire, -1);

boolean gameStarted = false;
boolean singlePlayer = true;

///-------Multiplayer Pong Game------------------
///----------------------------------------------

int paddle1X = SCREEN_WIDTH / 2;
int paddle2X = SCREEN_WIDTH / 2;
int ballX = SCREEN_WIDTH / 3;
int ballY = SCREEN_HEIGHT / 3;
int ballVelocityX = 1;
int ballVelocityY = 1;
const int paddleWidth = 16;
const int paddleHeight = 4;
const int ballSize = 2;

bool onFirstScreen = true;

bool pongOver = false;

int m1 = paddle1X;
int m2 = paddle2X;

void pongGameOver(int winner) {
  displaySPI.clearDisplay();
  displaySPI.setTextSize(1);
  displaySPI.setTextColor(SSD1306_WHITE);
  displaySPI.setCursor(0, 0);
  displaySPI.println("Game Over!");

  displayI2C.clearDisplay();
  displayI2C.setTextSize(1);
  displayI2C.setTextColor(SSD1306_WHITE);
  displayI2C.setCursor(0, 0);
  displayI2C.println("Game Over!");

  if (winner == 1) {
    Serial.println("P1 won");
    displaySPI.println("You won!");
    displayI2C.println("You lost!");
  } else {
    Serial.println("P2 won");
    displaySPI.println("You lost!");
    displayI2C.println("You won!");
  }

  displaySPI.display();
  displayI2C.display();

  delay(1000);
}

void updatePongGame() { 
  ballX += ballVelocityX;
  ballY += ballVelocityY;
  
  if (onFirstScreen) {
    if (ballY >= SCREEN_HEIGHT - ballSize && ballX >= paddle1X && ballX <= paddle1X + paddleWidth) {
      ballVelocityY = -ballVelocityY;
    } else if (ballY <= 0) {
      // Move to I2C screen
      onFirstScreen = false;
      ballVelocityY = -ballVelocityY;
      ballVelocityX = -ballVelocityX;
      ballY = 1; // Start from top of I2C screen      
      ballX = SCREEN_WIDTH - ballX;
    } else if (ballX >= SCREEN_WIDTH || ballX <= 0) {
      ballVelocityX = -ballVelocityX;
    } else if (ballY >= SCREEN_HEIGHT - 1) {
      pongOver = true;
      pongGameOver(2);
    }
  } else {
    if (ballY >= SCREEN_HEIGHT - ballSize && SCREEN_WIDTH - ballX >= paddle2X && SCREEN_WIDTH - ballX <= paddle2X + paddleWidth) {
      ballVelocityY = -ballVelocityY;
    } else if (ballY < 0) {
      // Move to SPI screen
      onFirstScreen = true;
      ballVelocityY = -ballVelocityY;
      ballVelocityX = -ballVelocityX;
      ballX = SCREEN_WIDTH - ballX;
      ballY = 1; // Start from bottom of SPI screen      
    } else if (ballX >= SCREEN_WIDTH || ballX <= 0) {
      ballVelocityX = -ballVelocityX;
    } else if (ballY >= SCREEN_HEIGHT - 1) {
      pongOver = true;
      pongGameOver(1);
    }
  }
}

void drawPongGame() {
  displaySPI.clearDisplay();
  displayI2C.clearDisplay();

  displaySPI.fillRect(paddle1X, SCREEN_HEIGHT - paddleHeight, paddleWidth, paddleHeight, SSD1306_WHITE);
  displayI2C.fillRect(SCREEN_WIDTH - paddle2X - paddleWidth, SCREEN_HEIGHT - paddleHeight, paddleWidth, paddleHeight, SSD1306_WHITE);

  if (onFirstScreen) {
    displaySPI.fillRect(ballX, ballY, ballSize, ballSize, SSD1306_WHITE);
  } else {
    displayI2C.fillRect(ballX, ballY, ballSize, ballSize, SSD1306_WHITE);
  }

  displaySPI.display();
  displayI2C.display();
}

///----------------------------------------------

///-------Single Player Flappy Bird--------------
///----------------------------------------------

int birdY;
int velocity;
const int gravity = 1;
const int jumpStrength = -5;
bool gameOver;
int score;

const int obstacleWidth = 5;
const int gapHeight = random(26, 30);
const int obstacleInterval = random(30, 45); // Time between spawning obstacles
int timeSinceLastObstacle = 0;

struct Obstacle {
  int x;
  int gapY;
};

const int maxObstacles = 5;
Obstacle obstacles[maxObstacles];

void addObstacle(int x, int gapY) {
  for (int i = 0; i < maxObstacles; i++) {
    if (obstacles[i].x == 0) {
      obstacles[i].x = x;
      obstacles[i].gapY = gapY;
      return;
    }
  }
}

void moveObstacles() {
  for (int i = 0; i < maxObstacles; i++) {
    if (obstacles[i].x > 0) {
      obstacles[i].x--;
      if (obstacles[i].x + obstacleWidth < 0) {
        // Obstacle is off the screen, reset it
        obstacles[i].x = 0;
      }
    }
  }
}

void drawObstacles() {
  for (int i = 0; i < maxObstacles; i++) {
    if (obstacles[i].x > 0) {
      int gapStart = obstacles[i].gapY - gapHeight / 2;
      int gapEnd = obstacles[i].gapY + gapHeight / 2;
      for (int y = 0; y < SCREEN_HEIGHT; y++) {
        if (y < gapStart || y > gapEnd) {
          displaySPI.drawPixel(obstacles[i].x, y, SSD1306_WHITE);
          displaySPI.drawPixel(obstacles[i].x + obstacleWidth - 1, y, SSD1306_WHITE);
        }
      }
    }
  }
}

void checkCollision() {
  // Check collision with the ground
  if (birdY >= SCREEN_HEIGHT + 10) {
    gameOver = true;
    return;
  }

  for (int i = 0; i < maxObstacles; i++) {
    if (obstacles[i].x > 0) {
      int gapStart = obstacles[i].gapY - gapHeight / 2;
      int gapEnd = obstacles[i].gapY + gapHeight / 2;
      // Check if bird is within the horizontal range of the obstacle
      if (10 >= obstacles[i].x && 10 <= obstacles[i].x + obstacleWidth - 1) {
        // Check if bird is outside the vertical range of the gap
        if (birdY < gapStart || birdY > gapEnd) {
          gameOver = true;
          break; // Stop checking further if collision is detected
        }
      }
    }
  }
}

void resetFlappyBirdGame() {
  birdY = SCREEN_HEIGHT / 2;
  velocity = 0;
  gameOver = false;
  score = 0;
  displaySPI.clearDisplay();
  displaySPI.display();

  for (int i = 0; i < maxObstacles; i++) {
    obstacles[i].x = 0;
  }
}

///----------------------------------------------

void startScreen() {
  displaySPI.clearDisplay();
  displaySPI.setTextSize(1);
  displaySPI.setTextColor(SSD1306_WHITE);
  displaySPI.setCursor(0, 0);
  displaySPI.println(singlePlayer ? "Singleplayer" : "Multiplayer");
  displaySPI.display();

  displayI2C.clearDisplay();
  displayI2C.setTextSize(1);
  displayI2C.setTextColor(SSD1306_WHITE);
  displayI2C.setCursor(0, 0);
  displayI2C.println(singlePlayer ? "Singleplayer" : "Multiplayer");
  displayI2C.display();
}

void setup() {
  Serial.begin(115200);

  // SPI Display Initialization
  if (!displaySPI.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println("SPI OLED failed to initialize");
    for (;;); // Loop forever
  }

  // I2C Display Initialization
  if (!displayI2C.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("I2C OLED failed to initialize");
    for (;;); // Loop forever
  }

  startScreen();
}

void loop() {
  if (!gameStarted && Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');

    Serial.println(command);

    if (command.indexOf("Multiplayer") >= 0) {
      singlePlayer = false;
    } else if (command.indexOf("Single Player") >= 0) {
      singlePlayer = true;
    } else if (command.indexOf("Game Start") >= 0) {
      delay(1000);
      gameStarted = true;
    }

    displaySPI.clearDisplay();
    displaySPI.setTextSize(1);
    displaySPI.setTextColor(SSD1306_WHITE);
    displaySPI.setCursor(0, 0);
    displaySPI.println(singlePlayer ? "Singleplayer" : "Multiplayer");
    displaySPI.display();

    displayI2C.clearDisplay();
    displayI2C.setTextSize(1);
    displayI2C.setTextColor(SSD1306_WHITE);
    displayI2C.setCursor(0, 0);
    displayI2C.println(singlePlayer ? "Singleplayer" : "Multiplayer");
    displayI2C.display();
  }

  if (gameStarted) {
    if (singlePlayer) {
      if (!gameOver) {
        if (Serial.available() > 0) {
          String command = Serial.readStringUntil('\n');

          if (command.indexOf("Pressed") >= 0) {
            velocity = jumpStrength;
          } else if (command.indexOf("Reset") >= 0) {
            gameStarted = false;
            singlePlayer = true;
            gameOver = false;
            startScreen();
          }
        }

        velocity += gravity;
        birdY += velocity;

        // Spawn new obstacle
        timeSinceLastObstacle++;
        if (timeSinceLastObstacle > obstacleInterval) {
          timeSinceLastObstacle = 0;
          int gapY = random(10, SCREEN_HEIGHT - 10);
          addObstacle(SCREEN_WIDTH, gapY);
        }

        moveObstacles();
        checkCollision();

        displaySPI.clearDisplay();
        if (!gameOver) {
          displaySPI.drawPixel(10, birdY, SSD1306_WHITE); // Draw bird
          score++; // Increase score as time passes
        } else {
          displaySPI.setTextSize(1);
          displaySPI.setTextColor(SSD1306_WHITE);
          displaySPI.setCursor(0, 0);
          displaySPI.println("Game Over!");
          displaySPI.print("Score: ");
          displaySPI.println(score);

          Serial.println("Game Over");
        }
        drawObstacles();
        displaySPI.display();
      } else if (Serial.available() > 0) {
        String command = Serial.readStringUntil('\n');

        if (command.indexOf("Pressed") >= 0) {
          resetFlappyBirdGame();
        } else if (command.indexOf("Reset") >= 0) {
          gameStarted = false;
          singlePlayer = true;
          gameOver = false;
          startScreen();
        }
      }
    } else {

      if (!pongOver) {

        if (Serial.available() > 0) {
          String command = Serial.readStringUntil('\n');
          if (command.startsWith("P1")) {
            int t = command.substring(3).toInt();
            m1 = (abs(t - m1) > 2) ? t : m1;
          } else if (command.startsWith("P2")) {
            int t = SCREEN_WIDTH - command.substring(3).toInt();
            m2 = (abs(t - m2) > 2) ? t : m2;
          } else if (command.indexOf("Reset") >= 0) {
            gameStarted = false;
            singlePlayer = true;
            pongOver = false;
            paddle1X = SCREEN_WIDTH / 2;
            paddle2X = SCREEN_WIDTH / 2;
            ballX = SCREEN_WIDTH / 3;
            ballY = SCREEN_HEIGHT / 3;
            ballVelocityX = 1;
            ballVelocityY = 1;
            startScreen();
          }
        }

        if (paddle1X != m1) {
          paddle1X += (paddle1X > m1) ? min(-1, (m1 - paddle1X) / 2) : max(1, (m1 - paddle1X) / 2);
        }

        if (paddle2X != m2) {
          paddle2X += (paddle2X > m2) ? min(-1, (m2 - paddle2X) / 2) : max(1, (m2 - paddle2X) / 2);
        }
        
        updatePongGame();
        if (!pongOver)
          drawPongGame();
      } else {
        if (Serial.available() > 0) {
          String command = Serial.readStringUntil('\n');
          if (command.startsWith("P1") || command.startsWith("P2")) {
            pongOver = false;
            paddle1X = SCREEN_WIDTH / 2;
            paddle2X = SCREEN_WIDTH / 2;
            ballX = SCREEN_WIDTH / 3;
            ballY = SCREEN_HEIGHT / 3;
            ballVelocityX = 1;
            ballVelocityY = 1;
          } else if (command.indexOf("Reset") >= 0) {
            gameStarted = false;
            singlePlayer = true;
            pongOver = false;
            paddle1X = SCREEN_WIDTH / 2;
            paddle2X = SCREEN_WIDTH / 2;
            ballX = SCREEN_WIDTH / 3;
            ballY = SCREEN_HEIGHT / 3;
            ballVelocityX = 1;
            ballVelocityY = 1;
            startScreen();
          }
        }
      }
    }
  }

  delay(50);
}
