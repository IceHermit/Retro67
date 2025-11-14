#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include <SPI.h>
#include <math.h>

// ================== TFT ==================
#define TFT_CS 15
#define TFT_DC 2
#define TFT_RST 4
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320

// ================== Buttons ==================
#define BUTTON_LEFT 32
#define BUTTON_RIGHT 33
#define BUTTON_UP 27
#define BUTTON_DOWN 26
#define BUTTON_ENTER 25

void setupButtons() {
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_ENTER, INPUT_PULLUP);
}

// ================= Theme Colours ==================

auto BG_Colour = tft.color565(15, 56, 15);
auto FG1_Colour = tft.color565(155, 188, 15);
auto FG2_Colour = tft.color565(48, 98, 48);

// ================== Menu ==================
enum Game { MENU,
            PONG,
            SNAKE,
            TETRIS,
            ARCADE,
            PLATFORMER,
            BATTLE };

Game currentGame = MENU;

const char* games[] = { "Pong", "Snake", "Tetris", "Space Runner", "Italian Plumber", "Pocket Animals", "CHANGE THEME" };
int TOTAL_SELECTIONS = 7;
int selectedGame = 0;

void drawMenu(bool blink = false) {
  tft.setTextSize(2);
  for (int i = 0; i < TOTAL_SELECTIONS; i++) {
    tft.setCursor(40, 30 + i * 40);

    if (i != selectedGame)
      tft.setTextColor(FG2_Colour);
    else
      tft.setTextColor(FG1_Colour);

    tft.print(games[i]);
    if (i == selectedGame && blink) {
      tft.fillTriangle(30, 35 + i * 40, 20, 30 + i * 40, 20, 40 + i * 40, FG2_Colour);
    }
    if (!blink) {
      tft.fillTriangle(30, 35 + i * 40, 20, 30 + i * 40, 20, 40 + i * 40, BG_Colour);
    }
  }
}

// ========== Exit to Menu system ==========
unsigned long enterPressStart = 0;
bool enterHeld = false;
const unsigned long HOLD_DURATION = 1000;  // 1 second

bool checkReturnToMenu() {
  if (digitalRead(BUTTON_ENTER) == LOW) {
    if (!enterHeld) {
      enterHeld = true;
      enterPressStart = millis();
    } else if (millis() - enterPressStart >= HOLD_DURATION) {
      tft.fillScreen(BG_Colour);
      tft.setCursor(50, SCREEN_HEIGHT / 2);
      tft.setTextColor(FG1_Colour);
      tft.setTextSize(2);
      tft.print("Returning to");
      tft.setCursor(SCREEN_WIDTH / 2 - 40, SCREEN_HEIGHT / 2 + 20);
      tft.print("Menu...");
      delay(500);
      tft.fillScreen(BG_Colour);
      drawMenu();
      currentGame = MENU;
      delay(300);
      tft.fillScreen(BG_Colour);
      enterHeld = false;
      return true;
    }
  } else {
    enterHeld = false;  // Reset if released early
  }
  return false;
}


// ================== PONG ==================
#define PADDLE_WIDTH 50
#define PADDLE_HEIGHT 10
int paddle1X, paddle1Y;
int paddle2X, paddle2Y;
int paddleSpeed = 5;
#define BALL_SIZE 8
float ballX, ballY;
float ballSpeedX = 3.0, ballSpeedY = -3.0;
float speedMultiplier = 1.05;
int score1 = 0, score2 = 0;
int prevBallX, prevBallY;
int prevPaddle1X, prevPaddle2X;
int prevPaddle1Y, prevPaddle2Y;

bool pongGameOver = false;

void setupPong() {

  paddle1X = 80;
  paddle2X = 80;

  paddle1Y = 20;
  paddle2Y = SCREEN_HEIGHT - PADDLE_HEIGHT - 20;

  ballX = SCREEN_WIDTH / 2.0;
  ballY = SCREEN_HEIGHT / 2.0;
  prevPaddle1X = paddle1X;
  prevPaddle2X = paddle2X;
  prevBallX = ballX;
  prevBallY = ballY;
  score1 = 0;
  score2 = 0;

  pongGameOver = false;

  tft.fillScreen(BG_Colour);
}
#define WINNING_SCORE 5

