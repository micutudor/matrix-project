#include <LedControl.h>
#include <LiquidCrystal.h>
#include <EEPROM.h>

LiquidCrystal lcd(9, 8, 7, 3, 5, 4);

int snakeHead[2] = {1, 5};
int snakeBody[56][2] = {{0, 5}, {1, 5}};
int snakeDirection[2] = {1, 0};
int snakeLength = 2;

int appleRow = (int) random(0, 8); 
int appleColumn = (int) random(0, 8);

const int DIN = 12;
const int CS = 10;
const int CLK = 11;

LedControl lc = LedControl(DIN, CLK, CS,1);

const int xPin = A1;
const int yPin = A0;
const int switchPin = A2;

byte rows[8] = {0, 0, 0, 0, 0, 0, 0, 0};

float oldTime = 0;
float timer = 0;
float updateRate = 3;

int const buzzerPin = 2;

bool introWasShowed = false;

int currentPageMaxItems = 4;
bool currentPageHasBackButton = false;
bool currentIsReadOnly = false;

const int MIN_TRESHOLD = 200;
const int MAX_TRESHOLD = 600;
const byte MOVEMENT_INTERVAL = 100;

int currentlyViewingMenu = 0;
int currentlyHovering = 1;

boolean switchState = false;

unsigned long lastMovementTime = 0;

int currentLCDBrightness;
int currentMatrixBrightness;
bool currentSoundIsOn;

bool isPlaying = false;
int currentScore = 0;
int currentLives = 3;

bool showApple = true;

void light(int toggle) {
  for(int i = 0; i < 8; i ++)
    lc.setRow(0, i, (toggle << 4));
}

void setup() {
  currentLCDBrightness = EEPROM.read(0);
  currentMatrixBrightness = EEPROM.read(1);
  currentSoundIsOn = (bool)EEPROM.read(2);

  lcd.begin(16, 2);
  lcd.setCursor(0, 1);
  lc.shutdown(0, false);

  analogWrite(6, currentLCDBrightness * 51);
  lc.setIntensity(0, currentMatrixBrightness); // 0 - 10

  lc.clearDisplay(0);
  Serial.begin(9600);

  pinMode(xPin, INPUT);
  pinMode(yPin, INPUT);
  pinMode(switchPin, INPUT_PULLUP);
  pinMode(buzzerPin, OUTPUT);
}

void runGame() {
  float deltaTime = elapsedTime();
  timer += deltaTime;

  //Check For Inputs
  int x = analogRead(xPin);
  int y = analogRead(yPin);
  
  if (x < MIN_TRESHOLD && snakeDirection[1] == 0) 
  {
    snakeDirection[0] = 0;
    snakeDirection[1] = 1;
  } 
  else if (x > MAX_TRESHOLD && snakeDirection[1] == 0)
  {
    snakeDirection[0] = 0;
    snakeDirection[1] = -1;
  }
  else if (y < MIN_TRESHOLD && snakeDirection[0] == 0)
  {
    snakeDirection[0] = -1;
    snakeDirection[1] = 0;
  }
  else if (y > MAX_TRESHOLD && snakeDirection[0] == 0)
  {
    snakeDirection[0] = 1;
    snakeDirection[1] = 0;
  }
  
  //Update
  if (timer > 1000 / updateRate) {
    timer = 0;
    
    Update();
  }
  
  for(int i = 0; i < 8; i ++)
    lc.setRow(0, i, rows[i]);
}

void showIntro() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Snake");
  lcd.setCursor(0, 1);
  lcd.print("by Tudor Micu");
  
  if (currentSoundIsOn) {
    for (int i = 5; i >= 0; i --)
    {
      tone(buzzerPin, i * 100);
      delay(1000);
    }

    noTone(buzzerPin);
  }
  else
    delay(5000);

  lcd.clear();

  introWasShowed = true;
}

