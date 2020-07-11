#include "WiFiHelper.h"

void WiFiHelperClass::read() {
  preferences.begin("esp", false);  // create folder

  ssid = preferences.getString("ssid", DEFAULT_SSID);
  password = preferences.getString("password", DEFAULT_PSK);
  host = preferences.getString("host", DEFAULT_HOST);

  mode = preferences.getString("mode", "dhcp");
  ipv4 = preferences.getString("ipv4");
  dns = preferences.getString("dns");
  subnet = preferences.getString("subnet");
  gateway = preferences.getString("gateway");

  preferences.end();
}

void WiFiHelperClass::write() {
  preferences.begin("esp", false);  // create folder

  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("host", host);

  preferences.putString("mode", mode);
  preferences.putString("ipv4", ipv4);
  preferences.putString("dns", dns);
  preferences.putString("subnet", subnet);
  preferences.putString("gateway", gateway);

  preferences.end();
}

void WiFiHelperClass::server() {
  Serial.println("Setup WiFi helper");

  if (WiFiHelper.connect(true)) {
    WiFiHelper.setupMDNS();
  } else {
    WiFiHelper.setupAP();
  }

  WebServerHelper.server.on("/api/wifi", HTTP_GET, [this](AsyncWebServerRequest *request) {
    int args = request->args();

    if (args > 0) {
      request->send(200, "text/plain", "message received");

      Serial.println("Update wifi settings");
      if (request->hasArg("host")) host = request->arg("host");
      if (request->hasArg("ssid")) ssid = request->arg("ssid");
      if (request->hasArg("password")) password = request->arg("password");

      write();
      WiFi.disconnect();
      ESP.restart();

    } else {
      AsyncJsonResponse *response = new AsyncJsonResponse();
      response->addHeader("Server", "ESP Async Web Server");

      JsonVariant &root = response->getRoot();
      root["host"] = host;
      root["ssid"] = ssid;
      root["password"] = password;

      root["rssi"] = String(WiFi.RSSI());
      root["time"] = String(wifiTime);

      response->setLength();
      request->send(response);
    }
  });

  WebServerHelper.server.on("/api/network", HTTP_GET, [this](AsyncWebServerRequest *request) {
    int args = request->args();

    if (args > 0) {
      request->send(200, "text/plain", "message received");

      Serial.println("Update network settings");
      if (request->hasArg("mode")) mode = request->arg("mode");
      if (request->hasArg("ipv4")) ipv4 = request->arg("ipv4");
      if (request->hasArg("dns")) dns = request->arg("dns");
      if (request->hasArg("subnet")) subnet = request->arg("subnet");
      if (request->hasArg("gateway")) gateway = request->arg("gateway");

      write();
      WiFi.disconnect();
      ESP.restart();
    } else {
      AsyncJsonResponse *response = new AsyncJsonResponse();
      response->addHeader("Server", "ESP Async Web Server");

      JsonVariant &root = response->getRoot();
      root["mode"] = mode;
      root["ipv4"] = WiFi.localIP().toString();
      root["dns"] = WiFi.dnsIP().toString();
      root["subnet"] = WiFi.subnetMask().toString();
      root["gateway"] = WiFi.gatewayIP().toString();

      response->setLength();
      request->send(response);
    }
  });
}

bool WiFiHelperClass::connect(bool firstConnect) {
  if (WiFi.status() == WL_CONNECTED) return true;

  read();

  unsigned long wifiStartTime = millis();

  if (mode == "static") {
    IPAddress ipv4IP;
    IPAddress dnsIP;
    IPAddress subnetIP;
    IPAddress gatewayIP;

    ipv4IP.fromString(ipv4);
    dnsIP.fromString(dns);
    subnetIP.fromString(subnet);
    gatewayIP.fromString(gateway);

    Serial.println("WIFI use static IP: " + ipv4);

    WiFi.config(ipv4IP, gatewayIP, subnetIP, dnsIP);
  }

  Serial.println("WIFI connect to " + ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  // The first connect can take a while
  // use the built in function to wait (up to 10sec)
  if (firstConnect) {
    WiFi.waitForConnectResult();
  } else {
    int retry = 0;
    while (WiFi.status() != WL_CONNECTED && retry++ < 100) {
      delay(10);
    }
  }

  wifiTime = millis() - wifiStartTime;
  Serial.println("WIFI connection took " + String(wifiTime));

  return WiFi.status() == WL_CONNECTED;
}

void WiFiHelperClass::setupAP() {
  // configure WiFi AP
  Serial.println("WIFI Setup AP");

  WiFi.disconnect();
  WiFi.softAP("espbs");

  IPAddress myIP = WiFi.softAPIP();
  Serial.println("AP IP address: " + myIP.toString());
}

void WiFiHelperClass::setupMDNS() {
  // use mdns for host name resolution
  if (!MDNS.begin(host.c_str())) {
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("mDNS responder started connect to http://" + host + ".local");
}

void WiFiHelperClass::sleep() {
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);

  btStop();
  esp_wifi_stop();
}

WiFiHelperClass WiFiHelper;