void updatePong() {

  if (checkReturnToMenu()) return;

  if (pongGameOver) {
    if (digitalRead(BUTTON_ENTER) == LOW)
      setupPong();
    return;
  }

  // ------------------ Paddle Controls ------------------
  // Player 1 (top paddle) LEFT/RIGHT using UP/DOWN buttons
  if (digitalRead(BUTTON_UP) == LOW) {
    paddle1X -= paddleSpeed;
    if (paddle1X < 0) paddle1X = 0;
  }
  if (digitalRead(BUTTON_DOWN) == LOW) {
    paddle1X += paddleSpeed;
    if (paddle1X > SCREEN_WIDTH - PADDLE_WIDTH - 25) paddle1X = SCREEN_WIDTH - PADDLE_WIDTH - 25;
  }

  // Player 2 (bottom paddle) LEFT/RIGHT using LEFT/RIGHT buttons
  if (digitalRead(BUTTON_LEFT) == LOW) {
    paddle2X -= paddleSpeed;
    if (paddle2X < 0) paddle2X = 0;
  }
  if (digitalRead(BUTTON_RIGHT) == LOW) {
    paddle2X += paddleSpeed;
    if (paddle2X > SCREEN_WIDTH - PADDLE_WIDTH - 25) paddle2X = SCREEN_WIDTH - PADDLE_WIDTH - 25;
  }

  // ------------------ Erase Previous Objects ------------------
  tft.fillRect(prevBallX, prevBallY, BALL_SIZE, BALL_SIZE, BG_Colour);
  if (prevPaddle1X != paddle1X) tft.fillRect(prevPaddle1X, paddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, BG_Colour);
  if (prevPaddle2X != paddle2X) tft.fillRect(prevPaddle2X, paddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, BG_Colour);

  // ------------------ Ball Movement ------------------
  ballX += ballSpeedX;
  ballY += ballSpeedY;

  // Bounce off side walls
  if (ballX <= 0 || ballX >= SCREEN_WIDTH - 25 - BALL_SIZE) ballSpeedX = -ballSpeedX;

  // Paddle collisions
  // Top paddle
  if ((ballY <= paddle1Y + PADDLE_HEIGHT) && (ballY >= paddle1Y) && (ballX + BALL_SIZE >= paddle1X) && (ballX <= paddle1X + PADDLE_WIDTH)) {
    ballY = paddle1Y + PADDLE_HEIGHT;
    ballSpeedY = -ballSpeedY * speedMultiplier;
    if (abs(ballSpeedY) > 8) ballSpeedY = (ballSpeedY > 0 ? 8 : -8);
  }

  // Bottom paddle
  if ((ballY + BALL_SIZE >= paddle2Y) && (ballY + BALL_SIZE <= paddle2Y + PADDLE_HEIGHT) && (ballX + BALL_SIZE >= paddle2X) && (ballX <= paddle2X + PADDLE_WIDTH)) {
    ballY = paddle2Y - BALL_SIZE;
    ballSpeedY = -ballSpeedY * speedMultiplier;
    if (abs(ballSpeedY) > 8) ballSpeedY = (ballSpeedY > 0 ? 8 : -8);
  }

  // ------------------ Scoring ------------------
  if (ballY < 0) {  // Player 2 scores
    score2++;
    ballX = SCREEN_WIDTH / 2.0;
    ballY = SCREEN_HEIGHT / 2.0;
    paddle1X = 80;
    paddle2X = 80;
    delay(300);
    tft.fillScreen(BG_Colour);
    ballSpeedX = random(0, 2) == 0 ? 3.0 : -3.0;
    ballSpeedY = 3.0;
  }
  if (ballY > SCREEN_HEIGHT + BALL_SIZE) {  // Player 1 scores
    score1++;
    ballX = SCREEN_WIDTH / 2.0;
    ballY = SCREEN_HEIGHT / 2.0;
    paddle1X = 80;
    paddle2X = 80;
    delay(300);
    tft.fillScreen(BG_Colour);
    ballSpeedX = random(0, 2) == 0 ? 3.0 : -3.0;
    ballSpeedY = -3.0;
  }

  // ------------------ Check Game Over ------------------
  if (score1 >= WINNING_SCORE || score2 >= WINNING_SCORE) {
    pongGameOver = true;

    tft.setTextSize(2);
    tft.setTextColor(FG1_Colour, BG_Colour);
    tft.setCursor(30, SCREEN_HEIGHT / 2 - 20);
    if (score1 >= WINNING_SCORE) {
      tft.print("PLAYER LEFT WINS!");
    } else {
      tft.print("PLAYER RIGHT WINS!");
    }
    tft.setCursor(30, SCREEN_HEIGHT / 2 + 20);
    tft.print("Press X");
    tft.setCursor(30, SCREEN_HEIGHT / 2 + 40);
    tft.print("to play again");
    return;  // Stop drawing ball/paddles
  }

  // ------------------ Draw Scores ------------------
  tft.setRotation(1);
  tft.setTextSize(2);
  tft.setTextColor(FG1_Colour, BG_Colour);
  tft.setCursor(SCREEN_WIDTH / 2, 5);
  tft.print(score1);
  tft.print(" : ");
  tft.print(score2);
  tft.setRotation(0);

  // Draw middle lines
  tft.drawLine(0, SCREEN_HEIGHT / 2, SCREEN_WIDTH - 25, SCREEN_HEIGHT / 2, FG2_Colour);
  tft.drawLine(SCREEN_WIDTH - 25, 0, SCREEN_WIDTH - 25, SCREEN_HEIGHT, FG2_Colour);

  // Draw ball and paddles
  tft.fillRect((int)ballX, (int)ballY, BALL_SIZE, BALL_SIZE, FG2_Colour);
  tft.fillRect(paddle1X, paddle1Y, PADDLE_WIDTH, PADDLE_HEIGHT, FG1_Colour);
  tft.fillRect(paddle2X, paddle2Y, PADDLE_WIDTH, PADDLE_HEIGHT, FG1_Colour);

  // Update previous positions
  prevBallX = ballX;
  prevBallY = ballY;
  prevPaddle1X = paddle1X;
  prevPaddle2X = paddle2X;

  delay(20);
}


