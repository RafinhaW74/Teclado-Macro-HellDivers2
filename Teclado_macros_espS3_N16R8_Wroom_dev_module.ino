#include "USB.h"
#include "USBHIDKeyboard.h"

USBHIDKeyboard Keyboard;

/* ================= HARDWARE ================= */

const uint8_t ROWS = 2;
const uint8_t COLS = 3;

const uint8_t rowPins[ROWS] = {1, 2};
const uint8_t colPins[COLS] = {39, 41, 42};

/* ================= STRAT ENUM ================= */

enum StratagemID {
  STRAT_NONE = 0,
  STRAT_REINFORCE,
  STRAT_RESUPPLY,
  STRAT_GUARD_DOG,
  STRAT_ORBITAL_120MM,
  STRAT_EAGLE_AIRSTRIKE,
  STRAT_JUMP_PACK,
  STRAT_COUNT
};

/* ================= SEQUÊNCIAS OFICIAIS ================= */

const char* stratagemSequences[STRAT_COUNT] = {
  "",
  "WSDAW",    // Reinforce
  "SSWD",     // Resupply
  "SWAWDD",   // AX/LAS-5 Rover
  "DDSADS",   // Orbital 120mm
  "WDSD",     // Eagle Airstrike
  "SWWSW"     // Jump Pack
};

/* ================= MAPA DA MATRIZ ================= */

StratagemID macroMap[ROWS][COLS] = {
  { STRAT_REINFORCE, STRAT_RESUPPLY, STRAT_GUARD_DOG },
  { STRAT_ORBITAL_120MM, STRAT_EAGLE_AIRSTRIKE, STRAT_JUMP_PACK }
};

/* ================= ENGINE ================= */

#define KEY_DELAY 40
#define STEP_DELAY 30

bool macroRunning = false;
StratagemID activeStrat = STRAT_NONE;
uint8_t sequenceIndex = 0;
unsigned long actionTimer = 0;
bool ctrlPressed = false;
bool keyPressedState = false;

void startStratagem(StratagemID id) {
  if (macroRunning || id == STRAT_NONE) return;

  activeStrat = id;
  sequenceIndex = 0;
  macroRunning = true;
  actionTimer = 0;
  ctrlPressed = false;
  keyPressedState = false;
}

void updateMacroEngine() {

  if (!macroRunning) return;

  unsigned long now = millis();
  const char* seq = stratagemSequences[activeStrat];

  if (!ctrlPressed) {
    Keyboard.press(KEY_LEFT_CTRL);
    ctrlPressed = true;
    actionTimer = now;
    return;
  }

  if (now - actionTimer < STEP_DELAY) return;

  char key = seq[sequenceIndex];

  if (key == '\0') {
    Keyboard.releaseAll();   // 🔥 segurança extra
    macroRunning = false;
    activeStrat = STRAT_NONE;
    return;
  }

  if (!keyPressedState) {
    Keyboard.press(key);
    keyPressedState = true;
    actionTimer = now;
  } 
  else if (now - actionTimer >= KEY_DELAY) {
    Keyboard.release(key);
    keyPressedState = false;
    sequenceIndex++;
    actionTimer = now;
  }
}

/* ================= MATRIZ ================= */

#define DEBOUNCE_TIME 10

bool stableState[ROWS][COLS] = {false};
bool lastRawState[ROWS][COLS] = {false};
unsigned long lastChangeTime[ROWS][COLS] = {0};

void initMatrix() {

  for (uint8_t r = 0; r < ROWS; r++) {
    pinMode(rowPins[r], OUTPUT);
    digitalWrite(rowPins[r], HIGH);
  }

  for (uint8_t c = 0; c < COLS; c++) {
    pinMode(colPins[c], INPUT_PULLUP);
  }
}

void scanMatrix() {

  unsigned long now = millis();

  for (uint8_t r = 0; r < ROWS; r++) {

    digitalWrite(rowPins[r], LOW);

    for (uint8_t c = 0; c < COLS; c++) {

      bool raw = digitalRead(colPins[c]) == LOW;

      if (raw != lastRawState[r][c]) {
        lastRawState[r][c] = raw;
        lastChangeTime[r][c] = now;
      }

      if ((now - lastChangeTime[r][c]) > DEBOUNCE_TIME) {

        if (raw != stableState[r][c]) {
          stableState[r][c] = raw;
          if (raw) startStratagem(macroMap[r][c]);
        }
      }
    }

    digitalWrite(rowPins[r], HIGH);
  }
}

void setup() {
  initMatrix();
  Keyboard.begin();
  USB.begin();
}

void loop() {
  scanMatrix();
  updateMacroEngine();
}