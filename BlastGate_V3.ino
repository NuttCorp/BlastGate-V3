//Author: NuttCorp.

const bool DebugMode = false;
const bool DebugWithoutRadio = false;

/////GateID Declrations BEGIN
/////This will pull from the 5bit Gate ID pins and hold the value for reference
  int ID_Pin_0 = A0; // Position 1
  int ID_Pin_1 = A1; // Position 2
  int ID_Pin_2 = A2; // Position 3
  int ID_Pin_3 = A3; // Position 4
  int ID_Pin_4 = A4; // Position 5

/////Pin tied to the pot to set the max open position
  int MOP_Pin = A7; 
  
  struct DATARAW
  {
    byte b0: 1;
    byte b1: 1;
    byte b2: 1;
    byte b3: 1;
    byte b4: 1;
    byte b5: 1;
    byte b6: 1;
    byte b7: 1;
  };
  
  union DATA
  {
    DATARAW rawdata;
    byte value;
  };
  
  DATA data;

  uint8_t GateID(){
    return data.value;
  }
/////GateID Declrations END

/////Servo Declarations BEGIN
#include <Servo.h>
Servo BGservo;  
int maxOpen =  115;
int pos = 0;
/////Servo Declarations ENDe

/////Radio Declarations BEGIN
#include <NRFLite.h>
const static uint8_t RADIO_ID = 60;
const static uint8_t PIN_RADIO_CE = 9;
const static uint8_t PIN_RADIO_CSN = 10;

struct RadioPacket // Any packet up to 32 bytes can be sent.
{
    uint8_t GateID;
    uint8_t GateStatus;
};

NRFLite _radio;
RadioPacket _SentRadioData;
RadioPacket _RecRadioData;
/////Radio Declarations BEGIN

/////General Declarations BEGIN
int PrebuttonState = 0;
int ButtonState = 0;
int GateState = 0;
const int CycleButtonPin = 2;     // the number of the pushbutton pin (needs to be a interrupt pin like D2)
const int R_LED_Pin =  6;      // the number of the LED pin
const int G_LED_Pin =  8;      // the number of the LED pin
const int B_LED_Pin =  7;      // the number of the LED pin
/////General Declarations END

void SetLEDColor(String vColor){
  if (vColor == "RED") {
      digitalWrite(R_LED_Pin, HIGH);
      digitalWrite(G_LED_Pin, LOW);
      digitalWrite(B_LED_Pin, LOW); 
  } else if (vColor == "GREEN") {
      digitalWrite(R_LED_Pin, LOW);
      digitalWrite(G_LED_Pin, HIGH);
      digitalWrite(B_LED_Pin, LOW); 
  } else if (vColor == "BLUE") {
      digitalWrite(R_LED_Pin, LOW);
      digitalWrite(G_LED_Pin, LOW);
      digitalWrite(B_LED_Pin, HIGH); 
  } else if (vColor == "WHITE") {
      digitalWrite(R_LED_Pin, HIGH);
      digitalWrite(G_LED_Pin, HIGH);
      digitalWrite(B_LED_Pin, HIGH); 
  } else {
      digitalWrite(R_LED_Pin, LOW);
      digitalWrite(G_LED_Pin, LOW);
      digitalWrite(B_LED_Pin, LOW);     
  }
}

void openGate() {
  if (isGateOpen() == true) return;
  if (DebugMode) {
    Serial.println("Gate Opening....");
    Serial.print("Max Open Position: ");
    Serial.println(maxOpen);
  }
  SetLEDColor("BLUE");
    // Read potentiometer (0–1023)
  int potVal = analogRead(MOP_Pin);
  // Map pot to 0–180 (servo range)
  maxOpen = map(potVal, 0, 1023, 0, 180);
  for (pos = 0; pos <= maxOpen; pos += 3) 
    {
    BGservo.write(pos);
    delay(15); 
    }
  pos = maxOpen;
  SetLEDColor("GREEN");
}

void closeGate() { 
  if (isGateOpen() == false) return;
  if (DebugMode) {
    Serial.println("Gate Closing....");
  }
  SetLEDColor("BLUE");
  for (pos = maxOpen; pos >= 0; pos -= 3) 
    {
    BGservo.write(pos);
    delay(15); 
    }
  pos = 0;
  SetLEDColor("RED");
}

bool isGateOpen() {
  if (pos == 0) {
    return false;
  } else if (pos >= maxOpen) {
    return true;
  } else {
    return false;
  }
}

void SendStatus() {
   if (DebugMode) {
    Serial.print("Sending Status:");
    Serial.println(isGateOpen());    
  }

  if (DebugWithoutRadio) return; 
  _SentRadioData.GateID = GateID();
  _SentRadioData.GateStatus = isGateOpen();
  
  _radio.send(RADIO_ID, &_SentRadioData, sizeof(_SentRadioData), NRFLite::NO_ACK); // Sends the data with no acknowledge 4 times to be sure everyone gets it. having acknowledge enabled causeing things to act up
  _radio.send(RADIO_ID, &_SentRadioData, sizeof(_SentRadioData), NRFLite::NO_ACK);
  _radio.send(RADIO_ID, &_SentRadioData, sizeof(_SentRadioData), NRFLite::NO_ACK);
  _radio.send(RADIO_ID, &_SentRadioData, sizeof(_SentRadioData), NRFLite::NO_ACK);
}