// ================== SNAKE ==================
#define SNAKE_BLOCK 8
#define SNAKE_ROWS ((SCREEN_HEIGHT - 24) / SNAKE_BLOCK)
#define SNAKE_COLS (SCREEN_WIDTH / SNAKE_BLOCK)
int snakeX[256], snakeY[256], snakeLength;
int snakeDir = 0;  // 0=UP,1=RIGHT,2=DOWN,3=LEFT
int snakeFoodX, snakeFoodY;
int prevSnakeX[256], prevSnakeY[256];

void spawnSnakeFood() {
  snakeFoodX = random(SNAKE_COLS);
  snakeFoodY = random(SNAKE_ROWS);
}
bool snakeGameOver = false;

void setupSnake() {
  tft.fillScreen(BG_Colour);
  snakeLength = 5;
  int startX = SNAKE_COLS / 2, startY = SNAKE_ROWS / 2;
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = startX - i;
    snakeY[i] = startY;
    prevSnakeX[i] = snakeX[i];
    prevSnakeY[i] = snakeY[i];
  }
  snakeDir = 1;

  snakeGameOver = false;

  spawnSnakeFood();
  tft.fillRect(snakeFoodX * SNAKE_BLOCK, 24 + snakeFoodY * SNAKE_BLOCK, SNAKE_BLOCK, SNAKE_BLOCK, FG2_Colour);
}


void updateSnake() {
  if (checkReturnToMenu()) return;

  // If game over, show screen and wait for restart
  if (snakeGameOver) {
    tft.setTextSize(2);
    tft.setTextColor(FG1_Colour, BG_Colour);
    tft.setCursor(30, SCREEN_HEIGHT / 2 - 20);
    tft.print("GAME OVER");
    tft.setTextSize(2);
    tft.setCursor(30, SCREEN_HEIGHT / 2 + 20);
    tft.print("Score: ");
    tft.print(snakeLength - 5);

    if (digitalRead(BUTTON_ENTER) == LOW) {
      setupSnake();
      snakeGameOver = false;
    }
    return;
  }

  // ------------------ Controls ------------------
  if (digitalRead(BUTTON_UP) == LOW && snakeDir != 2) snakeDir = 0;
  if (digitalRead(BUTTON_RIGHT) == LOW && snakeDir != 3) snakeDir = 1;
  if (digitalRead(BUTTON_DOWN) == LOW && snakeDir != 0) snakeDir = 2;
  if (digitalRead(BUTTON_LEFT) == LOW && snakeDir != 1) snakeDir = 3;

  // ------------------ Move Snake ------------------
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }
  if (snakeDir == 0) snakeY[0]--;
  else if (snakeDir == 1) snakeX[0]++;
  else if (snakeDir == 2) snakeY[0]++;
  else snakeX[0]--;

  // Wrap-around
  if (snakeX[0] < 0) snakeX[0] = SNAKE_COLS - 1;
  if (snakeX[0] >= SNAKE_COLS) snakeX[0] = 0;
  if (snakeY[0] < 0) snakeY[0] = SNAKE_ROWS - 1;
  if (snakeY[0] >= SNAKE_ROWS) snakeY[0] = 0;

  // ------------------ Eat Food ------------------
  if (snakeX[0] == snakeFoodX && snakeY[0] == snakeFoodY) {
    snakeLength++;
    spawnSnakeFood();
  }

  // ------------------ Check Self-Collision ------------------
  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      snakeGameOver = true;
      return;
    }
  }

  // ------------------ Draw Snake ------------------
  for (int i = 0; i < snakeLength; i++) {
    if (prevSnakeX[i] != snakeX[i] || prevSnakeY[i] != snakeY[i]) {
      tft.fillRect(prevSnakeX[i] * SNAKE_BLOCK, 24 + prevSnakeY[i] * SNAKE_BLOCK, SNAKE_BLOCK, SNAKE_BLOCK, BG_Colour);
      tft.fillRect(snakeX[i] * SNAKE_BLOCK, 24 + snakeY[i] * SNAKE_BLOCK, SNAKE_BLOCK, SNAKE_BLOCK, FG1_Colour);
      prevSnakeX[i] = snakeX[i];
      prevSnakeY[i] = snakeY[i];
    }
  }

  // Draw food
  tft.fillRect(snakeFoodX * SNAKE_BLOCK, 24 + snakeFoodY * SNAKE_BLOCK, SNAKE_BLOCK, SNAKE_BLOCK, FG2_Colour);

  // Draw score
  tft.setTextSize(2);
  tft.setTextColor(FG1_Colour, BG_Colour);
  tft.setCursor(5, 5);
  tft.print("Score: ");
  tft.print(snakeLength - 5);

  // Draw top line
  tft.drawLine(0, 24, SCREEN_WIDTH, 24, FG2_Colour);

  delay(100);
}