void handlePageScrolling() {
  int y = analogRead(yPin);
  int x = analogRead(xPin);
  boolean currentSwitchState = digitalRead(switchPin);

  if (isPlaying)
    return;
  
  // derulam in jos
  if (y > MAX_TRESHOLD)
  {
    if (currentlyHovering - 1 < 1) // asiguram derularea circulara
      currentlyHovering = currentPageMaxItems; // ii dam ultima pagina
    else
      currentlyHovering --;

    if (currentSoundIsOn) {
      tone(buzzerPin, 100);
      delay(100);
      noTone(buzzerPin);
    }
  }
  else if (y < MIN_TRESHOLD) // aceeasi marie cu cozorocul intors
  {
    if (currentlyHovering + 1 > currentPageMaxItems)
      currentlyHovering = 1;
    else
      currentlyHovering ++;

    if (currentSoundIsOn) {
      tone(buzzerPin, 100);
      delay(100);
      noTone(buzzerPin);
    }
  }

  if (currentlyHovering == 1) {
    if (x > MAX_TRESHOLD)
    {
      if (currentlyViewingMenu == 21) {
        if (currentLCDBrightness < 5)
            currentLCDBrightness ++;
      } else if (currentlyViewingMenu == 22) {
        if (currentMatrixBrightness < 10)
            currentMatrixBrightness ++;
      } else if (currentlyViewingMenu == 23) {
        currentSoundIsOn = !currentSoundIsOn;
      }

      if (currentSoundIsOn) {
        tone(buzzerPin, 100);
        delay(100);
        noTone(buzzerPin);
      }
    }
    else if (x < MIN_TRESHOLD)
    {
      if (currentlyViewingMenu == 21) {
        if (currentLCDBrightness > 1)
          currentLCDBrightness --;
      } else if (currentlyViewingMenu == 22) {
        if (currentMatrixBrightness > 1)
          currentMatrixBrightness --;
      } else if (currentlyViewingMenu == 23) {
        currentSoundIsOn = !currentSoundIsOn;
      }

      if (currentSoundIsOn) {
        tone(buzzerPin, 100);
        delay(100);
        noTone(buzzerPin);
      }
    }

    EEPROM.write(0, currentLCDBrightness);
    EEPROM.write(1, currentMatrixBrightness);
    EEPROM.write(2, (int) currentSoundIsOn);

    analogWrite(6, currentLCDBrightness * 51); // 0 - 255
    lc.setIntensity(0, currentMatrixBrightness); // 0 - 10
  }

  // la click pastram pe ce hoveruim
  if (currentSwitchState == LOW) 
    switchState = !switchState;

  light(currentlyViewingMenu == 22 && currentlyHovering == 1);

  if (millis() - lastMovementTime >= MOVEMENT_INTERVAL) 
  {
    if (switchState) {
      if (currentlyViewingMenu == 11) {
        if (currentlyHovering == 2) {
          isPlaying = true;
          currentScore = 0;
          currentLives = 3;

          currentlyViewingMenu = 1;
          currentlyHovering = 1;
        }
        else if (currentlyHovering == 3)
        {
          currentlyViewingMenu = 0;
          currentlyHovering = 1;
        }
      }
      else {
        if (currentPageHasBackButton && currentlyHovering == currentPageMaxItems)
          currentlyViewingMenu /= 10;
        else if (!currentIsReadOnly) {
          currentlyViewingMenu = currentlyViewingMenu * 10 + currentlyHovering;

          if (currentlyViewingMenu == 1)
          {
            isPlaying = true;
            currentScore = 0;
            currentLives = 3;
          }

          currentlyHovering = 1;
        }
      }

      if (currentSoundIsOn) {
        tone(buzzerPin, 100);
        delay(200);
        noTone(buzzerPin);
      }

      switchState = false;
    }

    lastMovementTime = millis();
  }
}

void showMainMenu() {
  lcd.setCursor(0, 0);

  switch (currentlyHovering) {
    case 1:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Start game");
      lcd.setCursor(0, 1);
      lcd.print(" Settings");
      break;
    }
    case 2:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Settings");
      lcd.setCursor(0, 1);
      lcd.print(" About");
      break;
    }
    case 3:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> About");
      lcd.setCursor(0, 1);
      lcd.print(" How to play");
      break;
    }
    case 4:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> How to play");
      lcd.setCursor(0, 1);
      lcd.print(" Start game");
      break;
    }
  }

  delay(150);
}

void showLCDBrightness() {
  lcd.setCursor(0, 0);

  switch (currentlyHovering) {
    case 1:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("LCD bright: <");
      lcd.print(currentLCDBrightness);
      lcd.print(" >");
      lcd.setCursor(0, 1);
      lcd.print("Back");
      break;
    }
    case 2:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Back");
      lcd.setCursor(0, 1);
      lcd.print("LCD bright: <");
      lcd.print(currentLCDBrightness);
      lcd.print(" >");
      break;
    }
  }

  delay(150);
}

