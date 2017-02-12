#include <SoftwareSerial.h> // For Bluetooth, HP-06
#include <Wire.h>
#include <IRremote.h>

#define LED_PIN 12
#define SWITCH_PIN 2
#define RECV_PIN  11
#define SLAVE_ADDRESS 0x04
#define DEBUG_ENABLE 1
SoftwareSerial BTSerial(3, 4);
IRrecv irrecv(RECV_PIN);
boolean recording = true;
decode_results results;

typedef enum {
  DS_NONE = 0,
  DS_BLUETOOTH = 1,
  DS_SWITCH = 2,
  DS_IR = 3
}DeviceStatus;

boolean BT_STATUS = false;
boolean SWITCH_STATUS = false;
boolean IR_STATUS = false;
boolean fFeeding = false;
  
void setup() {
  // Serial For Debugging
  Serial.begin(9600);
  
  // LED
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  delay(2000);
  // Bluetooth
  BTSerial.begin(9600);  
  
  // Switch
  pinMode(SWITCH_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(SWITCH_PIN), triggerSwitchEve, RISING);

  // EV3 Connection I/F
  Wire.begin(SLAVE_ADDRESS);
  Wire.onRequest(sendData_I2C);
  
  // IR Receiver
  irrecv.enableIRIn(); // Start the receiver

  digitalWrite(LED_PIN, LOW);
  Serial.println("Start Pet Feeder V1.0"); 
}

void setStatus(boolean value)
{
  fFeeding = value;
}

boolean getStatusFlag()
{
  return fFeeding;
}

boolean checkBTData() {
  String myString="";
  while(BTSerial.available())  //mySerial에 전송된 값이 있으면
  {
    char myChar = (char)BTSerial.read();  // convert data from int to char
    myString+=myChar;   //Copy consecutive data
    delay(5);           // Protect diconnection
  }
  if(!myString.equals(""))  // If value is not empty
  {
    Serial.println("input value: "+myString); // For debugging
    if(myString.equals("FEED")) {
      Serial.println("FEEDING"); 
      if (!getStatusFlag()) {
        setStatus(true);
        digitalWrite(LED_PIN, HIGH);
        BT_STATUS = true;
      }
      myString="";  // Initialize
    }
  }
  return BT_STATUS;
}

void triggerSwitchEve()
{
  Serial.println("Button Pressed");
  if (!getStatusFlag()) {
    setStatus(true);
    digitalWrite(LED_PIN, HIGH);
    SWITCH_STATUS = true;
  }
}

boolean translateIR()
{
  boolean fStatus = false;
  switch(results.value)
  {
  case 0xFF30CF:  
    Serial.println("button 1 ");
    fStatus = true; 
    break;

  default: 
    Serial.println(" other button   ");
  }
  delay(500);
  return fStatus;
}
boolean checkIRData() {
  if (irrecv.decode(&results)) {  
    if(translateIR()){
      irrecv.resume();
      if(!getStatusFlag()) {
        setStatus(true);
        digitalWrite(LED_PIN, HIGH);
        IR_STATUS = true;
        return IR_STATUS;
      } 
    }
    irrecv.resume();
  }
  return false;
}

void clearAllDevStatus() {
  if(BT_STATUS) BT_STATUS = false;
  if(SWITCH_STATUS) SWITCH_STATUS = false;
  if(IR_STATUS) IR_STATUS = false;
}

// callback for sending data
void sendData_I2C()
{
#if DEBUG_ENABLE
  Serial.println("Calling from EV3 which request data");
#endif
  if(getStatusFlag() == true) {
     Wire.write("1");
     setStatus(false);
     clearAllDevStatus();
     digitalWrite(LED_PIN, LOW);
  }
}

DeviceStatus checkInputMethod() {
  DeviceStatus enStatus = DS_NONE;
  
  if (checkBTData()) {
   enStatus = DS_BLUETOOTH;  
  }
  else if (SWITCH_STATUS) {
    enStatus = DS_SWITCH;
  }
  else if (checkIRData()) {
    enStatus = DS_IR;
  }
  else {
    enStatus =  DS_NONE;
  }
  return enStatus;
}

void loop() {
  DeviceStatus methodStatus = DS_NONE;
  // check which device makes trigger for feeding
  methodStatus = checkInputMethod();
#if DEBUG_ENABLE
  //Serial.print("MethodStatus = ");
  //Serial.println(methodStatus);
#endif
}