// ================== TETRIS ==================
#define BLOCK_SIZE 18
#define TETRIS_ROWS (SCREEN_HEIGHT / BLOCK_SIZE)
#define TETRIS_COLS (SCREEN_WIDTH / BLOCK_SIZE)
int tetrisGrid[TETRIS_ROWS][TETRIS_COLS] = { 0 };
int prevTetrisGrid[TETRIS_ROWS][TETRIS_COLS] = { 0 };
int tetrisShape[4][4], prevShape[4][4];
int tetrisX, tetrisY, prevX = -1, prevY = -1;
unsigned long tetrisLastFall = 0;
int tetrisScore = 0;
int tetrisColor;
int nextShapeIdx, nextColor;
int lastDrawX = -1, lastDrawY = -1;
int lastDrawShape[4][4] = { 0 };

const int shapes[7][4][4] = {
  { { 1, 1, 1, 1 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },  // I
  { { 1, 1, 0, 0 }, { 0, 1, 1, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },  // Z
  { { 0, 1, 1, 0 }, { 1, 1, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },  // S
  { { 1, 1, 1, 0 }, { 0, 1, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },  // T
  { { 1, 1, 0, 0 }, { 1, 1, 0, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },  // O
  { { 1, 0, 0, 0 }, { 1, 1, 1, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } },  // L
  { { 0, 0, 1, 0 }, { 1, 1, 1, 0 }, { 0, 0, 0, 0 }, { 0, 0, 0, 0 } }   // J
};

const uint16_t shapeColors[7] = { tft.color565(73, 182, 169), tft.color565(170, 8, 21), tft.color565(1, 165, 138), tft.color565(236, 54, 125), tft.color565(250, 234, 3), tft.color565(253, 201, 18), tft.color565(61, 155, 225) };

void setupTetris() {
  tetrisX = TETRIS_COLS / 2 - 2;
  tetrisY = 0;
  tetrisGrid[TETRIS_ROWS][TETRIS_COLS] = { 0 };
  int idx = nextShapeIdx >= 0 ? nextShapeIdx : random(7);
  tetrisColor = nextColor > 0 ? nextColor : shapeColors[idx];
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++) tetrisShape[i][j] = shapes[idx][i][j];

  nextShapeIdx = random(7);
  nextColor = shapeColors[nextShapeIdx];
  prevX = -1;
  prevY = -1;
  lastDrawX = -1;
  lastDrawY = -1;
  memset(lastDrawShape, 0, sizeof(lastDrawShape));
}

bool checkCollision(int x, int y, int shape[4][4]) {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (shape[i][j]) {
        int nx = x + j, ny = y + i;
        if (nx < 0 || nx >= TETRIS_COLS || ny >= TETRIS_ROWS) return true;
        if (tetrisGrid[ny][nx]) return true;
      }
    }
  }
  return false;
}

// Force full redraw by making prevTetrisGrid contain a sentinel
void invalidatePrevGrid() {
  for (int i = 0; i < TETRIS_ROWS; i++)
    for (int j = 0; j < TETRIS_COLS; j++)
      prevTetrisGrid[i][j] = -1;  // -1 will never match a valid color or 0
}


void mergeShape() {
  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      if (tetrisShape[i][j]) {
        int nx = tetrisX + j, ny = tetrisY + i;
        if (ny >= 0 && ny < TETRIS_ROWS && nx >= 0 && nx < TETRIS_COLS) tetrisGrid[ny][nx] = tetrisColor;
      }
  invalidatePrevGrid();
}

void clearLines() {
  for (int i = 0; i < TETRIS_ROWS; i++) {
    bool full = true;
    for (int j = 0; j < TETRIS_COLS; j++)
      if (!tetrisGrid[i][j]) full = false;
    if (full) {
      for (int k = i; k > 0; k--)
        for (int j = 0; j < TETRIS_COLS; j++) tetrisGrid[k][j] = tetrisGrid[k - 1][j];
      for (int j = 0; j < TETRIS_COLS; j++) tetrisGrid[0][j] = 0;
      tetrisScore += 10;
    }
  }
  invalidatePrevGrid();
}

void drawTetris() {
  static int prevScore = -1;
  static int prevNextShape = -1;
  static int prevNextColor = -1;

  const int Y_OFFSET = 20;  // visual shift in pixels

  // --- Draw border around playfield ---
  tft.drawRect(0, Y_OFFSET, TETRIS_COLS * BLOCK_SIZE, TETRIS_ROWS * BLOCK_SIZE, FG2_Colour);

  // --- 1) Erase old falling shape ---
  if (lastDrawX != -1 && lastDrawY != -1) {
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        if (lastDrawShape[i][j]) {
          int gx = lastDrawX + j;
          int gy = lastDrawY + i;
          if (gx >= 0 && gx < TETRIS_COLS && gy >= 0 && gy < TETRIS_ROWS) {
            if (!tetrisGrid[gy][gx]) {
              int px = gx * BLOCK_SIZE;
              int py = gy * BLOCK_SIZE + Y_OFFSET;
              tft.fillRect(px, py, BLOCK_SIZE, BLOCK_SIZE, BG_Colour);
            }
          }
        }
      }
    }
  }

  // --- 2) Draw only static grid cells that changed ---
  for (int r = 0; r < TETRIS_ROWS; r++) {
    for (int c = 0; c < TETRIS_COLS; c++) {
      if (tetrisGrid[r][c] != prevTetrisGrid[r][c]) {
        int px = c * BLOCK_SIZE;
        int py = r * BLOCK_SIZE + Y_OFFSET;
        if (tetrisGrid[r][c]) {
          tft.fillRect(px + 1, py + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2, tetrisGrid[r][c]);
          tft.drawRect(px, py, BLOCK_SIZE, BLOCK_SIZE, FG1_Colour);
        } else {
          tft.fillRect(px, py, BLOCK_SIZE, BLOCK_SIZE, BG_Colour);
        }
        prevTetrisGrid[r][c] = tetrisGrid[r][c];
      }
    }
  }

  // --- 3) Draw current falling piece ---
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      if (tetrisShape[i][j]) {
        int gx = tetrisX + j;
        int gy = tetrisY + i;
        if (gx >= 0 && gx < TETRIS_COLS && gy >= 0 && gy < TETRIS_ROWS) {
          int px = gx * BLOCK_SIZE;
          int py = gy * BLOCK_SIZE + Y_OFFSET;
          tft.fillRect(px + 1, py + 1, BLOCK_SIZE - 2, BLOCK_SIZE - 2, tetrisColor);
          tft.drawRect(px, py, BLOCK_SIZE, BLOCK_SIZE, FG1_Colour);
        }
      }
    }
  }

  // --- 4) Save last drawn shape for next erase ---
  memcpy(lastDrawShape, tetrisShape, sizeof(lastDrawShape));
  lastDrawX = tetrisX;
  lastDrawY = tetrisY;

  // --- 5) Update HUD only when needed ---
  if (tetrisScore != prevScore) {
    tft.fillRect(TETRIS_COLS * BLOCK_SIZE + 5, 5, 70, 16, BG_Colour);
    tft.setTextColor(FG1_Colour, BG_Colour);
    tft.setTextSize(2);
    tft.setCursor(TETRIS_COLS * BLOCK_SIZE + 5, 5);
    tft.print("Score:");
    tft.print(tetrisScore);
    prevScore = tetrisScore;
  }

  if (nextShapeIdx != prevNextShape || nextColor != prevNextColor) {
    // clear preview area
    tft.fillRect(TETRIS_COLS * BLOCK_SIZE + 5, 40, 50, 50, BG_Colour);
    for (int i = 0; i < 4; i++) {
      for (int j = 0; j < 4; j++) {
        if (shapes[nextShapeIdx][i][j]) {
          int sx = TETRIS_COLS * BLOCK_SIZE + 5 + j * (BLOCK_SIZE / 2);
          int sy = 40 + i * (BLOCK_SIZE / 2);
          tft.fillRect(sx + 1, sy + 1, BLOCK_SIZE / 2 - 2, BLOCK_SIZE / 2 - 2, nextColor);
          tft.drawRect(sx, sy, BLOCK_SIZE / 2, BLOCK_SIZE / 2, FG1_Colour);
        }
      }
    }
    prevNextShape = nextShapeIdx;
    prevNextColor = nextColor;
  }
}


