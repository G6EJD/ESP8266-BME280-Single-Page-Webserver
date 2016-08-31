// ESP8266 WebServer - using DHT11 sensor
// (c) D L Bird 2016
//
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

const char *ssid     = "********"; // Your SSID here
const char *password = "********"; // Your password here

IPAddress ip(192, 168, 0, 53);   // The address 192.168.0.53 is arbitary, if could be any address in the range of your router, but not another device!
IPAddress gateway(192,168,0,1);  // My router has this base address
IPAddress subnet(255,255,255,0); // Define the sub-network

float bme_pressure, bme_temp, bme_humidity;
int count = 0;

WiFiServer server(80);

Adafruit_BME280 bme; // Note Adafruit assumes I2C adress = 0x77 my module (eBay) uses 0x76 so the library address has been changed.

void setup() {
  Serial.begin(115200);
  Serial.print("Connecting to ");
  Serial.println(ssid);          // Connect to WiFi network
  WiFi.config(ip, gateway, subnet);
  WiFi.persistent(false);  // disables the storage of credentials to flash.
  WiFi.begin(ssid, password);   
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected..");
  // Start the webserver
  server.begin();
  Serial.println("Webserver started...");
  pinMode(D3, INPUT_PULLUP); //Set input (SDA) pull-up resistor on

  Wire.setClock(2000000);    // Set I2C bus speed 
  Wire.begin(D3,D4); // Define which ESP8266 pins to use for SDA, SCL of the Sensor
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}
 
void loop() {
  // Check if a client has connected
  WiFiClient client = server.available();
  if (client) { // an http request has been made
    boolean currentLineIsBlank = true;
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();  // if at the end of a line because newline character received and the line is blank, the http request is complete, so the client can receive a response
        if (c == '\n' && currentLineIsBlank) {
          bme_temp     = bme.readTemperature();        // No correction factor needed for this sensor
          delay(1000);
          bme_humidity = bme.readHumidity() + 1.0;     // Plus a correction factor for this sensor
          delay(1000);
          bme_pressure = bme.readPressure()/100 + 3.7; // Plus a correction factor for this sensor
          Serial.println(bme_temp);
          Serial.println(bme_humidity);
          Serial.println(bme_pressure);
          float T = (bme_temp * 9 / 5) + 32;           // Convert back to deg-F for the RH equation
          float RHx = bme_humidity;                    // Short form of RH for inclusion in the equation makes it easier to read
          float heat_index = (-42.379+(2.04901523*T)+(10.14333127*RHx)-(0.22475541*T*RHx)-(0.00683783*sq(T))-(0.05481717*sq(RHx))+(0.00122874*sq(T)*RHx)+(0.00085282*T*sq(RHx))-(0.00000199*sq(T)*sq(RHx))-32)*5/9;
          if ((bme_temp <= 26.66) || (bme_humidity <= 40)) heat_index = bme_temp; // The convention is not to report heat Index when temperature is < 26.6 Deg-C or humidity < 40%
          float dew_point = 243.04*(log(bme_humidity/100)+((17.625*bme_temp)/(243.04+bme_temp)))/(17.625-log(bme_humidity/100)-((17.625*bme_temp)/(243.04+bme_temp)));

          // Now send a correctly formatted HTML response together with the sensor data, statements indented to help with HTML formatting
          client.println("<!DOCTYPE HTML>");
          client.println("<html>");
            client.println("<head>");
               client.println("<style>");
                 client.println("h2 { color: blue; font-family: verdana; font-size: 200%; text-align: left; display:inline; } ");
                 client.println("table { font-family: arial, sans-serif; font-size: 150%; border-collapse: collapse; border: 3px blue; width: 60%; }");
                 client.println("td, th {  border: 1px solid blue; text-align: center; padding: 8px; } ");
                 client.println("tr:nth-child(even) { background-color: skyblue; }");
              client.println("</style>");
            client.println("<title>ESP8266 Readings</title>");
            client.println("<meta http-equiv=\"refresh\" content=\"15\">"); // Refresh the screen every 15-seconds
            client.println("</head>");
            client.println("<body>");
              client.println("<h2>BOSCH BME280 Sensor</h2>");
              client.println("<table>");
                client.println("<tr>");
                  client.println("<th>Temperature</th>");
                  client.println("<th>Humidity</th>");
                  client.println("<th>Pressure</th>");
                  client.println("<th>Dew Point</th>");
                  client.println("<th>Heat Index</th>");
                client.println("</tr>");
                client.println("<tr>");
                  client.println("<td> "+String(bme_temp,1)     + "&deg;C</td>"); // Mixing HTML with sensor values for display
                  client.println("<td> "+String(bme_humidity,1) + "% RH</td>");
                  client.println("<td> "+String(bme_pressure,1) + " hPa</td>");
                  client.println("<td> "+String(dew_point,1)    + "&deg;C</td>");
                  client.println("<td> "+String(heat_index,1)   + "&deg;C</td>");
                client.println("</tr>");
              client.println("</table>");
              client.println("<h6>&copy; D Bird 2016 ("+String(count)+")</h6>"); // And display how many times the screen has been refreshed
            client.println("</body>");
          client.println("</html>");
          break;
        }
        if (c == '\n') {
          currentLineIsBlank = true;  // It's a new line
        } else if (c != '\r') {
          currentLineIsBlank = false; // There's a character on the current line
        }
      }
    }
    delay(10);
    client.flush();     // Flush the buffers
    client.stop();      // Close the Client connection
    count = count + 1;  // Increase refresh indicator count
    delay(5000);        // Control speed of BME280 sensor reading
  }
}


