#include <SPI.h>

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include "TimeClient.h"
#include <DNSServer.h>

int timeout = -1;

const char* def_ssid = "deeper_tl";
const char* def_password = "visonic123";
IPAddress apIP(192, 168, 0, 1);
const char *myHostname = "deeper";


const int ledPin = D2; // an LED is connected to NodeMCU pin D1 (ESP8266 GPIO5) via a 1K Ohm resistor
bool ledState = false;

ESP8266WebServer server(80);
const byte DNS_PORT = 53;
DNSServer dnsServer;


void vibro() {
    digitalWrite (ledPin, 1);
    delay(timeout);
    digitalWrite (ledPin, 0);
    delay(timeout);
}

void handleStatus() {
  char state[80];
  char html[1000];

  timeout = server.arg("led").toInt();

  if (timeout == 1) {
    strcpy(state, "State is ON");
  }
  if (timeout == -1) {
    strcpy(state, "State is OFF");
  }
  if (timeout > 1) {
    strcpy(state, "State is Vibro");
  }

  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.


  // Build an HTML page to display on the web-server root address
  snprintf ( html, 1000,

             "<html>\
  <head>\
  <meta name=\"viewport\" content=\"width=device-width\">\
  </head>\
  <body>\
    <script type=\"text/javascript\">document.write(\"<h1>\"+new Date().toLocaleString())+\"</h1>\";</script>\
    <h1>DEEPER CONTROLLER STATUS [%d] <span style=\"color:red\">[%s]</span></h1>\
  </body>\
</html>", timeout, state);

 server.sendContent(html);
  
  server.client().stop(); // Stop is needed because we sent no content length

   if (timeout == 1) {
    digitalWrite (ledPin, 1);
   } else if (timeout == -1) {
    digitalWrite (ledPin, 0);
   }
   
  delay(1000);
}


/** Handle root or redirect to captive portal */
void handleRoot() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the page.
    return;
  }
  char html[1000];
  
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.

  // Build an HTML page to display on the web-server root address
  snprintf ( html, 1000,

             "<html>\
  <head>\
    <title>DEEPER CONTROLLER</title>\
    <style>\
      body { background-color: #cccccc; font-family: Arial, Helvetica, Sans-Serif; font-size: 1.5em; Color: #000000; }\
      h1 { Color: #AA0000; }\
      button {width: 100%; height: 300px;}\
    </style>\
    <meta name=\"viewport\" content=\"width=device-width\">\
  </head>\
  <body>\
     <form target=\"myi\" action=\"/status\"><input type=\"hidden\" name=\"led\" value=\"1\"><button type=\"submit\" style=\"background-color: green; font-size: 20px;\">ON</button></form><br/>\
     <form target=\"myi\" action=\"/status\"><input type=\"hidden\" name=\"led\" value=\"-1\"><button type=\"submit\" style=\"background-color: gray; font-size: 20px;\">OFF</button></form><br/>\
     <form target=\"myi\" action=\"/status\"><input type=\"hidden\" name=\"led\" value=\"300\"><button type=\"submit\" style=\"background-color: blue; font-size: 20px;\">VIBRO</button></form><br/>\
     <iframe name=\"myi\" src=\"/status?led=-1\" style=\"width: 100%;height:200px;display:block;\"></iframe>\
  </body>\
</html>");
  
  server.sendContent(html);
  
  server.client().stop(); // Stop is needed because we sent no content length
  delay(1000);
}

/** Is this an IP? */
boolean isIp(String str) {
  for (int i = 0; i < str.length(); i++) {
    int c = str.charAt(i);
    if (c != '.' && (c < '0' || c > '9')) {
      return false;
    }
  }
  return true;
}

/** IP to String? */
String toStringIp(IPAddress ip) {
  String res = "";
  for (int i = 0; i < 3; i++) {
    res += String((ip >> (8 * i)) & 0xFF) + ".";
  }
  res += String(((ip >> 8 * 3)) & 0xFF);
  return res;
}

/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean captivePortal() {
  if (!isIp(server.hostHeader()) && server.hostHeader() != (String(myHostname)+".local")) {
    Serial.print("Request redirected to captive portal");
    server.sendHeader("Location", String("http://") + toStringIp(server.client().localIP()), true);
    server.send ( 302, "text/plain", ""); // Empty content inhibits Content-length header so we have to close the socket ourselves.
    server.client().stop(); // Stop is needed because we sent no content length
    return true;
  }
  return false;
}

void handleNotFound() {
  if (captivePortal()) { // If caprive portal redirect instead of displaying the error page.
    return;
  }
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += ( server.method() == HTTP_GET ) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";

  for ( uint8_t i = 0; i < server.args(); i++ ) {
    message += " " + server.argName ( i ) + ": " + server.arg ( i ) + "\n";
  }
  server.sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  server.sendHeader("Pragma", "no-cache");
  server.sendHeader("Expires", "-1");
  server.send ( 404, "text/plain", message );
}

void setup()   {
  // Pin for engine
  pinMode ( ledPin, OUTPUT );

  //Set up AP
  WiFi.mode(WIFI_AP_STA);
  //WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));   // subnet FF FF FF 00
  WiFi.softAP(def_ssid, def_password);

  dnsServer.setTTL(300);
  dnsServer.setErrorReplyCode(DNSReplyCode::ServerFailure);
  dnsServer.start(DNS_PORT, "*", apIP);

  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.on("/generate_204", handleRoot);  //Android captive portal. Maybe not needed. Might be handled by notFound handler.
  server.on("/fwlink", handleRoot);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
  //server.on("/submit_ap", HTTP_POST, handleSubmitAP);
  /*
  server.onNotFound([]() {
      server.sendHeader("Location", String("/"), true);
      server.send ( 302, "text/plain", "");
  });
  */
  server.onNotFound ( handleNotFound );
  server.begin();

}


void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  if (timeout > 1) {
       vibro();
    }
}