void updateTetris() {
  if (checkReturnToMenu()) return;

  unsigned long current = millis();
  int speed = 500 - min(tetrisScore * 10, 400);  // speed up as score increases

  // --- 1) Handle inputs (move & rotate) ---
  if (digitalRead(BUTTON_LEFT) == LOW) {
    if (!checkCollision(tetrisX - 1, tetrisY, tetrisShape)) tetrisX--;
  }

  if (digitalRead(BUTTON_RIGHT) == LOW) {
    if (!checkCollision(tetrisX + 1, tetrisY, tetrisShape)) tetrisX++;
  }

  if (digitalRead(BUTTON_DOWN) == LOW) {
    if (!checkCollision(tetrisX, tetrisY + 1, tetrisShape)) tetrisY++;
  }

  if (digitalRead(BUTTON_UP) == LOW) {
    int temp[4][4];
    memcpy(temp, tetrisShape, sizeof(tetrisShape));

    for (int i = 0; i < 4; i++)
      for (int j = 0; j < 4; j++)
        tetrisShape[j][3 - i] = temp[i][j];  // rotate

    if (checkCollision(tetrisX, tetrisY, tetrisShape))
      memcpy(tetrisShape, temp, sizeof(temp));  // undo if blocked

    delay(100);  // debounce rotation
  }

  delay(50);  // small input delay to prevent jitter

  // --- 2) Gravity (piece falling) ---
  if (current - tetrisLastFall > speed) {
    tetrisLastFall = current;

    if (!checkCollision(tetrisX, tetrisY + 1, tetrisShape)) {
      tetrisY++;
    } else {
      mergeShape();
      clearLines();
      setupTetris();

      // game over check
      if (checkCollision(tetrisX, tetrisY, tetrisShape)) {
        memset(tetrisGrid, 0, sizeof(tetrisGrid));
        invalidatePrevGrid();
        memset(prevTetrisGrid, 0, sizeof(prevTetrisGrid));
        tetrisScore = 0;
      }
    }
  }

  // --- 3) Draw everything once per frame ---
  drawTetris();
}

