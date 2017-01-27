#include <Arduino.h>
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <Adafruit_PWMServoDriver.h>

#define MOTOR_A 0
#define MOTOR_B 1

/*** Wifi ***/

//Set these to your desired credentials. These are CASE SENSITIVE.
#define WIFI_CLIENT_ENABLED 1
const char *ap_ssid = "ScrappyRod";
const char *ap_password = "scrappybot";
const char *client_ssid = "Z3r0";
const char *client_password = "teleport";

IPAddress ip(10, 0, 1, 50);
WiFiServer server(80); // Listening port is 80



/*** Servo ***/
void setMotor(int motor, int speed);
void brakeMotor(int motor);

Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();


/*** Setup ***/
void setup() {
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
  WiFi.mode(WIFI_AP_STA);
  WiFi.hostname("ScrappyBot");
  WiFi.softAP(ap_ssid, ap_password);
  if(WIFI_CLIENT_ENABLED){
    WiFi.begin(client_ssid, client_password);
    delay(2000);//Give time for wifi to connect (if possible)
  }

  server.begin();
  server.setNoDelay(true);

  yield();
}

// the loop function runs over and over again forever
unsigned int loop_count = 0;
int led_state = LOW;
WiFiClient serverClient;

void loop() {

  if ((loop_count % 16384) == 0) {
    if (led_state == LOW) {
      led_state = HIGH;
    } else led_state = LOW;
    digitalWrite(LED_BUILTIN, led_state);
  }

  if (server.hasClient()) {
    serverClient = server.available();
  }

  if (serverClient && serverClient.connected()) {
    if (serverClient.available()) {
      char data_buffer[129];//buffer to store data from/to the remote
      char* tokens[8];
      int num_tokens;
      int bytes_read = serverClient.readBytesUntil('\n', data_buffer, 128);
      data_buffer[bytes_read] = '\0';      //Parse the string from the remote into an array of strings, each word is it own entry in the array
      num_tokens = 0;
      tokens[num_tokens] = strtok(data_buffer, " ");
      while (tokens[num_tokens] != NULL) {
        ++num_tokens;
        tokens[num_tokens] = strtok(NULL, " ");
      } //Parse complete
      //Proccess command
      if (num_tokens >= 1 && strcmp(tokens[0], "PING") == 0) {
        serverClient.println("PONG");
      } else if (num_tokens >= 1 && strcmp(tokens[0], "HALT") == 0) {
        brakeMotor(MOTOR_A);
        brakeMotor(MOTOR_B);
      } else if (num_tokens >= 3 && strcmp(tokens[0], "DRIVE") == 0) {
        setMotor(MOTOR_A, atoi(tokens[2]));
        setMotor(MOTOR_B, atoi(tokens[1]));
      }
    }
  }
  if (serverClient && !serverClient.connected()) {
    serverClient.stop();
  }

  ++loop_count;
}

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

void brakeMotor(int motor) {
  int offset = motor << 1;
  pwm.setPWM(0 + offset, 4096, 0); //special value for pin fully high
  pwm.setPWM(1 + offset, 4096, 0); //special value for pin fully high
}
