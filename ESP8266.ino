
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <Bounce2.h>



#include <string.h>
#include "FS.h"
#include <DHT.h>

#define DHTPIN 14     // GPIO 2 pin of ESP8266
#define DHTTYPE DHT22   // DHT 22  (AM2302)
DHT dht(DHTPIN, DHTTYPE, 30);
float h;
float t;
long sec;
int ss;
long rssi;

bool    spiffsActive = false;

MDNSResponder mdns;

// Wi-Fi
const char* ssid = "Trudova52_1";
const char* password = "21644399";

byte arduino_mac[] = { 0xDE, 0xED, 0xBA, 0xFE, 0xFE, 0xED };
IPAddress ip(192,168,0,192);
IPAddress gateway(192,168,0,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);

File fsUploadFile; 

String getContentType(String filename); // convert the file extension to the MIME type
bool handleFileRead(String path);       // send the right file to the client (if it exists)
void handleFileUpload();                // upload a new file to the SPIFFS

int relay_pin = 12;
int green_pin = 13;
int buttonPin = 0;
Bounce bouncer = Bounce(buttonPin, 5);

void setup(void){
  // preparing GPIOs
  pinMode(buttonPin,INPUT);
  
  pinMode(relay_pin, OUTPUT);
  digitalWrite(relay_pin, LOW);
  
  pinMode(green_pin, OUTPUT);
  digitalWrite(green_pin, HIGH);

  delay(100);
  
  Serial.begin(115200);
  dht.begin();
  
  WiFi.begin(ssid, password);
  WiFi.config(ip, gateway, subnet);
  // Start filing subsystem
  if (SPIFFS.begin()) {
      Serial.println("SPIFFS Active");
      Serial.println();
      spiffsActive = true;
  } else {
      Serial.println("Unable to activate SPIFFS");
  }
 
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
   delay(500);
   Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  
  if (mdns.begin("esp8266", WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }
  
  server.on("/", root);  
  server.on("/set", setValue);  
  server.on("/get", getValue);
  server.on("/fl", getFileList);
  server.on("/reboot", reboot);
  server.on("/upload", HTTP_GET, getupload);
  server.on("/upload", HTTP_POST,                       // if the client posts to the upload page
   sOk,                          // Send status 200 (OK) to tell the client we are ready to receive
   handleFileUpload                                    // Receive and save the file
   );

  server.on("/update", HTTP_GET, getupdate);
  server.on("/update", HTTP_POST, updateEnd, updateStart); 
  server.onNotFound(root);
      
  server.begin();
  Serial.println("HTTP server started");
}

void setValue(){
 if (server.hasArg("d") and server.hasArg("v")){ 
  digitalWrite(server.arg("d").toInt(), server.arg("v").toInt());  
 }  
 server.send(200);
}

void getValue(){
 String message = "{";  
 if (server.hasArg("v")){ 
  message += "\"value\":\"";
  if (server.arg("v") == "t") message += t;
  if (server.arg("v") == "h") message += h;
  if (server.arg("v") == "r") message += rssi; 
  message += "\"";
 }
 if (server.hasArg("d")){ 
  message += "\"pin";
  message += server.arg("d");
  message += "\":\"";
  int pin_n;
  pin_n = server.arg("d").toInt();
  message += digitalRead(pin_n);
  message += "\"";
 } 
 message += "}";
 server.send(200, "application/json", message);
}

void sOk(){
 server.send(200);
}

void updateStart(){
      HTTPUpload& upload = server.upload();
      if(upload.status == UPLOAD_FILE_START){
        Serial.setDebugOutput(true);
        WiFiUDP::stopAll();
        Serial.printf("Update: %s\n", upload.filename.c_str());
        uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        if(!Update.begin(maxSketchSpace)){//start with max available size
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_WRITE){
        if(Update.write(upload.buf, upload.currentSize) != upload.currentSize){
          Update.printError(Serial);
        }
      } else if(upload.status == UPLOAD_FILE_END){
        if(Update.end(true)){ //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
        Serial.setDebugOutput(false);
      }
 yield();  
}

void updateEnd(){
 server.sendHeader("Connection", "close");
 server.send(200, "text/plain", (Update.hasError())?"FAIL":"OK");
 ESP.restart();
}

void root(){
 String muri = server.uri();
 if (muri=="/") muri="/index.html";
 if (muri.indexOf(".")==-1) muri+=".html";
 if (!handleFileRead(muri))
  if (!handleFileRead("/index.html"))                // send it if it exists
    server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error 
}

void getupload(){
 if (!handleFileRead("/upload.html"))                // send it if it exists
  server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error 
}
void getupdate(){
 if (!handleFileRead("/update.html"))                // send it if it exists
  server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error 
}
void upload(){
    server.send(200);
    handleFileUpload();   
}

void reboot(){
  server.send ( 200, "text/html",  getFileText("/reboot.html"));
  delay(1000);
  ESP.reset();  
}

void temper(){
  float lh = dht.readHumidity();
  float lt = dht.readTemperature();
  if (!isnan(lh)) h = lh;
  if (!isnan(lt)) t = lt;
}
 
void loop(void){
  bouncer.update();
  if(bouncer.read() == LOW) {
    digitalWrite(green_pin, !digitalRead(green_pin));
  }
  sec=millis()/1000;
  ss=sec%60; // second
  if(ss==0 || ss==30 ) {
      temper(); 
      rssi = WiFi.RSSI();
  }
  server.handleClient();
  
} 


String getFileText(String fname){
 File f = SPIFFS.open(fname, "r");
 String s = "";
 String text = ""; 
 if (f) {
  while (f.position()<f.size()) {
   s=f.readStringUntil('\n');
   //s.trim();
   text+=s;
  } 
 }
 f.close();
 return(text);  
}

void getFileList(){
  String web; 
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    web +="<BR>";
    web +=dir.fileName();
  }
  server.send(200, "text/html", web);
}
  
String getContentType(String filename) { // convert the file extension to the MIME type
  if (filename.endsWith(".html")) return "text/html";
  else if (filename.endsWith(".css")) return "text/css";
  else if (filename.endsWith(".js")) return "application/javascript";
  else if (filename.endsWith(".ico")) return "image/x-icon";
  else if (filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path) { // send the right file to the client (if it exists)
  Serial.println("handleFileRead: " + path);
  if (path.endsWith("/")) path += "index.html";          // If a folder is requested, send the index file
  String contentType = getContentType(path);             // Get the MIME type
  String pathWithGz = path + ".gz";
  if (SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)) { // If the file exists, either as a compressed archive, or normal
    if (SPIFFS.exists(pathWithGz))                         // If there's a compressed version available
      path += ".gz";                                         // Use the compressed verion
    File file = SPIFFS.open(path, "r");                    // Open the file
    size_t sent = server.streamFile(file, contentType);    // Send it to the client
    file.close();                                          // Close the file again
    Serial.println(String("\tSent file: ") + path);
    return true;
  }
  Serial.println(String("\tFile Not Found: ") + path);   // If the file doesn't exist, return false
  return false;
}

void handleFileUpload(){ // upload a new file to the SPIFFS
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    if(!filename.startsWith("/")) filename = "/"+filename;
    Serial.print("handleFileUpload Name: "); Serial.println(filename);
    fsUploadFile = SPIFFS.open(filename, "w");            // Open the file for writing in SPIFFS (create if it doesn't exist)
    filename = String();
  } else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
      fsUploadFile.write(upload.buf, upload.currentSize); // Write the received bytes to the file
  } else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile) {                                    // If the file was successfully created
      fsUploadFile.close();                               // Close the file again
      Serial.print("handleFileUpload Size: "); Serial.println(upload.totalSize);
      server.sendHeader("Location","/success.html");      // Redirect the client to the success page
      server.send(303);
    } else {
      server.send(500, "text/plain", "500: couldn't create file");
    }
  }
}