// ================== Themes ================

int theme = 0;
int total_themes = 4;

void cycleThemes() {
  theme = (theme + 1) % total_themes;
  switch (theme) {
    case 0:
      BG_Colour = tft.color565(15, 56, 15);
      FG1_Colour = tft.color565(155, 188, 15);
      FG2_Colour = tft.color565(48, 98, 48);
      break;
    case 1:
      BG_Colour = tft.color565(54, 59, 116);
      FG1_Colour = tft.color565(199, 157, 215);
      FG2_Colour = tft.color565(239, 79, 145);
      break;
    case 2:
      BG_Colour = tft.color565(53, 20, 62);
      FG1_Colour = tft.color565(137, 183, 208);
      FG2_Colour = tft.color565(100, 79, 156);
      break;
    case 3:
      BG_Colour = tft.color565(13, 43, 69);
      FG1_Colour = tft.color565(141, 105, 122);
      FG2_Colour = tft.color565(255, 170, 94);
      break;
  }
}

// ================== ARCADE (Operius-like) ==================

#define MAX_STARS 40

bool gameOver = false;
unsigned long arcadeStartTime = 0;  // when the game starts
int score = 0;

#define MAX_HILLS 10
struct Hill {
  float x;  // horizontal position
  float z;  // depth from horizon, 70 → 0
};
Hill hills[MAX_HILLS];
unsigned long lastHillSpawn = 0;
unsigned long HILL_SPAWN_INTERVAL = 1500;
const float difficulty = 0.98;
const int horizonY = SCREEN_HEIGHT - 100;

void updateHills() {
  unsigned long current = millis();

  // Spawn a new hill if timer elapsed
  if (current - lastHillSpawn > HILL_SPAWN_INTERVAL) {
    lastHillSpawn = current;
    for (int i = 0; i < MAX_HILLS; i++) {
      if (hills[i].z <= 0) {
        hills[i].x = random(20, SCREEN_WIDTH - 20);
        hills[i].z = 100;
        score += 10;
        HILL_SPAWN_INTERVAL *= difficulty;
        tft.fillRect(0, 0, SCREEN_WIDTH, 20, BG_Colour);
        break;
      }
    }
  }

  // Update hills z-position
  for (int i = 0; i < MAX_HILLS; i++) {
    if (hills[i].z > 0) hills[i].z -= 2;  // move smoothly towards 0
  }
}

void drawHills() {
  for (int i = 0; i < MAX_HILLS; i++) {
    if (hills[i].z > 0) {
      float size = map(hills[i].z, 100, 0, 5, 35);  // size grows as z → 0
      int x = hills[i].x;
      int y = horizonY + (100 - hills[i].z);  // y position above horizon
      tft.fillTriangle(x, y - (1.5 * size), x - size, y, x + size, y, FG2_Colour);
    }
  }
}


struct Star {
  float x, y, speed;
};

Star stars[MAX_STARS];

float shipX, shipY;
float shipAngle;  // tilt angle in radians
float shipVelX, shipVelY;
float thrustPower = 0.15;
float friction = 0.95;  // friction for smooth movement

// Colors
uint16_t shipColor, starColor;

void setupArcade() {
  tft.fillScreen(BG_Colour);

  shipX = SCREEN_WIDTH / 2;
  shipY = SCREEN_HEIGHT - 20;
  shipAngle = 0;
  shipVelX = 0;
  shipVelY = 0;

  shipColor = FG1_Colour;
  starColor = FG2_Colour;

  // Initialize stars
  for (int i = 0; i < MAX_STARS; i++) {
    stars[i].x = random(SCREEN_WIDTH);
    stars[i].y = random(SCREEN_HEIGHT) + 20;
    stars[i].speed = 0.5 + random(10) / 5.0;
  }

  // Reset hills
  memset(hills, 0, sizeof(hills));

  // Reset game state
  gameOver = false;
  score = 0;
  arcadeStartTime = millis();

  HILL_SPAWN_INTERVAL = 1500;
}


