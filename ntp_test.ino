#include <TimeLib.h>
#include <NtpClientLib.h>
#include <ESP8266WiFi.h>

#include <NeoPixelAnimator.h>
#include <NeoPixelBus.h>

#define YOUR_WIFI_SSID "LexPhone"
#define YOUR_WIFI_PASSWD "alexander"

int8_t timeZone = 1;
int8_t minutesTimeZone = 0;
bool wifiFirstConnected = false;

typedef RowMajorLayout MyPanelLayout;
const uint8_t PanelWidth = 8;  // 8 pixel x 8 pixel matrix of leds
const uint8_t PanelHeight = 8;
const uint16_t PixelCount = PanelWidth * PanelHeight;
const uint8_t PixelPin = 3;
NeoTopology<MyPanelLayout> topo(PanelWidth, PanelHeight);
NeoPixelBus<NeoGrbFeature, Neo800KbpsMethod> strip(PixelCount, PixelPin);

const uint16_t left = 0;
const uint16_t right = PanelWidth - 1;
const uint16_t top = 0;
const uint16_t bottom = PanelHeight - 1;

RgbColor red(32, 0, 0);
RgbColor green(0, 32, 0);
RgbColor blue(0, 0, 32);
RgbColor white(32);
RgbColor black(0);

void onSTAConnected(WiFiEventStationModeConnected ipInfo) {
  Serial.printf("Connected to %s\r\n", ipInfo.ssid.c_str ());
}

// Start NTP only after IP network is connected
void onSTAGotIP(WiFiEventStationModeGotIP ipInfo) {
  Serial.printf("Got IP: %s\r\n", ipInfo.ip.toString ().c_str ());
  Serial.printf("Connected: %s\r\n", WiFi.status () == WL_CONNECTED ? "yes" : "no");
  wifiFirstConnected = true;
}

// Manage network disconnection
void onSTADisconnected(WiFiEventStationModeDisconnected event_info) {
  NTP.stop();
}

void processSyncEvent(NTPSyncEvent_t ntpEvent) {
  if (ntpEvent) {
    Serial.print ("Time Sync error: ");
    if (ntpEvent == noResponse)
      Serial.println ("NTP server not reachable");
    else if (ntpEvent == invalidAddress)
      Serial.println ("Invalid NTP server address");
  } else {
    Serial.print ("Got NTP time: ");
    Serial.println (NTP.getTimeDateString (NTP.getLastNTPSync ()));
  }
}

boolean syncEventTriggered = false; // True if a time even has been triggered
NTPSyncEvent_t ntpEvent; // Last triggered event

void setup() {
  static WiFiEventHandler e1, e2, e3;

  Serial.begin (115200);
  Serial.println();
  WiFi.mode(WIFI_STA);
  WiFi.begin(YOUR_WIFI_SSID, YOUR_WIFI_PASSWD);

  NTP.onNTPSyncEvent([](NTPSyncEvent_t event) {
    ntpEvent = event;
    syncEventTriggered = true;
  });

  strip.Begin();
  strip.Show();

  e1 = WiFi.onStationModeGotIP(onSTAGotIP);// As soon WiFi is connected, start NTP Client
  e2 = WiFi.onStationModeDisconnected(onSTADisconnected);
  e3 = WiFi.onStationModeConnected(onSTAConnected);
}

void showDial(int hand, int topRow, RgbColor color) {
  for (int i = 0; i < 6; i++) {
    RgbColor c = bitRead(hand, i) == 1 ? color : black;
    strip.SetPixelColor(topo.Map(right - 1 - i, topRow), c);
    strip.SetPixelColor(topo.Map(right - 1 - i, topRow + 1), c);
  }
}

void loop () {
  static int last = 0;

  if (wifiFirstConnected) {
    wifiFirstConnected = false;
    NTP.begin("pool.ntp.org", timeZone, true, minutesTimeZone);
    NTP.setInterval (63);
  }

  if (syncEventTriggered) {
    processSyncEvent (ntpEvent);
    syncEventTriggered = false;
  }

  if ((millis() - last) > 1000) {
    last = millis();
    showDial(hour(), 1, red);
    showDial(minute(), 3, green);
    showDial(second(), 5, blue);
    strip.Show();
  }
}