void showMatrixBrightness() {
  lcd.setCursor(0, 0);

  switch (currentlyHovering) {
    case 1:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("M bright: < ");
      lcd.print(currentMatrixBrightness);
      lcd.print(" >");
      lcd.setCursor(0, 1);
      lcd.print("Back");
      break;
    }
    case 2:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Back");
      lcd.setCursor(0, 1);
      lcd.print("M bright: < ");
      lcd.print(currentMatrixBrightness);
      lcd.print(" >");
      break;
    }
  }

  delay(150);
}

void showSound() {
  lcd.setCursor(0, 0);

  switch (currentlyHovering) {
    case 1:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sound: ");
      lcd.print((currentSoundIsOn) ? "on" : "off");
      lcd.setCursor(0, 1);
      lcd.print("Back");
      break;
    }
    case 2:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Back");
      lcd.setCursor(0, 1);
      lcd.print("Sound: ");
      lcd.print((currentSoundIsOn) ? "on" : "off");
      break;
    }
  }

  delay(150);
}

void showSettings() {
  lcd.setCursor(0, 0);

  switch (currentlyHovering) {
    case 1:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> LCD Brightness");
      lcd.setCursor(0, 1);
      lcd.print(" Mat brightness");
      break;
    }
    case 2:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Mat brightness");
      lcd.setCursor(0, 1);
      lcd.print(" Sound");
      break;
    }
    case 3:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Sound");
      lcd.setCursor(0, 1);
      lcd.print(" Back");
      break;
    }
    case 4:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Back");
      lcd.setCursor(0, 1);
      lcd.print(" LCD Brightness");
      break;
    }
  }

  delay(150);
}

void showAbout() {
  lcd.setCursor(0, 0);

  switch (currentlyHovering) {
    case 1:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> SNAKE");
      lcd.setCursor(0, 1);
      lcd.print(" (c) Tudor Micu");
      break;
    }
    case 2:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> (c) Tudor Micu");
      lcd.setCursor(0, 1);
      lcd.print(" Git: micutudor");
      break;
    }
    case 3:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Git: micutudor");
      lcd.setCursor(0, 1);
      lcd.print(" Back");
      break;
    }
    case 4:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Back");
      lcd.setCursor(0, 1);
      lcd.print(" SNAKE");
      break;
    }
  }

  delay(150);
}

void showHowToPlay() {
  lcd.setCursor(0, 0);

  switch (currentlyHovering) {
    case 1:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Use joystick to");
      lcd.setCursor(0, 1);
      lcd.print("move the snake");
      break;
    }
    case 2:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Eat apples to");
      lcd.setCursor(0, 1);
      lcd.print("grow & score up");
      break;
    }
    case 3:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Be careful: do");
      lcd.setCursor(0, 1);
      lcd.print("not eat yourself");
      break;
    }
    case 4:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Back");
      break;
    }
  }

  delay(150);
}

void showGameScreen() {
  lcd.setCursor(0, 0);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Score: ");
  lcd.print(currentScore);
  lcd.setCursor(0, 1);
  lcd.print("Lives: ");
  lcd.print(currentLives);

  delay(150);
}

void showDeadScreen() {
  lcd.setCursor(0, 0);

  switch (currentlyHovering) {
    case 1:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Score: ");
      lcd.print(currentScore);
      lcd.setCursor(0, 1);
      lcd.print(" Restart");
      break;
    }
    case 2:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Restart");
      lcd.setCursor(0, 1);
      lcd.print(" Main menu");
      break;
    }
    case 3:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Main menu");
      lcd.setCursor(0, 1);
      lcd.print(" Back");
      break;
    }
    case 4:
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("> Back");
      lcd.setCursor(0, 1);
      lcd.print("Score: ");
      lcd.print(currentScore);
      lcd.print("\n");
      break;
    }
  }

  delay(150);
}

