#include <WiFi.h>
#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define I2C_SDA_DEFAULT 21
#define I2C_SCL_DEFAULT 22

#define SEALEVELPRESSURE_HPA (1012.00)

//I2C devices
LiquidCrystal_PCF8574 lcd(0x27);
Adafruit_BME280 bme;

//WiFi credentials
const char* ssid     = "YOUR_WIFI_NAME";
const char* password = "YOUR_WIFI_PASSWORD";

// Set web server port number to 80
WiFiServer server(80);

// Variable to store the HTTP request
String header;

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0; 
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;

//global variables
int show = -1;
unsigned long delayTime;
float temperature;
float humidity;
int altitude;
int pressure;

void check_i2C_devices()
{
  Serial.println("Checking for LCD...");
  
  int error = Wire.endTransmission();
  Serial.print("Error: ");
  Serial.print(error);

  if (error == 0) {
    Serial.println("...LCD found.");
    show = 0;
  } else {
    Serial.println("...LCD not found.");
    while(1);
  }

  Serial.println("Checking for bme280 sensor...");

  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}

void setup()
{

  Serial.begin(115200);
  Serial.println("LCD...");

  // wait on Serial to be available on Leonardo
  while (!Serial);

  Wire.begin(I2C_SDA_DEFAULT, I2C_SCL_DEFAULT);
  Wire.beginTransmission(0x27);//bme address

  check_i2C_devices();

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  lcd.begin(16, 2); // initialize the lcd
  delayTime = 5000;

}


void listen_for_client(){
  WiFiClient client = server.available();   // Listen for incoming clients

  if (client) {                             // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime) {  // loop while the client's connected
      currentTime = millis();
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the table 
            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial;}");
            client.println("table { border-collapse: collapse; width:35%; margin-left:auto; margin-right:auto; }");
            client.println("th { padding: 12px; background-color: #0043af; color: white; }");
            client.println("tr { border: 1px solid #ddd; padding: 12px; }");
            client.println("tr:hover { background-color: #bcbcbc; }");
            client.println("td { border: none; padding: 12px; }");
            client.println(".sensor { color:white; font-weight: bold; background-color: #bcbcbc; padding: 1px; }");
            
            // Web Page Heading
            client.println("</style></head><body><h1>ESP32 with BME280 sensor and 1602A lcd</h1>");
            client.println("<table><tr><th>Measurement</th><th>Value</th></tr>");
            client.println("<tr><td>Temperature</td><td><span class=\"sensor\">");
            client.println(temperature);
            client.println(" C</span></td></tr>");       
            client.println("<tr><td>Pressure</td><td><span class=\"sensor\">");
            client.println(pressure);
            client.println(" hPa</span></td></tr>");
            client.println("<tr><td>Altitude</td><td><span class=\"sensor\">");
            client.println(altitude);
            client.println(" m</span></td></tr>"); 
            client.println("<tr><td>Humidity</td><td><span class=\"sensor\">");
            client.println(humidity);
            client.println(" %</span></td></tr>");
            client.println(" <h3>For real time updates refresh the page or check the lcd.</h3>"); 
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void update_lcd()
{
  if (show == 0) {
    lcd.setBacklight(25);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(temperature);
    lcd.print(" C  ");
    lcd.print(humidity);
    lcd.print(" %");
    lcd.setCursor(0, 1);
    lcd.print(altitude);
    lcd.print(" m    ");
    lcd.print(pressure);
    lcd.print(" hPa");
    delay(2500);
    lcd.setBacklight(0);
  }
  else{
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Error");
  }
}


void loop()
{
  temperature = bme.readTemperature();
  humidity = bme.readHumidity();
  altitude = bme.readAltitude(SEALEVELPRESSURE_HPA);
  pressure = bme.readPressure() / 100.0F;

  listen_for_client();
  
  update_lcd();

  delay(delayTime);
} 
