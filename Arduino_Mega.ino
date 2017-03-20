// Global encoder counters
float EncoderValue[5];
int EncoderPos[5];

// Stepper motor control
int StepperMove[5];

// Encoder pin set-Up
const int NumEncoders = 1;
const int dpInEncoderA[5] = {0, 0, 0, 0, 0};
const int dpInEncoderB[5] = {0, 0, 0, 0, 0};

// Stepper Motor Pin Set-Up
const int NumSteppers = 5;
const int dirPin[5] = {22, 25, 28, 31, 34};
const int stepPin[5] = {23, 26, 29, 32, 35};
const int motorDisablePin[5] = {24, 27, 30, 33, 36};
const int ms1Pin = 5;
const int ms2Pin = 6;
const int ms3Pin = 7;

// Remove slack step
const int RemoveSlack = -50;

// LED Debugging Pin Set-Up

// Demo mode for TENS Mesh
const bool demo = true;
const int relaypin = 46;
const int switchpin = 50;

bool SwitchOn;

// Union for easy conversion between float and bytes
union uf_tag {
  byte b[4];
  float fval;
};

// Union for easy conversion between int and bytes
union ui_tag {
  byte b[2];
  float ival;
};

// Resets encoder pins
static void ResetEncoderPins()
{
  // Rotary encoder input lines
  for (int i = 0; i < NumEncoders; i++) {
    pinMode(dpInEncoderA[i], INPUT);
    digitalWrite(dpInEncoderA[i], HIGH);
    pinMode(dpInEncoderB[i], INPUT);
    digitalWrite(dpInEncoderB[i], HIGH);
  }
}

// Moves all stepper motors 1 step in direction specified
void MoveMotor()
{
  for (int i = 0; i < NumSteppers; i++) {
    if (StepperMove[i] > 0) {
      digitalWrite(dirPin[i], HIGH);
      digitalWrite(stepPin[i], HIGH);
      StepperMove[i] -= 1;
    }
    else if (StepperMove[i] < 0) {
      digitalWrite(dirPin[i], LOW);
      digitalWrite(stepPin[i], HIGH);
      StepperMove[i] += 1;
    }
  }
  delayMicroseconds(1000);
  for (int i = 0; i < NumSteppers; i++) {
    digitalWrite(stepPin[i], LOW);
  }
  delayMicroseconds(1000);
}

// Checks if encoder values have changed and updates global encoder counters
void UpdateEncoders() {
  int TempPos;
  for (int i = 0; i < NumEncoders; i ++) {
    ReadEncoderValue(i, TempPos);
    if (EncoderPos[i] != TempPos) {
      if (((EncoderPos[i] == 0) && (TempPos == 1)) || ((EncoderPos[i] == 1) && (TempPos == 3)) || ((EncoderPos[i] == 3) && (TempPos == 2)) || ((EncoderPos[i] == 2) && (TempPos == 0)))
        EncoderValue[i] += 1;
      else
        EncoderValue[i] -= 1;
      EncoderPos[i] = TempPos;
    }
  }
}

// Reads specified encoder values
void ReadEncoderValue(int EnNum, int &rotate) {
  rotate = (  (dpInEncoderB[EnNum]) * 2) + digitalRead(dpInEncoderA[EnNum]);
}

// Controls TENS Mesh
void TENSWrite(int Intensity, int ElectrodeIndex, int FingerIndex, int FingerPadIndex, int Sensation) {
  // To be filled.
}

// Partial Controls of TENS Mesh
void TENSWriteIntensity(int Intensity) {
  int MessageLength = 2;
  byte Message[MessageLength + 1];
  ui_tag u;
  u.ival = Intensity;

  Message[0] = (byte)MessageLength;
  Message[1] = u.b[0];
  Message[2] = u.b[1];
  SendTENSBytes(Message);
}


void toggleMotorLED(int fingerIndex, bool enable) {
  int ledPin = 0;
  if (fingerIndex == 0) {
    ledPin = 2;
  } else if (fingerIndex == 1 ) {
    ledPin = 4;
  } else if (fingerIndex == 2) {
    ledPin = 6;
  } else if (fingerIndex == 3) {
    ledPin = 8;
  } else if (fingerIndex == 4) {
    ledPin = 10;
  }
  digitalWrite(ledPin, enable);
}


void toggleTENSLED(int fingerIndex, bool enable) {
  int ledPin = 0;
  if (fingerIndex == 0) {
    ledPin = 3;
  } else if (fingerIndex == 1 ) {
    ledPin = 5;
  } else if (fingerIndex == 2) {
    ledPin = 7;
  } else if (fingerIndex == 3) {
    ledPin = 9;
  } else if (fingerIndex == 4) {
    ledPin = 11;
  }
  digitalWrite(ledPin, enable);
}

void toggleAllTENSLED(bool enable) {
  for (int i = 0; i <= 4; i++) {
    toggleTENSLED(i, enable);
  }
}


