//COMUNICACION
#include <SPI.h>
#include <ESP8266WiFi.h>
#include <SimpleTimer.h>
#include <ArduinoJson.h>
#include "iconos.h"

//DISPLAY
//#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
#include <SH1106.h>
//SSD1306Wire display(0x3C, 4, 5); // Initialize the OLED display using Wire library
SH1106 display(0x3C, 4, 5);

byte ledPin = 2;
byte ledtest1 = 9;
byte ledtest2 = 15;
char ssid[] = "TP-LINK_155"; // SSID of your home WiFi
char pass[] = "52299002"; // password of your home WiFi
int datos = 0;

double latitude, longitude, angulo_X, angulo_Y, corriente_bateria,
 voltage_bateria, corriente_panel, porcentaje_bateria, elevacion,
 azimuth;
int hora, minutos, dia, mes, anio;
String localidad_s, pais_s, detalle_clima_s, temp_actual_s, humedad_actual_s;

IPAddress server(192, 168, 0, 80); // the fix IP address of the server
WiFiClient client;
SimpleTimer timer;

void recibir(void);

void setup() {
    //SERIAL
    Serial.begin(115200); // only for debug

    //WIFI
    WiFi.begin(ssid, pass); // connects to the WiFi router
    WiFi.mode(WIFI_STA);
    while (WiFi.status() != WL_CONNECTED) {
        Serial.print(".");
        delay(500);
    }
    Serial.println("Connected to wifi");
    Serial.print("Status: "); Serial.println(WiFi.status()); // Network parameters
    Serial.print("IP: "); Serial.println(WiFi.localIP());
    Serial.print("Subnet: "); Serial.println(WiFi.subnetMask());
    Serial.print("Gateway: "); Serial.println(WiFi.gatewayIP());
    Serial.print("SSID: "); Serial.println(WiFi.SSID());
    Serial.print("Signal: "); Serial.println(WiFi.RSSI());

    //I/O
    pinMode(ledPin, OUTPUT);
    pinMode(ledtest1, OUTPUT);
    pinMode(ledtest2, OUTPUT);
    digitalWrite(ledPin, HIGH);
    digitalWrite(ledtest1, LOW);
    digitalWrite(ledtest2, LOW);

    //DISPLAY
    display.init(); // Initialising the UI will init the display too.
    display.flipScreenVertically();
    display.setFont(ArialMT_Plain_16);
    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.clear();
    display.drawString(0, 0, "BIENVENIDOS");
    display.drawString(0, 15, "A SEGUIDOR");
    display.drawString(0, 30, "SOLAR!");
    display.display();

    //TIMER
    datos = timer.setInterval(750, recibir);
}


void loop () {
    timer.run();
    if (client.connect(server, 80)) // Connection to the server
        digitalWrite(ledPin, LOW);
    else
        digitalWrite(ledPin, HIGH);

    if (latitude) { //Si estan vacias las variables, no muestra nada
        display.clear();
        display.drawString(0, 0, "Lat: ");
        display.drawString(60, 0, (String)latitude);
        display.drawString(0, 15, "Long: ");
        display.drawString(60, 15, (String)longitude);
        display.drawString(0, 45, (String)localidad_s);
        display.display();
        delay(2000);
        display.clear();
        display.drawString(0, 0, "FECHA:");
        display.drawString(0, 15, (String)dia + "/" + String(mes) + "/" + String(anio));
        display.drawString(0, 30, "HORA: ");
        if (minutos > 10)
            display.drawString(0, 45, (String)hora + ":" + String(minutos));
        else
            display.drawString(0, 45, (String)hora + ":" + String(minutos) + "0");
        display.display();
        delay(2000);
        display.clear();
        display.drawString(0, 0, "CLIMA: ");
        display.drawString(0, 15, (String)detalle_clima_s);
        display.drawString(0, 30, "Temp: ");
        display.drawString(60, 30, (String)temp_actual_s);
        display.drawString(0, 45, "Hum: ");
        display.drawString(60, 45, (String)humedad_actual_s);
        display.display();
        delay(2000);
        display.clear();
        display.drawString(0, 0, "Angulo motor X: ");
        display.drawString(0, 15, (String)angulo_X);
        display.drawString(0, 30, "Angulo motor Y: ");
        display.drawString(0, 45, (String)angulo_Y);
        display.display();
        delay(2000);
        display.clear();
        display.drawString(0, 0, "Elevacion: ");
        display.drawString(0, 15, (String)elevacion);
        display.drawString(0, 30, "Azimuth: ");
        display.drawString(0, 45, (String)azimuth);
        display.display();
        delay(2000);
        display.clear();
        display.drawString(0, 0, "Tension Bat: ");
        display.drawString(0, 15, (String)voltage_bateria);
        display.drawString(0, 30, "Porcentaje de carga: ");
        display.drawString(0, 45, (String)porcentaje_bateria);
        display.display();
        delay(2000);
        display.clear();
        display.drawString(0, 0, "Potencia Bat: ");
        display.drawString(0, 15, (String)(voltage_bateria * corriente_bateria));
        display.drawString(0, 30, "Corriente Panel: ");
        display.drawString(0, 45, (String)(corriente_panel));
        display.display();
        delay(2000);
    }// client will trigger the communication after two seconds
    client.flush();
    delay(250);
}


void recibir() {
    //timer.disable(datos);
    noInterrupts();
    StaticJsonBuffer<2000> jsonBuffer;
    String json = client.readStringUntil('\r'); // receives the answer from the sever
    JsonObject& root = jsonBuffer.parseObject(json);
    if (root.success())
        datos = 1;
    else
        datos = 0;
    if (datos) {
        latitude = root["latitud"];
        longitude = root["longitud"];
        hora = root["hora"];
        minutos = root["minutos"];
        dia = root["dia"];
        mes = root["mes"];
        anio = root["anio"];
        const char* localidad = root["localidad"];
        const char* pais = root["pais"];
        const char* detalle_clima = root["detalle"];
        const char* temp_actual = root["temperatura"];
        const char* humedad_actual = root["humedad"];
        localidad_s = String(localidad);
        pais_s = String(pais);
        detalle_clima_s = String(detalle_clima);
        temp_actual_s = String(temp_actual);
        humedad_actual_s = String(humedad_actual);
        angulo_X = root["ang_X"];
        angulo_Y = root["ang_Y"];
        elevacion = root["elevacion"];
        azimuth = root["azimuth"];
        corriente_bateria = root["corriente_bateria"];
        voltage_bateria = root["voltage_bateria"];
        porcentaje_bateria = root["porcentaje_bateria"];
        corriente_panel = root["corriente_panel"];
    }
    //timer.enable(datos);
    interrupts();
}