void handleLCDRendering() {
  handlePageScrolling();

  switch (currentlyViewingMenu) {
    case 0: {
      currentIsReadOnly = false;
      currentPageHasBackButton = false;
      currentPageMaxItems = 4;

      showMainMenu();
      break;
    }
    
    case 1: {
      currentIsReadOnly = false;
      currentPageHasBackButton = false;
      currentPageMaxItems = 1;
      
      showGameScreen();
      break;
    }

    case 11: {
      currentIsReadOnly = false;
      currentPageHasBackButton = false;
      currentPageMaxItems = 3;

      showDeadScreen();
      break;
    }

    case 2: {
      currentIsReadOnly = false;
      currentPageHasBackButton = true;
      currentPageMaxItems = 4;

      showSettings();
      break;
    }

    case 3: {
      currentIsReadOnly = true;
      currentPageHasBackButton = true;
      currentPageMaxItems = 4;
      
      showAbout();
      break;
    }

    case 4: {
      currentIsReadOnly = true;
      currentPageHasBackButton = true;
      currentPageMaxItems = 4;
      
      showHowToPlay();
      break;
    }

    case 21: {
      currentIsReadOnly = true;
      currentPageHasBackButton = true;
      currentPageMaxItems = 2;
      
      showLCDBrightness();
      break;
    }

    case 22: {
      currentIsReadOnly = true;
      currentPageHasBackButton = true;
      currentPageMaxItems = 2;
      
      showMatrixBrightness();
      break;
    }

    case 23: {
      currentIsReadOnly = true;
      currentPageHasBackButton = true;
      currentPageMaxItems = 2;
      
      showSound();
      break;
    }
  }
}

void loop() {
  if (introWasShowed)
    handleLCDRendering();
  else
    showIntro();

  if (isPlaying)
    runGame();
}

float elapsedTime() {
  float currentTime = millis();
  float dt = currentTime - oldTime;
  oldTime = currentTime;
  return dt;
}

void Update() {
  for (int j = 0; j < 8; j ++) 
    rows[j] = 0;
  
  int newSnakeHead[2] = { snakeHead[0] + snakeDirection[0], snakeHead[1] + snakeDirection[1] };

  // Teleport to other end
  if (newSnakeHead[0] == 8)
    newSnakeHead[0] = 0;
  else if (newSnakeHead[0] == -1)
    newSnakeHead[0] = 7;
  else if (newSnakeHead[1] == 8)
    newSnakeHead[1] = 0;
  else if (newSnakeHead[1] == -1)
    newSnakeHead[1] = 7;
  
  // Snake eat himself
  for (int i = 0; i < snakeLength; i ++)
  {
    if (snakeBody[i][0] == newSnakeHead[0] && snakeBody[i][1] == newSnakeHead[1])
    {
      if (currentLives - 1 == 0)
      {
        delay(1000);
        
        snakeHead[0] = 1;
        snakeHead[1] = 5;

        snakeBody[0][0] = 0;
        snakeBody[0][1] = 5;
        snakeBody[1][0] = 1;
        snakeBody[1][1] = 5;

        for (int j = 2; j < 56; j ++)
        {
          snakeBody[j][0] = 0;
          snakeBody[j][1] = 0;
        }

        snakeDirection[0] = 1;
        snakeDirection[1] = 0;

        snakeLength = 2;

        appleRow = (int) random(0, 8);
        appleColumn = (int) random(0, 8);

        isPlaying = false;
        currentlyViewingMenu = 11;

        if (currentSoundIsOn) {
          tone(buzzerPin, 500);
          delay(500);
          noTone(buzzerPin);
        }

        return;
      }
      else
        currentLives --;
    }
  }

  // Snake eat the apple
  if (newSnakeHead[0] == appleRow && newSnakeHead[1] == appleColumn)
  {
    snakeLength ++;

    appleRow = (int) random(0,8);
    appleColumn = (int) random(0,8);

    currentScore ++;
    updateRate += 0.25;

    if (currentSoundIsOn) {
      tone(buzzerPin, 250);
      delay(100);
      noTone(buzzerPin);
    }
  }
  else
  {
    // Move
    for (int j = 1; j < snakeLength; j ++)
    {
      snakeBody[j - 1][0] = snakeBody[j][0];
      snakeBody[j - 1][1] = snakeBody[j][1];
    }
  }
  
  snakeBody[snakeLength - 1][0]= newSnakeHead[0];
  snakeBody[snakeLength - 1][1]= newSnakeHead[1];
  
  snakeHead[0] = newSnakeHead[0];
  snakeHead[1] = newSnakeHead[1];
  
  for(int i = 0; i < snakeLength; i ++)
    rows[snakeBody[i][0]] |= 128 >> snakeBody[i][1];
  
  if (showApple) 
  {
    rows[appleRow] |= 128 >> appleColumn; // 128 = 2 ^ 7
    showApple = false;
  } 
  else 
    showApple = true;
  
}