bool checkShipHillCollision() {
  if (gameOver) return false;  // ignore if already over

  for (int i = 0; i < MAX_HILLS; i++) {
    if (hills[i].z > 5 && hills[i].z < 10) {  // collision only when close
      float hillSize = map(hills[i].z, 100, 0, 5, 35);
      if (shipX > hills[i].x - hillSize && shipX < hills[i].x + hillSize) {
        gameOver = true;  // trigger game over
        tft.fillScreen(BG_Colour);
        return true;
      }
    }
  }
  return false;
}

void updateArcade() {
  if (checkReturnToMenu()) return;

  if (!gameOver) {
    // --- Ship input ---
    if (digitalRead(BUTTON_LEFT) == LOW) shipVelX -= 0.3;
    if (digitalRead(BUTTON_RIGHT) == LOW) shipVelX += 0.3;

    // Apply velocity and friction
    shipX += shipVelX;
    shipY += shipVelY;
    shipVelX *= friction;
    shipVelY *= friction;

    // Clamp ship inside screen
    if (shipX < 10) shipX = 10;
    if (shipX > SCREEN_WIDTH - 10) shipX = SCREEN_WIDTH - 10;
    if (shipY < 10) shipY = 10;
    if (shipY > SCREEN_HEIGHT - 20) shipY = SCREEN_HEIGHT - 20;

    // Tilt ship
    shipAngle = shipVelX * 0.2;

    // Update stars & hills
    for (int i = 0; i < MAX_STARS; i++) {
      stars[i].y += stars[i].speed;
      if (stars[i].y > SCREEN_HEIGHT - 100) {
        stars[i].y = 20;
        stars[i].x = random(SCREEN_WIDTH);
      }
    }
    updateHills();

    // Check collision
    if (checkShipHillCollision()) {
      tft.fillScreen(BG_Colour);
    }
  } else {
    // Game Over screen
    tft.setTextSize(3);
    tft.setTextColor(FG1_Colour);
    tft.setCursor(50, SCREEN_HEIGHT / 2 - 40);
    tft.print("GAME OVER");

    tft.setTextSize(2);
    tft.setCursor(SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 20);
    tft.print("Press X");
    tft.setCursor(SCREEN_WIDTH / 2 - 50, SCREEN_HEIGHT / 2 + 40);
    tft.print("to restart");

    // Wait for ENTER to restart
    if (digitalRead(BUTTON_ENTER) == LOW) {
      setupArcade();                    // reset everything
      memset(hills, 0, sizeof(hills));  // clear hills
      gameOver = false;
    }
    return;
  }

  drawArcade();
}

void drawArcade() {
  const int horizonY = SCREEN_HEIGHT - 100;

  // Clear screen
  tft.fillRect(0, 20, SCREEN_WIDTH, SCREEN_HEIGHT, BG_Colour);

  // Draw score
  tft.setTextSize(2);
  tft.setTextColor(FG1_Colour);
  tft.setCursor(5, 5);
  tft.print("Score: ");
  tft.print(score);


  // Draw stars
  for (int i = 0; i < MAX_STARS; i++) {
    tft.fillRect((int)stars[i].x, (int)stars[i].y, 2, 2, starColor);
  }

  // Draw horizon and hills
  tft.drawLine(0, horizonY, SCREEN_WIDTH, horizonY, FG2_Colour);
  drawHills();

  // Draw ship
  float rad = shipAngle + radians(90);
  int x1 = shipX + cos(rad) * 10;
  int y1 = shipY + sin(rad) * 10;
  int x2 = shipX + cos(rad + radians(140)) * 8;
  int y2 = shipY + sin(rad + radians(140)) * 8;
  int x3 = shipX + cos(rad - radians(140)) * 8;
  int y3 = shipY + sin(rad - radians(140)) * 8;

  tft.drawLine(x1, y1, x2, y2, shipColor);
  tft.drawLine(x2, y2, x3, y3, shipColor);
  tft.drawLine(x3, y3, x1, y1, shipColor);


  delay(20);
}

// ================== Platformer ============

// =========================
// PLATFORMER GAME
// =========================

// ---- Player sprite palette (RGB565) ----
uint16_t platformerPalette[] = {
  ST77XX_BLACK,   // 0 = transparent / empty
  ST77XX_RED,     // 1 = body
  ST77XX_YELLOW,  // 2 = face
  ST77XX_BLUE,    // 3 = clothing
  ST77XX_WHITE    // 4 = highlight
};

// ---- Micro-pixel size ----
const int px = 3;

// ---- Hardcoded sprite map ----
// (values correspond to palette indices)
//
// 12 × 12 pixel sprite (each pixel = 3x3 on screen)
// You can modify this to change how the player looks.
const uint8_t playerSprite[12][12] = {
  {0,0,0,1,1,1,0,0,0,0,0,0},   // hat top
  {0,0,1,1,1,1,1,0,0,0,0,0},   // hat brim
  {0,4,2,2,2,2,2,4,0,0,0,0},   // face + hair
  {0,4,2,5,2,5,2,4,0,0,0,0},   // face with eyes
  {0,4,2,4,4,4,2,4,0,0,0,0},   // mustache
  {0,1,1,1,1,1,1,1,0,0,0,0},   // red shirt/chest
  {1,1,3,1,1,1,3,1,1,0,0,0},   // shirt + overall straps
  {1,3,3,3,3,3,3,3,3,1,0,0},   // overalls body
  {0,3,3,3,3,3,3,3,3,0,0,0},   // overalls lower
  {0,4,4,0,0,0,0,4,4,0,0,0},   // shoes
  {0,4,4,0,0,0,0,4,4,0,0,0},   // shoes (second row)
  {0,0,0,0,0,0,0,0,0,0,0,0}
};
struct PlatformerPlayer {
  float x, y;
  float vx, vy;
  int width = 12 * px;
  int height = 12 * px;
  bool onGround = false;
} p;

