// http://librarymanager/All#M5StickC https://github.com/m5stack/M5StickC
#include <M5StickC.h> 
#include <WiFi.h>

// Download these from here
// https://github.com/kasperskaarhoj/SKAARHOJ-Open-Engineering/tree/master/ArduinoLibs
#include <SkaarhojPgmspace.h>
#include <ATEMbase.h>
#include <ATEMstd.h>

// Define the IP address of your M5 device
IPAddress clientIp(10, 11, 200, 199);

// Define the IP address of your ATEM switcher
IPAddress switcherIp(10, 11, 200, 1);

// Put your wifi SSID and password here
const char* ssid = "";
const char* password =  "";

// Set this to 1 if you want the orientation to update automatically
#define AUTOUPDATE_ORIENTATION 0

// You can customize the red/green/grey if you want
// http://www.barth-dev.de/online/rgb565-color-picker/
#define GRAY  0x0020 //   8  8  8
#define GREEN 0x0400 //   0 128  0
#define RED   0xF800 // 255  0  0



/////////////////////////////////////////////////////////////
// You probably don't need to change things below this line

ATEMstd AtemSwitcher;

int orientation = 0;
int orientationPrevious = 0;
int orientationMillisPrevious = millis();

int cameraNumber = 1;
int ledPin = 10;

int PreviewTallyPrevious = 1;
int ProgramTallyPrevious = 1;
int cameraNumberPrevious = cameraNumber;

void setup() {
  Serial.begin(9600);

  WiFi.begin(ssid, password);
  M5.MPU6886.Init();

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.println("Connecting to WiFi..");
  }
  Serial.println("Connected to the WiFi network");
    
  // 初期化
  M5.begin();
  M5.Lcd.setRotation(orientation);
  
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);

  AtemSwitcher.begin(switcherIp);
  AtemSwitcher.serialOutput(0x80);
  AtemSwitcher.connect();
  
  // GPIO初期化
  pinMode(26, INPUT); // PIN  (INPUT, OUTPUT, ANALOG)無線利用時にはANALOG利用不可, DAC出力可
  pinMode(36, INPUT); // PIN  (INPUT,       , ANALOG)入力専用、INPUT_PULLUP等も不可
  pinMode( 0, INPUT); // PIN  (INPUT, OUTPUT,       )外部回路でプルアップ済み
  pinMode(32, INPUT); // GROVE(INPUT, OUTPUT, ANALOG)
  pinMode(33, INPUT); // GROVE(INPUT, OUTPUT, ANALOG)
}

int buttonBCount = 0;

float accX = 0;
float accY = 0;
float accZ = 0;

void setOrientation() {
    M5.MPU6886.getAccelData(&accX,&accY,&accZ);
    //Serial.printf("%.2f   %.2f   %.2f \n",accX * 1000, accY * 1000, accZ * 1000);

    if(accZ < .9) {
      if(accX > .6) {
        orientation = 1;
      } else if(accX < .4 && accX > -.5) {
        if(accY > 0) {
          orientation = 0;
        } else {
          orientation = 2;
        }
      } else {
        orientation = 3;
      }
    }
  
    if(orientation != orientationPrevious) {
      Serial.printf("Orientation changed to %d", orientation);
      M5.Lcd.setRotation(orientation);
    }  
}


void loop() {
  // ボタンの状態更新
  M5.update();

  if(AUTOUPDATE_ORIENTATION) {
    if(orientationMillisPrevious + 500 < millis()) {
      setOrientation();
      orientationMillisPrevious = millis();
    }
  }

  if (M5.BtnA.wasPressed()) {
    AtemSwitcher.changeProgramInput(cameraNumber);
  }
  if (M5.BtnB.wasPressed()) {
    setOrientation();
    buttonBCount = millis();
  }

  if(buttonBCount != 0 && buttonBCount < millis() - 500) {
    Serial.println("Changing camera number");
    cameraNumber = (cameraNumber + 1) % 4;
    if(cameraNumber == 0) {
      cameraNumber = 4;
    }
    Serial.print("New camera number: ");
    Serial.println(cameraNumber);

    buttonBCount = 0;
  }

  // Check for packets, respond to them etc. Keeping the connection alive!
  AtemSwitcher.runLoop();

  int ProgramTally = AtemSwitcher.getProgramTally(cameraNumber);
  int PreviewTally = AtemSwitcher.getPreviewTally(cameraNumber);

  if ((orientation != orientationPrevious) || (cameraNumber != cameraNumberPrevious) || (ProgramTallyPrevious != ProgramTally) || (PreviewTallyPrevious != PreviewTally)) { // changed?

    if ((ProgramTally && !PreviewTally) || (ProgramTally && PreviewTally) ) { // only program, or program AND preview
      drawLabel(RED, BLACK, LOW);
    } else if (PreviewTally && !ProgramTally) { // only preview
      drawLabel(GREEN, BLACK, HIGH);
    } else if (!PreviewTally || !ProgramTally) { // neither
      drawLabel(BLACK, GRAY, HIGH);
    }

  }

  ProgramTallyPrevious = ProgramTally;
  PreviewTallyPrevious = PreviewTally;
  cameraNumberPrevious = cameraNumber;
  orientationPrevious  = orientation;
}

void drawLabel(unsigned long int screenColor, unsigned long int labelColor, bool ledValue) {
  digitalWrite(ledPin, ledValue);
  M5.Lcd.fillScreen(screenColor);
  M5.Lcd.setTextColor(labelColor, screenColor);
  if(orientation == 1 || orientation == 3) {
    M5.Lcd.drawString(String(cameraNumber), 55, 2, 8);
  } else {
    M5.Lcd.drawString(String(cameraNumber), 15, 60, 8);
  }
}