void CycleGate() {
  if (isGateOpen()) {
    //Closing
    closeGate();
  } else {
    //Opening
    openGate();
  }
   if (DebugMode) {
    Serial.print("Gate Cycled - Gate ID:");
    Serial.print(GateID());
    Serial.print(" | Post Cycle Gate Status:");
    Serial.println(isGateOpen());
  }
}

void imPressed() { //interrupt callback function to set var to tell system the button has been pressed.
  ButtonState = 1;
}

void setup() {
  if (DebugMode) Serial.begin(115200);
  pinMode(R_LED_Pin, OUTPUT);
  pinMode(G_LED_Pin, OUTPUT);
  pinMode(B_LED_Pin, OUTPUT);
  pinMode(CycleButtonPin, INPUT);
  attachInterrupt(digitalPinToInterrupt(CycleButtonPin), imPressed, RISING);
  
  // Pull GateID Data
  pinMode(ID_Pin_0, INPUT_PULLUP); pinMode(ID_Pin_1, INPUT_PULLUP); pinMode(ID_Pin_2, INPUT_PULLUP); pinMode(ID_Pin_3, INPUT_PULLUP); pinMode(ID_Pin_4, INPUT_PULLUP);
  if (digitalRead(ID_Pin_0)) { data.rawdata.b0 = 0; } else { data.rawdata.b0 = 1; }; //Have to invert the pin read since it is using an internal pullup resistor
  if (digitalRead(ID_Pin_1)) { data.rawdata.b1 = 0; } else { data.rawdata.b1 = 1; };
  if (digitalRead(ID_Pin_2)) { data.rawdata.b2 = 0; } else { data.rawdata.b2 = 1; };
  if (digitalRead(ID_Pin_3)) { data.rawdata.b3 = 0; } else { data.rawdata.b3 = 1; };
  if (digitalRead(ID_Pin_4)) { data.rawdata.b4 = 0; } else { data.rawdata.b4 = 1; };

  // Servo Init
  BGservo.attach(5); 
  delay(100); //need to wait after attaching the servo pin or the servo system has a bit of a meltdown

  if (DebugWithoutRadio == false) {
    // Radio Init
    if (!_radio.init(RADIO_ID, PIN_RADIO_CE, PIN_RADIO_CSN, NRFLite::BITRATE250KBPS)) //setting the bitrate to 250 kbps ensures that the narrowest bandwidth is used which greatly increaces range and reliability
    {
        if (DebugMode) Serial.println("Cannot communicate with radio");
        while (true){
          SetLEDColor("RED");
          delay(200);
          SetLEDColor("NONE");
          delay(200);
        }; // Wait here forever.
    } else {
      if (DebugMode) Serial.println("Radio is UP!");
    }
    _SentRadioData.GateID = GateID();    
  }

  
  if (DebugMode) {
    Serial.print("Gate ID:");
    Serial.println(GateID());
  }
  
  CycleGate();
}


void loop() {
  // Check if the inturupt has triggered and if so cycle state. also cleanup debounce variable if no longer pressed
  if (ButtonState == 1 && PrebuttonState == 0) {
    PrebuttonState = 1;
    CycleGate();
    SendStatus(); //Only send status if the cycle was done by a button press
  } 
  
  if (digitalRead(CycleButtonPin) == 0 && PrebuttonState == 1) {
    PrebuttonState = 0;
    ButtonState = 0;
    delay(200);
  } 

  if (DebugWithoutRadio == false) {
    while (_radio.hasData()) {
      _radio.readData(&_RecRadioData);
      if (DebugMode) {
        Serial.print("Data Received:  Gate ID:");
        Serial.print(_RecRadioData.GateID);
        Serial.print("   Remote Gate Status:");
        Serial.print(_RecRadioData.GateStatus);
        Serial.print("   Local Gate Status:");
        Serial.println(isGateOpen());
      }
      if (_RecRadioData.GateID == GateID() && _RecRadioData.GateStatus == 1 && isGateOpen() == false) {
        if (DebugMode) Serial.println("Another Gate with my ID is opening and I am closed. I will open too");
        openGate();
      } else if (_RecRadioData.GateID == GateID() && _RecRadioData.GateStatus == 0 && isGateOpen() == true) {
        if (DebugMode) Serial.println("Another Gate with my ID is closing and I am open. I will close too");
        closeGate();
      } else if (_RecRadioData.GateID != GateID() && isGateOpen() == true) {
        if (DebugMode) Serial.println("A Gate with a different ID is cycling. I am going to close.");
        closeGate();
      }
    }    
  }
 
    
}