// Basic ground level
int groundY = 120;

// =========================
// INITIALIZATION
// =========================
void setupPlatformer() {
  p.x = 40;
  p.y = groundY - p.height;
  p.vx = 0;
  p.vy = 0;
}

// =========================
// UPDATE LOGIC
// =========================
void updatePlatformer() {

  if (checkReturnToMenu()) return;

  // Horizontal movement
  if (digitalRead(BUTTON_LEFT) == LOW) {
    p.vx = -1.5;
  } else if (digitalRead(BUTTON_RIGHT) == LOW) {
    p.vx = 1.5;
  } else {
    p.vx *= 0.85;  // friction
  }

  // Jump
  if (digitalRead(BUTTON_UP) == LOW && p.onGround) {
    p.vy = -4.0;
    p.onGround = false;
  }

  // Gravity
  p.vy += 0.20;
  if (p.vy > 4) p.vy = 4;

  // Apply movement
  p.x += p.vx;
  p.y += p.vy;

  // Ground collision
  if (p.y + p.height >= groundY) {
    p.y = groundY - p.height;
    p.vy = 0;
    p.onGround = true;
  }

  // Screen bounds
  if (p.x < 0) p.x = 0;
  if (p.x + p.width > tft.width()) p.x = tft.width() - p.width;

  drawPlatformer();
}

// =========================
// DRAW SPRITE FUNCTION
// =========================
void drawPlayerPlatformer() {

  for (int row = 0; row < 12; row++) {
    for (int col = 0; col < 12; col++) {

      uint8_t colorIndex = playerSprite[row][col];

      if (colorIndex == 0) continue;  // transparent

      uint16_t color = platformerPalette[colorIndex];

      tft.fillRect(
        p.x + col * px,
        p.y + row * px,
        px,
        px,
        color);
    }
  }
}

// =========================
// MAIN DRAW FUNCTION
// =========================
void drawPlatformer() {
  tft.fillScreen(BG_Colour);

  // Draw ground
  tft.fillRect(0, groundY, tft.width(), 4, FG2_Colour);

  drawPlayerPlatformer();
}


// ================== Main ==================

unsigned long blinkTimer = 0;
bool blinkState = false;

void setup() {
  Serial.begin(115200);
  tft.init(SCREEN_WIDTH, SCREEN_HEIGHT);
  tft.setRotation(0);
  tft.fillScreen(BG_Colour);
  tft.invertDisplay(false);
  setupButtons();
  nextShapeIdx = -1;
  nextColor = 0;
  drawMenu();
}

void loop() {
  switch (currentGame) {
    case MENU:
      {
        if (millis() - blinkTimer > 500) {
          blinkTimer = millis();
          blinkState = !blinkState;
          drawMenu(blinkState);
        }

        if (digitalRead(BUTTON_UP) == LOW) {
          selectedGame--;
          if (selectedGame < 0) selectedGame = TOTAL_SELECTIONS - 1;
          drawMenu(false);
          blinkState = true;
          blinkTimer = millis();
          drawMenu(blinkState);
          delay(300);
        }
        if (digitalRead(BUTTON_DOWN) == LOW) {
          selectedGame++;
          if (selectedGame > TOTAL_SELECTIONS - 1) selectedGame = 0;
          drawMenu(false);
          blinkState = true;
          blinkTimer = millis();
          drawMenu(blinkState);
          delay(300);
        }
        if (digitalRead(BUTTON_ENTER) == LOW) {
          tft.fillScreen(BG_Colour);
          switch (selectedGame) {
            case 0:
              setupPong();
              currentGame = PONG;
              break;
            case 1:
              setupSnake();
              currentGame = SNAKE;
              break;
            case 2:
              setupTetris();
              currentGame = TETRIS;
              break;
            case 6:
              cycleThemes();
              tft.fillScreen(BG_Colour);
              drawMenu(blinkState);
              break;
            case 3:
              setupArcade();
              currentGame = ARCADE;
              break;
            case 4:
              setupPlatformer();
              currentGame = PLATFORMER;
              break;
          }
          delay(200);
        }
        break;
      }
    case PONG: updatePong(); break;
    case SNAKE: updateSnake(); break;
    case TETRIS: updateTetris(); break;
    case ARCADE: updateArcade(); break;
    case PLATFORMER: updatePlatformer(); break;
  }
}
