#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <uButton.h>

// Your WiFi credentials.
// Set password to "" for open networks.
const char ssid[] = "";
const char pass[] = "";

const int RLY_PIN = 0; // pin connected to relay's pin
const int BTN_PIN = 2; // pin connected to button's pin
const int PIR_PIN = 3; // pin connected to pir's pin

int on_time = 0;

WiFiClient wifi;
PubSubClient mqtt(wifi);
uButton rly(RLY_PIN, OUTPUT, false);
uButton btn(BTN_PIN);
uButton pir(PIR_PIN, INPUT, false);

void callback(char* t, byte* p, unsigned int l) {
  String topic(t);
  String param;
  for (int i = 0; i < l; i++) {
    param.concat((char)p[i]);
  }
  
  if (topic == "urelay/switch1/setvalue") {
    rly.setState(param == "ON" ? rly.getOnValue() : rly.getOffValue());
  } else if (topic == "urelay/switch2/setvalue") {
    // btn.setState(param == "ON" ? btn.getOnValue() : btn.getOffValue());
  } else if (topic == "urelay/switch3/setvalue") {
    // btn.setState(param == "ON" ? btn.getOnValue() : btn.getOffValue());
  }
}

void setup() {
  Serial.begin(115200, SERIAL_8N1, SERIAL_TX_ONLY, 1);

  pinMode(PIR_PIN, FUNCTION_3);

  rly.begin();
  btn.begin();
  pir.begin();

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("wifi connected");
  Serial.println("ip address: ");
  Serial.println(WiFi.localIP());
  
  mqtt.setServer("192.168.0.111", 1883);
  mqtt.setCallback(callback);

  btn.setDebounceTime(50);
  pir.setDebounceTime(50);

  on_time = millis();
}

void loop() {
  if (!mqtt.connected()) {
    // Loop until we're reconnected
    while (!mqtt.connected()) {
      // Attempt to connect
      if (mqtt.connect("urelay", "mosquitto", "password")) {
        Serial.println("mqtt connected");
        // Once connected, publish an announcement...
        mqtt.publish("urelay/system/getonline", "online");
        mqtt.publish("urelay/switch1/getvalue", rly.isOn() ? "ON" : "OFF");
        mqtt.publish("urelay/switch2/getvalue", btn.isOn() ? "ON" : "OFF");
        mqtt.publish("urelay/switch3/getvalue", pir.isOn() ? "ON" : "OFF");
        // ... and resubscribe
        mqtt.subscribe("urelay/switch1/setvalue");
        mqtt.subscribe("urelay/switch2/setvalue");
        mqtt.subscribe("urelay/switch3/setvalue");
      } else {
        Serial.print("mqtt connection failed, rc=");
        Serial.print(mqtt.state());
        Serial.println(" try again in 2 seconds");
        // Wait 2 seconds before retrying
        delay(2000);
      }
    }
  }

  mqtt.loop();
  rly.loop();
  pir.loop();
  btn.loop();

  if (pir.isChanged()) {
    if (pir.isOn()) {
      rly.setOn();
    }

    mqtt.publish("urelay/switch3/getvalue", pir.isOn() ? "ON" : "OFF");
    Serial.println(pir.isOn() ? "pir is ON" : "pir is OFF");
  }

  if (btn.isChanged()) {
    rly.setState(rly.isOn() ? rly.getOffValue() : rly.getOnValue());

    mqtt.publish("urelay/switch2/getvalue", btn.isOn() ? "ON" : "OFF");
    Serial.println(btn.isOn() ? "btn is ON" : "btn is OFF");
  }

  if (rly.isChanged()) {
    mqtt.publish("urelay/switch1/getvalue", rly.isOn() ? "ON" : "OFF");
    Serial.println(rly.isOn() ? "rly is ON" : "rly is OFF");
  }

  // Every minute
  if (millis() - on_time >= 60000) {
    on_time = millis();

    mqtt.publish("urelay/system/getonline", "online");
    mqtt.publish("urelay/switch1/getvalue", rly.isOn() ? "ON" : "OFF");
    mqtt.publish("urelay/switch2/getvalue", btn.isOn() ? "ON" : "OFF");
    mqtt.publish("urelay/switch3/getvalue", pir.isOn() ? "ON" : "OFF");

    Serial.println("status updated");
  }
}
