// core from https://arduino.esp8266.com/stable/package_esp8266com_index.json
// for NodeMCU 0.9
// add a file secret.h with:
// #define TELEGRAM_BOT_TOKEN "XXXX" where XXX is the bot code from botfather
// #define CHAT_ID "-123465" where -123465 is the chat id, from https://api.telegram.org/bot<token>/getUpdates, same token as TELEGRAM_BOT_TOKEN
// constexpr char SSID[] = "xxx"; where xxx is the name of the wifi
// constexpr char PASS[] = "yyy"; where yyy is the password of the wifi

#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

#include "secret.h"

constexpr int pin = D1; //D1 Input button
constexpr int pinOut = D3; //D3 Output light

ESP8266WiFiMulti WiFiMulti;

void setup() {

  Serial.begin(115200);
  Serial.printf("[SETUP] STARTED\n");

  Serial.println();

  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(SSID, PASS);
  Serial.printf("[SETUP] CONNECTED\n");
  
  pinMode(pin, INPUT_PULLUP);
  pinMode(pinOut, OUTPUT);

  digitalWrite(pinOut, HIGH);
  
  Serial.printf("[SETUP] STARTED\n");
}

#define MSG "test"

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define BODY_OPEN "{\"chat_id\": "STR(CHAT_ID)", \"text\": \"The fablab is OPEN\", \"disable_notification\": true}"
#define BODY_CLOSE "{\"chat_id\": "STR(CHAT_ID)", \"text\": \"The fablab is CLOSED\", \"disable_notification\": true}"

uint32_t lastPress = millis();
void loop() {
  
  bool sendMsg = false;
  static bool is_open = false;

  // make sure the output led is aligned with the internal status every loop
  digitalWrite(pinOut, is_open);

  // check if someone pressed the button
  if (!digitalRead(pin)){
    // 6 seconds timeout between button press
    if (millis() - lastPress > 6000){
      lastPress = millis();
      
      // update the output led right away for fast feedback to user
      digitalWrite(pinOut, !is_open);

      // request to send the message about the current status. May fail!
      sendMsg = true;
    }
  }
  
  // wait for a request and WiFi connection
  if (sendMsg && WiFiMulti.run() == WL_CONNECTED) {
    Serial.printf("[SETUP] MESSAGE\n");

    WiFiClientSecure client;
    // eh, screw certificate autentication, i like to live dangerously
    client.setInsecure();

    // hard coded header, stolen directly from CURL
    constexpr char request[] = "POST /bot"TELEGRAM_BOT_TOKEN"/sendMessage HTTP/1.0\r\n" \
    "Host: api.telegram.org\r\n" \
    "user-agent: curl/7.79.1\r\n" \
    "accept: */*\r\n" \
    "content-type: application/json\r\n" \
    "content-length:";
    Serial.print(request);

    // connect over https to telegram server and send the request
    if ( client.connect("api.telegram.org", 443) ){

      // send the header
      client.print(request);

      // switch the status and send the actual request OPEN/CLOSE
      if (!is_open){
        client.println( sizeof(BODY_OPEN) );
        client.println();
        client.println(BODY_OPEN);
      }else{
        client.println( sizeof(BODY_CLOSE) );
        client.println();
        client.println(BODY_CLOSE);
      }

      // give a second to the server to answer at least one byte,
      // but as TCP work in packet, normally of size ~1400 byte, we should have the full answer
      uint32_t delay_ms = 1000;
      while(!client.available() && --delay_ms){
        delay(1);
      }

      // read the answer. And print it. 
      // TODO: check if the result is 200 (success)
      while(client.available())
      {
        char antwort = client.read();
        Serial.print(antwort);
      }

      // switch the actual state in memory. 
      // TODO: this should be done only if the server answered 200
      is_open = !is_open;
    }
  }
}