// Disables/Enables Motors
void MotorWrite(int FingerIndex, bool Enable) {
  if (SwitchOn) {
    return;
  }
  // Enable = true means motor on (in contact with object)
  if (Enable) {
    digitalWrite(motorDisablePin[FingerIndex], LOW);
    toggleMotorLED(FingerIndex, Enable);

    if (demo && FingerIndex == 1) {
      digitalWrite(relaypin, HIGH);

      // TENS LEDs
      toggleAllTENSLED(HIGH);
    }
  }
  else if (!Enable) {
    digitalWrite(motorDisablePin[FingerIndex], HIGH);
    toggleMotorLED(FingerIndex, Enable);

    if (demo && FingerIndex == 1) {
      digitalWrite(relaypin, LOW);

      //TENS LEDs
      toggleAllTENSLED( LOW);
    }
  }
}


// Packages Encoder values to send to Unity
void EncoderRead() {
  int MessageLength = NumEncoders * 4;
  byte Message[MessageLength + 1];
  uf_tag u;
  int n;

  Message[0] = (byte)MessageLength;
  n = 1;
  for (int i = 0; i < NumEncoders; i++) {
    u.fval = EncoderValue[i];
    Message[n] = u.b[0];
    Message[n + 1] = u.b[1];
    Message[n + 2] = u.b[2];
    Message[n + 3] = u.b[3];
    n += 4;
  }
  SendUnityBytes(Message);
}

// Sends response to Unity
void SendUnityBytes(byte SendMessage[]) {
  Serial.write(SendMessage, sizeof(SendMessage));
  Serial.flush();
  delay(20);
}

// Sends message to Arduino board controlling TENS Mesh
void SendTENSBytes(byte SendMessage[]) {
  Serial1.write(SendMessage, sizeof(SendMessage));
  Serial1.flush();
  delay(20);
}

// Checks for Message from Unity and then calls appropriate function
void CheckForMessage() {
  byte ReadMessage[10];
  int MessageLength;
  memset(ReadMessage, 0, sizeof(ReadMessage));
  if (Serial.available() > 0) {
    MessageLength = (int)Serial.read();
    Serial.readBytes(ReadMessage, MessageLength);

    if (ReadMessage[0] == 1 && demo == false) {
      ui_tag u;
      u.b[0] = ReadMessage[5];
      u.b[1] = ReadMessage[6];
      TENSWrite(u.ival, (int)ReadMessage[1], (int)ReadMessage[2], (int)ReadMessage[3], (int)ReadMessage[4]);
    }
    else if (ReadMessage[0] == 2) {
      int motorIndex = 0;
      switch ((int)ReadMessage[1]) {
        case 0:
          motorIndex = 4;
          break;
        case 1:
          motorIndex = 3;
          break;
        case 2:
          motorIndex = 2;
          break;
        case 3:
          motorIndex = 1;
          break;
        case 4:
          motorIndex = 0;
          break;
        default:
          motorIndex = 0;

      }
      MotorWrite(motorIndex, (bool)ReadMessage[2]);
    }
    else if (ReadMessage[0] == 3) {
      EncoderRead();
    }
  }
}

void setup() {
  // Start Serial Connection
  Serial.begin(9600);
  Serial1.begin(9600);

  //LEDs
  for (int i = 2; i <= 13; i++) {
    pinMode(i, OUTPUT);
  }

  // Turn on main green LED for mega
  digitalWrite(12, HIGH);

  pinMode(relaypin, OUTPUT);
  pinMode(switchpin, INPUT);

  // Encoder Set-Up
  ResetEncoderPins();

  // Reset Global Encoder Values
  memset(EncoderValue, 0, sizeof(EncoderValue));
  memset(StepperMove, 0, sizeof(StepperMove));
  for (int i = 0; i < 5; i++) {
    ReadEncoderValue(i, EncoderPos[i]);
  }

  // Stepper Motor Set-Up (000 = Full, 100 = Half, 010 = Quarter, 110 = Eighth, 111 = Sixteenth)
  pinMode(ms1Pin, OUTPUT);
  pinMode(ms2Pin, OUTPUT);
  pinMode(ms3Pin, OUTPUT);
  digitalWrite(ms1Pin, LOW);
  digitalWrite(ms1Pin, LOW);
  digitalWrite(ms1Pin, LOW);
  for (int i = 0; i < NumSteppers; i++) {
    pinMode(stepPin[i], OUTPUT);
    pinMode(dirPin[i], OUTPUT);
    pinMode(motorDisablePin[i], OUTPUT);
  }
  for (int i = 0; i < NumSteppers; i++) {
    digitalWrite(motorDisablePin[i], HIGH);
  }
  SwitchOn = false;
}

void loop() {
  CheckForMessage();
  MoveMotor();
  if (SwitchOn == false && digitalRead(switchpin) == true) {
    for (int i = 0; i < NumSteppers; i++) {
      digitalWrite(motorDisablePin[i], LOW);
      toggleMotorLED(i, HIGH);
    }
    digitalWrite(relaypin, HIGH);
    toggleAllTENSLED(HIGH);
    SwitchOn = true;
  }
  else if (SwitchOn == true && digitalRead(switchpin) == false) {
    for (int i = 0; i < NumSteppers; i++) {
      digitalWrite(motorDisablePin[i], HIGH);
      toggleMotorLED(i, LOW);
    }
    toggleAllTENSLED(LOW);
    digitalWrite(relaypin, LOW);
    SwitchOn = false;
  }
}

