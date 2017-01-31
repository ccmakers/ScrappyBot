#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <Adafruit_PWMServoDriver.h>
#include <OSCMessage.h>
#include <OSCBundle.h>
#include <OSCData.h>

#define MOTOR_A 0
#define MOTOR_B 1

/*** Wifi ***/

//Set these to your desired credentials. These are CASE SENSITIVE.
#define WIFI_CLIENT_ENABLED 1
const char *ap_ssid = "ScrappyNet01";
const char *ap_password = "scrappybot";

// A UDP instance to let us receive packets over UDP
WiFiUDP Udp;

// A UDP port set on TouchOSC
const unsigned int localPort = 8888;

OSCErrorCode error;

/*** Servo ***/
void setMotor(int motor, int speed = 0);
void brakeMotor(int motor);

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();

/*** Setup ***/
void setup() {

  // Initializing serial port for debugging purposes
  Serial.begin(9600);
  delay(5);

  pinMode(D4, OUTPUT);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  pinMode(D7, OUTPUT);
  digitalWrite(D4, LOW);
  digitalWrite(D5, LOW);
  digitalWrite(D6, LOW);
  digitalWrite(D7, LOW);
  pinMode(LED_BUILTIN, OUTPUT);     // Initialize the LED_BUILTIN pin as an output
  Wire.begin(D2, D1); //D2 is SDA, D1 is SCL
  pwm.reset();
  pwm.setPWMFreq(60);
  //Servos 0 and 1 are Motor A, Servos 2 and 3 are Motor B
  pwm.setPWM(0, 0, 4096);//special value for pin fully low
  pwm.setPWM(1, 0, 4096);//special value for pin fully low
  pwm.setPWM(2, 0, 4096);//special value for pin fully low
  pwm.setPWM(3, 0, 4096);//special value for pin fully low

  //Setup WiFi Access Point and start listening Server socket
  // Connecting to WiFi network
  Serial.println();
  Serial.print("Setting up WiFi Access Point ");
  Serial.println(ap_ssid);

  WiFi.mode(WIFI_AP_STA);
  WiFi.hostname("ScrappyBot");
  WiFi.disconnect();
  boolean result = WiFi.softAP(ap_ssid, ap_password);
  while (result != true) {
    Serial.print(".");
  }
  Serial.print(". Done!");
  Serial.println("");
  Serial.println("WiFi connected, local IP address: ");
  Serial.println(WiFi.softAPIP());

  // Starting the web server
  Serial.println("Starting UDP");
  Udp.begin(localPort);
  Serial.print("Local port: ");
  Serial.println(Udp.localPort());

  yield();
}

// the loop function runs over and over again forever
unsigned int loop_count = 0;
int led_state = LOW;
WiFiClient serverClient;

void loop() {

  OSCMsgReceive();

  if ((loop_count % 16384) == 0) {
    if (led_state == LOW) {
      led_state = HIGH;
    } else led_state = LOW;
    digitalWrite(LED_BUILTIN, led_state);
  }
  ++loop_count;
}

/*
* Route the TouchOSC messages to the wheels
*/
void OSCMsgReceive(){
  OSCMessage msgIN;
  int size;
  if((size = Udp.parsePacket())>0){
    while(size--)
      msgIN.fill(Udp.read());
    if(!msgIN.hasError()){
      msgIN.route("/bot/leftWheel", moveLeftWheel);
      msgIN.route("/bot/rightWheel", moveRightWheel);
    }
  }
}

void moveLeftWheel(OSCMessage &msg, int addrOffset) {
  int speed = msg.getFloat(0);
  Serial.print("LeftWheel: ");
  Serial.println(speed);
  setMotor(MOTOR_A, speed);
}

void moveRightWheel(OSCMessage &msg, int addrOffset) {
  int speed = msg.getFloat(0);
  Serial.print("RightWheel: ");
  Serial.println(speed);
  setMotor(MOTOR_B, speed);
}


/*
* Set servo speed and direction
*/
void setMotor(int motor, int speed) {
  int offset = motor << 1;
  //Bound the speed value
  if (speed > 255) speed = 255;
  if (speed < -255) speed = -255;

  if (speed == 0) { //Special case for motor in idle
    pwm.setPWM(0 + offset, 0, 4096); //special value for pin fully low
    pwm.setPWM(1 + offset, 0, 4096); //special value for pin fully low
  } else if (speed > 0) {
    pwm.setPWM(0 + offset, 0, speed << 4);
    pwm.setPWM(1 + offset, 0, 4096); //special value for pin fully low
  } else if (speed < 0) {
    pwm.setPWM(0 + offset, 0, 4096); //special value for pin fully low
    pwm.setPWM(1 + offset, 0, abs(speed) * 16);
  }
}
