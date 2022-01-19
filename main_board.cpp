/* Librerias */
//WiFi
#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiAP.h>
#include <ESP8266WiFiGeneric.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WiFiScan.h>
#include <ESP8266WiFiSTA.h>
#include <ESP8266WiFiType.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <WiFiServer.h>
#include <WifiLocation.h> //Nueva
#include <WiFiUdp.h>
#include <TimeLib.h>

//OTA
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>

//JSON
#include <ArduinoJson.h>

//Strings
#include <String.h>
//Habilitar timer
//#include <SimpleTimer.h>
//#include <Ticker.h> //Ticker Library
//Display

#include <LiquidCrystal_I2C.h>
/**/

/*Definiciones*/
#define IN1 12
#define IN2 13
#define IN3 14
#define IN4 16
#define EN1 4
#define EN2 5
#define LED_ROJO 10
#define LED_VERDE 15
#define MUX0 0
#define MUX1 2
#define MUX2 LED_ROJO

#define MAX_SSID_LEN 32

#define tiempo_db 1800000 // Envia datos a la DB cada 30 minutos
#define tiempo_clima 1200000 // Baja el clima cada 20 minutos
#define tiempo_display_clima 5000 //2500 ms = 2.5 segundos
#define tiempo_medir_bateria 60000 //60 segundos

#define rango_X 180.00 // 180 grados
#define rango_Y 60.00 // 180 grados
#define rango_calibracion_X 0.5 // angulo central (90º) +/- 0.4 grados
#define rango_calibracion_Y 0.5 // angulo central (0º) +/- 0.4 grados
#define ang_min_X 45
#define ang_max_X 180-ang_min_X //130
#define ang_min_Y -45
#define ang_max_Y -ang_min_Y
#define vel_min_ejeX 180 // vel_min_ejeX/1023 * 100% = 18%
//#define vel_min_ejeY 612 // vel_min_ejeY/1023 * 100% = 60%
#define vel_min_ejeY 450 // vel_min_ejeY/1023 * 100% = 45%

#define hora_inicial 6
#define cant_lecturas_ADC 1000
#define referencia 1.000 //pote 48.8k punto medio eje X
#define factor_cv_bateria 503/33 // Divisor resistivo 470k + 33k
#define sens_AC712 0.100

ADC_MODE(0);
/**/


/*Constantes*/
//Parametros WiFi
char ssid[20] = "default";
char password[20] = "123456";
const char* ssid1 = "TP-LINK_155";
const char* password1 = "52299002";
const char* host = "192.168.0.101"; //en Windows, abrir cmd y poner "ipconfig", la direccion IPv4 e
char ssid2[MAX_SSID_LEN] = "";

//URLs y APIs
const char* host_clima = "api.openweathermap.org";
String google_apikey = "PUT_YOUR_GOOGLE_API_KEY_HERE";
const char* weather_apikey = "e3912727ce72ac2bd3ce936ab7bf9b2e";
String line;
WifiLocation location(google_apikey);

//Constantes y variables globales
unsigned int Serie = 005;
unsigned char conexion = 1, state_display = 0, bandera = 0, conexion_server = 0;
unsigned char led_v = 0, conectar = 0, conectado = 0;
unsigned char x_tabla = 0;
int vel_x = 0, vel_y = 0;

//Geolocalizacion
double pi = 3.14159265358979323846;
float lat, lat2, lng;
double delta, h;
float elevacion[29], elevacion2[29], azimuth[29];
float timezone = -3;
float northOrSouth = 180;
unsigned char dia, mes, hora, minutos, demo;
int anio;
double elev, az;

//Strings Clima
String temperatura, humedad, localidad, pais, detalle_clima;
const int port_clima = 443;
const int port_server = 80;
const char* url = "/data/2.5/weather?lat=";
double temp = 0;
/*ADC*/
double voltage_bateria, porcentaje_bateria, promedio_bateria;
double corriente_bateria, corriente_panel, corriente_sensor1;
double voltage_pote, promedio_pote, angulo_X, angulo_Y;
unsigned char canal = 1, canal_temp = 1;

//canal del ADC
//= 000 Corriente bateria
//= 001 Bateria
//= 010 Potenciometro de motor en eje Y
//= 011 Potenciometro de motor en eje X
//= 100 Corriente panel

size_t EPROM_MEMORY_SIZE = 512;

//Interrupciones
//SimpleTimer timer;
//Ticker blinker;

//Server-client
WiFiServer server(port_server);
//Funciones
void setup_IO(void);
void geolocalizacion(void);
void posicion_del_sol(void);
void scanAndSort(void);
void conectar_WIFI (void);
void clima(void);
String httpsPost(String url, String contentType, String data, int &errorCode);
String getValue(String data, char separator, int index);
void formateo_hora (int epoch);
void enviar_datos(void);
void lectura_ADC(void);
//void medir_bateria(void);
void Encender_LED_Verde(void);
void modo_demo(void);
void modo_debug(void); //PROBAR ENTRADAS Y SALIDAS

struct st_t {
 float lat_eeprom;
 float lng_eeprom;
};


void setup() {
	setup_IO();
	while (!conectado)
		scanAndSort();

	//Hace 10 lecturas seguidas (cada 100ms) de la bateria en 1 segundo
	demo = 0;
	//modo_debug();
	modo_demo();
	delay(100);
	geolocalizacion();
	delay(100);
	clima();
	delay(100);
	ver_hora();
	delay(100);
	posicion_del_sol();
	delay(100);
	// medir_bateria();
	// delay(100);
	}


void loop() {
 // timer.run();
 ESP.wdtFeed();
}


void setup_IO() {
 //Salidas
 pinMode (IN1, OUTPUT);
 pinMode (IN2, OUTPUT);
 pinMode (IN3, OUTPUT);
 pinMode (IN4, OUTPUT);

 digitalWrite(IN1, LOW); //Activa por alto
 digitalWrite(IN2, LOW); //Activa por alto
 digitalWrite(IN3, LOW); //Activa por alto
 digitalWrite(IN4, LOW); //Activa por alto

 pinMode (MUX0, OUTPUT);
 pinMode (MUX1, OUTPUT);
 pinMode (MUX2, OUTPUT);

 digitalWrite(MUX0, LOW); //Activa por alto
 digitalWrite(MUX1, LOW); //Activa por alto
 digitalWrite(MUX2, HIGH); //Activa por bajo

 pinMode (EN1, OUTPUT);
 pinMode (EN2, OUTPUT);

 analogWrite(EN1, 0); //Activa por alto
 analogWrite(EN2, 0); //Activa por alto

 pinMode (LED_ROJO, OUTPUT);
 pinMode (LED_VERDE, OUTPUT);

 digitalWrite(LED_ROJO, LOW); //Activa por alto
 digitalWrite(LED_VERDE, LOW); //Activa por alto
 //Entradas
 Serial.begin(115200);
 //Parámetros análogicos Motor
 analogWriteFreq(200); //200Hz
 analogWriteRange(1023); //0 a 1024 es la intensidad
}


// ============================ FUNCIONES ACTIVADAS POR INTERRUPCIONES DEL TIMER ==================
void Encender_LED_Verde() {
 if (led_v)
 	digitalWrite(LED_VERDE, !(digitalRead(LED_VERDE)));
 else
 	digitalWrite(LED_VERDE, LOW);
}


// ============================ GPS ============================
// geolocalizacion = Geolocalizacion a traves de las redes WiFi disponibles y la API de Google
// Solo se realiza al iniciar la placa.
// posicion_del_sol = Elevacion y Azimuth del sol
void geolocalizacion() {
	int n = 0;
	struct st_t st;

	Serial.println("");
	Serial.println("Lectura Flash...");

	EEPROM.begin(EPROM_MEMORY_SIZE);
	delay(300);
	EEPROM.get(0, st.lat_eeprom);
	delay(300);
	EEPROM.get(0 + sizeof(st.lat_eeprom), st.lng_eeprom);

	if (!st.lat_eeprom || isnan(st.lat_eeprom)) {
		 Serial.println("Memoria vacia.");
		 delay(750);
		 Serial.println("Geolocalizacion en curso...");
		 memset(ssid2, 0, MAX_SSID_LEN);
		 int errorCode;
		 int n = WiFi.scanNetworks();

		if (n == 0) {
			Serial.println("No se encontraron redes WiFi...");
		 }
		else {
			location_t loc = location.getGeoFromWiFi();
			Serial.println("Location request data");
			Serial.println(location.getSurroundingWiFiJson());
			Serial.println("Latitude: " + String(loc.lat, 7));
			Serial.println("Longitude: " + String(loc.lon, 7));
			Serial.println("Accuracy: " + String(loc.accuracy));
			float latitud = loc.lat;
			float longitud = loc.lon;
			float acc = loc.accuracy;
			Serial.println(latitud);
			Serial.println(longitud);
			lat = latitud;
			lng = longitud;
			st.lat_eeprom = latitud;
			st.lng_eeprom = longitud;
			EEPROM.begin(EPROM_MEMORY_SIZE);
			EEPROM.put(0, st.lat_eeprom);
			delay(100);
			EEPROM.put(0 + sizeof(st.lat_eeprom), st.lng_eeprom);
			delay(100);
			EEPROM.commit();
			Serial.println("Grabando Flash...");
	 	}
	 }
		 else {
			delay(200);
			Serial.println("Datos cargados!");
			lat = st.lat_eeprom;
			lng = st.lng_eeprom;
			}

	Serial.print("Lat: ");
	Serial.print(lat, 6);
	Serial.print(" Lng: ");
	Serial.println(lng, 6);
	Serial.println("");
}


void posicion_del_sol() {
	float day1, day2;
	day1 = day2 = dia;
	lat2 = lat * pi / 180;

	float hour2 = hora_inicial;
	float minute2 = 00;

	for (int x = 0; x < 29; x++) {
	//START OF THE CODE THAT CALCULATES THE POSITION OF THE SUN
	double n = daynum(mes) + day2;//NUMBER OF DAYS SINCE THE START OF THE YEAR.
	delta = .40927971 * sin(2 * pi * ((284 + n) / 365.25)); //SUN'S DECLINATION.
	day1 = dayToArrayNum(day2);//TAKES THE CURRENT DAY OF THE MONTH AND CHANGES IT TO A LOOK UP VALU
	h = (FindH(day1, mes)) + lng + (timezone * -1 * 15); //FINDS THE NOON HOUR ANGLE ON THE TABLE AN
	h = ((((hour2 + minute2 / 60) - 12) * 15) + h) * pi / 180; //FURTHER MODIFIES THE NOON HOUR ANG
	elevacion[x] = (asin(sin(lat2) * sin(delta) + cos(lat2) * cos(delta) * cos(h))) * 180 / pi; //FI
	azimuth[x] = ((atan2((sin(h)), ((cos(h) * sin(lat2)) - tan(delta) * cos(lat2)))) + (northOrSouth));
	//END OF THE CODE THAT CALCULATES THE POSITION OF THE SUN
	////////////////////
	if (minute2 == 0)
		minute2 = 30;
	else if (minute2 == 30) {
		minute2 = 0;
		hour2++;
	}
	if (x < 15) { //Correcion azimuth (la formula lo mide con respecto al norte
	//pero el panel esta mirando al oeste
	if (azimuth[x] > 90) {
		azimuth[x] = azimuth[x] - 90;
		azimuth[x] *= (-1);
	}
	else
		azimuth[x] = 90 - azimuth[x];
	}
	if (x >= 15) {
		azimuth[x] -= 270;
		azimuth[x] *= (-1);
	}
	if (azimuth[x] > ang_max_Y)
		azimuth[x] = ang_max_Y;
	else if (azimuth[x] < ang_min_Y)
		azimuth[x] = ang_min_Y;

	//////////////////
	for (int x = 0; x < 15; x++) {
		elevacion2[x] = elevacion[x];
	if (elevacion2[x] > ang_max_X)
		elevacion2[x] = ang_max_X;
	if (elevacion2[x] < ang_min_X)
		elevacion2[x] = ang_min_X;
	}

	float giro;
	for (int x = 15; x < 29; x++) {
	 	elevacion2[x] = (elevacion[14] * 2) - elevacion[x];
	 if (x == 15) {
		 elevacion2[x] = 180 - elevacion2[x] * 0.675; //107.63
		 giro = elevacion2[x];
	 }
	 if (x > 15)
	 	elevacion2[x] = (2 * giro) - 180 + (elevacion2[x] * 0.625); //128.725
	 if (elevacion2[x] > ang_max_X)
	 	elevacion2[x] = ang_max_X;
	 if (elevacion2[x] < ang_min_X)
	 	elevacion2[x] = ang_min_X;
		}
	/////////////////
	}

	hour2 = hora_inicial;
	minute2 = 0;

	Serial.print("DIA: ");
	Serial.print(dia);
	Serial.print("/");
	Serial.println(mes);

	Serial.println("Hora: Elevación: Azimuth:");
	for (int x = 0; x < 29; x++) {
	 if (hour2 < 10)
	 	Serial.print("0");

	 Serial.print(hour2, 0);
	 Serial.print(":");
	 if (minute2 == 0)
	 	Serial.print("00");
	 else
	 	Serial.print(minute2, 0);

	 Serial.print(" ");

	 if (elevacion2[x] > 0)
	 	Serial.print(elevacion2[x]);
	 else
	 	Serial.print("0.00");

	 Serial.print(" ");
	 Serial.println(azimuth[x]);

	 if (minute2 == 0)
	 	minute2 = 30;
	 else if (minute2 == 30) {
	 	minute2 = 00;
	 	hour2++;
		}
	}
	Serial.print("");
}



// ============================ WIFI ============================
// scanAndSort = Escanea las redes WiFi disponibles y las ordena segun la intensidad
// OTA_update = Actualizacion online
// Conectar_WIFI = conectar a la red elegida en "scanAndSort"
void scanAndSort() {
	Serial.println("");
	memset(ssid2, 0, MAX_SSID_LEN);
	int n = WiFi.scanNetworks();
	Serial.println("Detectando redes WiFi...");

	if (n == 0) {
		Serial.println("No hay redes disponibles.");
		delay(300);
	}
	else {
		Serial.print((String)n);
		Serial.println(" redes disponibles");
		int indices[n], i;
		for (i = 0; i < n; i++) {
			indices[i] = i;
		}
		for (i = 0; i < n; i++) {
			for (int j = i + 1; j < n; j++) {
				if (WiFi.RSSI(indices[j]) > WiFi.RSSI(indices[i])) {
				std::swap(indices[i], indices[j]);
				}
			}
		}
		i = 0;

		if (WiFi.encryptionType(indices[i]) == ENC_TYPE_NONE) {
			delay(150);
			memset(ssid2, 0, MAX_SSID_LEN);
			strncpy(ssid2, WiFi.SSID(indices[i]).c_str(), MAX_SSID_LEN);
		}
		String temp_ssid = WiFi.SSID(indices[i]);
		char *cstr = new char[temp_ssid.length() + 1];
		strcpy(ssid, temp_ssid.c_str());
		delay(100);
		if (strcmp(ssid1, ssid) == 0) {
			strcpy(password, password1);
		}
		conectar_WIFI();
	}
}


void conectar_WIFI() {
	int tiempo_conexion = 10;
	Serial.println("");
	WiFi.begin(ssid, password);
	//WiFi.mode(WIFI_STA);
	Serial.println("Conectando a");
	Serial.println((String)ssid);
	while (WiFi.status() != WL_CONNECTED) {
		delay(500);
		tiempo_conexion--;
		if (!tiempo_conexion) {
			Serial.println("Imposible conectar!");
			conectado = 0;
			conectar = 0;
			delay(500);
			return;
		}
	}
	Serial.println("Conectado exitosamente!");
	conectado = 1;
	return;
}


// ============================ CLIMA ============================
// clima = Conexion a openweathermap
// httpsPost = Envio de trama Http al servidor
void clima() {
	delay(100);
	int conec_clima = 5;
	conexion = 1;
	temperatura = "0";
	humedad = "0";
	WiFiClientSecure client;
	while (conec_clima) {
		if (client.connect(host_clima, port_clima)) {
			Serial.println("CONECTADO A OPENWEATHERMAP");
			Serial.println("Descargando clima...");
			conec_clima = 0;
		}
		else {
			conec_clima--;
			Serial.println("NO SE PUDO CONECTAR A OPENWEATHERMAP");
			Serial.println("Reconectando...");
			delay(500);
		}
	}
	String param = String(url) +
	lat + "&lon=" + lng +
	"&APPID=" + weather_apikey + "&lang=es";
	Serial.print("requesting URL: ");
	Serial.println(url);
	client.print(String("GET ") + param + " HTTP/1.1\r\n" +
	"Host: " + host + "\r\n" +
	"User-Agent: ESP8266\r\n" +
	"Connection: close\r\n\r\n");
	Serial.println("request sent");
	line = client.readStringUntil('\n');

	if (line != "HTTP/1.1 200 OK\r") {
		Serial.print("Unexpected response: ");
		Serial.println(line);
		return;
	}
	if (client.find("\r\n\r\n")) {
		DynamicJsonBuffer jsonBuffer(4096);
		JsonObject& root = jsonBuffer.parseObject(client);
		//root.prettyPrintTo(Serial);
		// parsed output
		const char* loc = root["name"];
		Serial.print("City: "); Serial.println(loc);
		localidad = loc;
		temp = 0;
		temp = root["main"]["temp"];
		temp -= 273.15;
		char buffer[64];
		sprintf(buffer, "Temperature: %.02f", temp);
		temperatura = String(buffer);
		Serial.println(buffer);
		const int humidity = root["main"]["humidity"];
		sprintf(buffer, "Humidity: %d", humidity);
		humedad = String(humidity);
		Serial.println(buffer);
		const int pressure = root["main"]["pressure"];
		sprintf(buffer, "Pressure: %d", pressure);
		Serial.println(buffer);
		const double wind = root["wind"]["speed"];
		sprintf(buffer, "Wind speed: %.02f m/s", wind);
		Serial.println(buffer);
		const char* weather = root["weather"][0]["main"];
		const char* description = root["weather"][0]["description"];
		sprintf(buffer, "Weather: %s (%s)", weather, description);
		Serial.println(buffer);
		detalle_clima = String(description);
		const unsigned int hours = root["dt"];
		sprintf(buffer, "Hora: %d", hours);
		Serial.println(buffer);
		formateo_hora(hours);
		Serial.println("");
		Serial.print(dia);
		Serial.print("/");
		Serial.print(mes);
		Serial.print("/");
		Serial.println(anio);
		Serial.println("");
		Serial.print(hora);
		Serial.print(":");
		Serial.print(minutos);
		if (minutos > 10)
		Serial.println("");
		else
		Serial.println("0");
		Serial.println("");
	}
	conexion = 0;
	client.stop();
}


// ============================ PARSEO Y FORMATEO ============================
// getValue = Parseo de los datos del clima
// formateo_hora = Formateo fecha y hora de UNIX a formato estandar
String getValue(String data, char separator, int index) {
	int found = 0;
	int strIndex[] = {
	0, -1
	};
	int maxIndex = data.length() - 1;

	for (int i = 0; i <= maxIndex && found <= index; i++) {
		if (data.charAt(i) == separator || i == maxIndex) {
			found++;
			strIndex[0] = strIndex[1] + 1;
			strIndex[1] = (i == maxIndex) ? i + 1 : i;
		}
	}
	return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}


void formateo_hora (int epoch) {
	int hora_temp;
	dia = day(epoch);
	mes = month(epoch);
	anio = year(epoch);
	if (hour(epoch) == 0)
		hora_temp = 24;
	else if (hour(epoch) == 1)
		hora_temp = 25;
	else if (hour(epoch) == 2)
		hora_temp = 26;
	else hora_temp = hour(epoch);
		hora = (hora_temp - 3);
	minutos = minute(epoch);
}


// ============================ ADC ============================
// lectura_ADC = Lectura promediada
void lectura_ADC() {
	if (canal != canal_temp) {
		if (canal == 1) {
			digitalWrite(MUX2, LOW); //0
			digitalWrite(MUX1, LOW); //0
			digitalWrite(MUX0, LOW); //0
		}
		else if (canal == 2) {
			digitalWrite(MUX2, LOW); //0
			digitalWrite(MUX1, LOW); //0
			digitalWrite(MUX0, HIGH); //1
		}
		else if (canal == 3) {
			digitalWrite(MUX2, LOW); //0
			digitalWrite(MUX1, HIGH); //1
			digitalWrite(MUX0, HIGH); //1
		}
		else if (canal == 4) {
			digitalWrite(MUX2, LOW); //0
			digitalWrite(MUX1, HIGH); //1
			digitalWrite(MUX0, LOW); //0
		}
		else if (canal == 5) {
			digitalWrite(MUX2, HIGH); //1
			digitalWrite(MUX1, LOW); //0
			digitalWrite(MUX0, LOW); //0
		}
		canal_temp = canal;
		delay(500);
	}

	if (canal == 1) { //Corriente promedio bateria 000
		int a;
		corriente_sensor1 = 0;
		corriente_bateria = 0;
		for (a = 0; a < cant_lecturas_ADC; a++) {
			corriente_sensor1 = analogRead(A0) * 0.001 * 512 / 535;
			corriente_bateria += (corriente_sensor1 - 0.512) / sens_AC712;
		}
		corriente_bateria /= (cant_lecturas_ADC);
		porcentaje_bat();
	}

	else if (canal == 2) { //Promedio tension bateria 001
		int b;
		voltage_bateria = 0;
		for (b = cant_lecturas_ADC; b > 0; b--) {
			voltage_bateria += (analogRead(A0) * factor_cv_bateria * 0.001 * 0.952);
		}
		voltage_bateria /= (cant_lecturas_ADC);
		porcentaje_bat();
	}

	else if (canal == 3) { //Lectura instantanea (para angulo EJE X) 010
		int c;
		voltage_pote = 0;
		for (c = cant_lecturas_ADC; c > 0; c--) {
			voltage_pote += analogRead(A0) * (1.023 - (referencia - 1.023)) / 1023.0;
		}
		angulo_X = (voltage_pote / cant_lecturas_ADC) * rango_X;
	}

	else if (canal == 4) { //Lectura instantanea (para angulo EJE Y) 011
	int d;
	voltage_pote = 0;
	for (d = cant_lecturas_ADC; d > 0; d--) {
		voltage_pote += analogRead(A0) * (1.023 - (referencia - 1.023)) / 1023.0;
	}
	angulo_Y = ( ((referencia / 2) - (voltage_pote / cant_lecturas_ADC)) * 180.0);
	}

	else if (canal == 5) { //Corriente promedio panel 100
		int e;
		double corriente_sensor2 = 0;
		corriente_panel = 0;
		for (e = cant_lecturas_ADC; e > 0; e--) {
			corriente_sensor2 = analogRead(A0) * 0.001 * 512 / 535;
			corriente_panel += (corriente_sensor2 - 0.512) / sens_AC712;
		}
		corriente_panel /= (cant_lecturas_ADC);
	}
	ESP.wdtFeed();
}


void porcentaje_bat() {
 if (corriente_bateria <= 0.35) {
 	if (voltage_bateria >= 12.9)
		porcentaje_bateria = 100;
	else if (voltage_bateria >= 12.8)
		porcentaje_bateria = 95.83;
	else if (voltage_bateria >= 12.7)
		porcentaje_bateria = 91.66;
	else if (voltage_bateria >= 12.6)
		porcentaje_bateria = 87.5;
	else if (voltage_bateria >= 12.5)
		porcentaje_bateria = 83.33;
	else if (voltage_bateria >= 12.4)
		porcentaje_bateria = 79.16;
	else if (voltage_bateria >= 12.3)
		porcentaje_bateria = 75.00;
	else if (voltage_bateria >= 12.2)
		porcentaje_bateria = 70.83;
	else if (voltage_bateria >= 12.1)
		porcentaje_bateria = 66.66;
	else if (voltage_bateria >= 12.0)
		porcentaje_bateria = 62.5;
	else if (voltage_bateria >= 11.9)
		porcentaje_bateria = 58.33;
	else if (voltage_bateria >= 11.8)
		porcentaje_bateria = 56.24;
	else if (voltage_bateria >= 11.7)
		porcentaje_bateria = 54.16;
	else if (voltage_bateria >= 11.6)
		porcentaje_bateria = 45.83;
	else if (voltage_bateria >= 11.4)
		porcentaje_bateria = 37.5;
	else if (voltage_bateria >= 11.3)
		porcentaje_bateria = 33.33;
	else if (voltage_bateria >= 11.2)
		porcentaje_bateria = 29.16;
	else if (voltage_bateria >= 11.1)
		porcentaje_bateria = 25.0;
	else if (voltage_bateria >= 11.0)
		porcentaje_bateria = 20.83;
	else if (voltage_bateria >= 10.9)
		porcentaje_bateria = 16.66;
	else if (voltage_bateria >= 10.8)
		porcentaje_bateria = 12.5;
	else if (voltage_bateria >= 10.7)
		porcentaje_bateria = 8.33;
	else if (voltage_bateria >= 10.6)
		porcentaje_bateria = 4.16;
	else
		porcentaje_bateria = 0;
 }

 else if (corriente_bateria > 0.35 && corriente_bateria <= 0.68) {
	if (voltage_bateria >= 12.7)
		porcentaje_bateria = 100;
	else if (voltage_bateria >= 12.6)
		porcentaje_bateria = 95.45;
	else if (voltage_bateria >= 12.5)
		porcentaje_bateria = 90.909;
	else if (voltage_bateria >= 12.4)
		porcentaje_bateria = 88.636;
	else if (voltage_bateria >= 12.3)
		porcentaje_bateria = 82.953;
	else if (voltage_bateria >= 12.2)
		porcentaje_bateria = 77.27;
	else if (voltage_bateria >= 12.1)
		porcentaje_bateria = 72.725;
	else if (voltage_bateria >= 12.0)
		porcentaje_bateria = 68.181;
	else if (voltage_bateria >= 11.9)
		porcentaje_bateria = 63.63;
	else if (voltage_bateria >= 11.8)
		porcentaje_bateria = 59.090;
	else if (voltage_bateria >= 11.7)
		porcentaje_bateria = 54.54;
	else if (voltage_bateria >= 11.6)
		porcentaje_bateria = 50;
	else if (voltage_bateria >= 11.5)
		porcentaje_bateria = 45.45;
	else if (voltage_bateria >= 11.4)
		porcentaje_bateria = 40.90;
	else if (voltage_bateria >= 11.3)
		porcentaje_bateria = 33.33;
	else if (voltage_bateria >= 11.2)
		porcentaje_bateria = 29.16;
	else if (voltage_bateria >= 11.1)
		porcentaje_bateria = 25.94;
	else if (voltage_bateria >= 11.0)
		porcentaje_bateria = 22.727;
	else if (voltage_bateria >= 10.9)
		porcentaje_bateria = 18.181;
	else if (voltage_bateria >= 10.8)
		porcentaje_bateria = 13.636;
	else if (voltage_bateria >= 10.7)
		porcentaje_bateria = 9.08;
	else if (voltage_bateria >= 10.6)
		porcentaje_bateria = 4.54;
	else
		porcentaje_bateria = 0;
 }
}


// ============================ MOTOR ============================
// motor_horario_X => Pulso de "pulso_motor" milisegundos en sentido horario (EJE X)
// motor_antihorario_X => Pulso de "pulso_motor" milisegundos en sentido antihorario (EJE X)
// motor_horario_Y => Pulso de "pulso_motor" milisegundos en sentido horario (EJE Y)
// motor_antihorario_Y => Pulso de "pulso_motor" milisegundos en sentido antihorario (EJE Y)
// ver_hora => determinar el angulo en ambos ejes segun la hora
// rutina_de_arranque => Calibrar el panel en 180Âº
void mover_horario_X() {
	//Serial.println(vel_x);
	if (angulo_X >= ang_max_X) {
		digitalWrite (EN1, LOW);
		digitalWrite (EN2, LOW);
		digitalWrite (IN1, LOW);
		digitalWrite (IN2, LOW);
	}
	else {
		if (angulo_X > 92 && angulo_X <= 105)//bajada
			vel_x = vel_min_ejeX * 0.9;
		else if (angulo_X > 105 && angulo_X <= 115)
			vel_x = vel_min_ejeX * 0.875;
		else if (angulo_X > 115 && angulo_X <= 125)
			vel_x = vel_min_ejeX * 0.85;
		else if (angulo_X > 125)
			vel_x = vel_min_ejeX * 0.8;
		else if (angulo_X < 88 && angulo_X > 80)//subida
			vel_x = vel_min_ejeX;
		else if (angulo_X <= 80 && angulo_X > 75)
			vel_x = vel_min_ejeX * 1.05;
		else if (angulo_X <= 75 && angulo_X > 70)
			vel_x = vel_min_ejeX * 1.08;
		else if (angulo_X <= 70 && angulo_X > 65)
			vel_x = vel_min_ejeX * 1.14;
		else if (angulo_X <= 65 && angulo_X > 60)
			vel_x = vel_min_ejeX * 1.2;
		else if (angulo_X <= 60)
			vel_x = vel_min_ejeX * 1.22;
		else
		vel_x = vel_min_ejeX * 1.1;
		digitalWrite (IN1, HIGH);
		digitalWrite (IN2, LOW);
		analogWrite (EN1, vel_x);
		analogWrite (EN2, 0);
		Encender_LED_Verde();
	}
}


void mover_antihorario_X() {
	//Serial.println(vel_x);
	if (angulo_X <= ang_min_X) {
		digitalWrite (EN1, LOW);
		digitalWrite (EN2, LOW);
		digitalWrite (IN1, LOW);
		digitalWrite (IN2, LOW);
	}
	else
	{
		if (angulo_X < 88 && angulo_X >= 80)//bajada
			vel_x = vel_min_ejeX * 0.9;
		else if (angulo_X < 80 && angulo_X >= 70)
			vel_x = vel_min_ejeX * 0.875;
		else if (angulo_X < 70 && angulo_X >= 60)
			vel_x = vel_min_ejeX * 0.85;
		else if (angulo_X < 60)
			vel_x = vel_min_ejeX * 0.8;
		else if (angulo_X > 92 && angulo_X < 100)//subida
			vel_x = vel_min_ejeX;
		else if (angulo_X >= 100 && angulo_X < 105)
			vel_x = vel_min_ejeX * 1.05;
		else if (angulo_X >= 105 && angulo_X < 110)
			vel_x = vel_min_ejeX * 1.08;
		else if (angulo_X >= 110 && angulo_X < 115)
			vel_x = vel_min_ejeX * 1.14;
		else if (angulo_X >= 115 && angulo_X < 120)
			vel_x = vel_min_ejeX * 1.2;
		else if (angulo_X >= 120)
			vel_x = vel_min_ejeX * 1.25;
		else
		vel_x = vel_min_ejeX * 1.1;
		digitalWrite (IN1, LOW);
		digitalWrite (IN2, HIGH);
		analogWrite (EN1, vel_x);
		analogWrite (EN2, 0);
		Encender_LED_Verde();
	}
}


void mover_horario_Y() {
	digitalWrite (IN3, LOW);
	digitalWrite (IN4, HIGH);
	analogWrite (EN1, 0);
	analogWrite (EN2, vel_min_ejeY);
	delay(13);
	analogWrite (EN2, 0);
	Encender_LED_Verde();
}


void mover_antihorario_Y() {
	digitalWrite (IN3, HIGH);
	digitalWrite (IN4, LOW);
	analogWrite (EN1, 0);
	analogWrite (EN2, vel_min_ejeY);
	delay(13);
	analogWrite (EN2, 0);
	Encender_LED_Verde();
}


void rutina_de_arranque_X(float ang_X) { //ang = angulo a girar ; angulo = angulo actual leido por e
	int salida = 0;
	led_v = 1;
	while (!salida) {
		canal = 3;
		lectura_ADC();
		if (angulo_X < ang_X - rango_calibracion_X) { // ang < 88
			lectura_ADC();
			mover_horario_X();
			if (demo == 0) {
				Serial.print("(");
				Serial.print(angulo_X, 4);
				Serial.println(")");
			}
		}
		else if (angulo_X > ang_X + rango_calibracion_X) {// ang < 92
			lectura_ADC();
			mover_antihorario_X();
			if (demo == 0) {
				Serial.print("(");
				Serial.print(angulo_X, 4);
				Serial.println(")");
			}
		}
		else {
			analogWrite (IN1, 0);
			analogWrite (IN2, 0);
			lectura_ADC();
			Serial.print("ANGULO X ALCANZADO:");
			Serial.println(angulo_X, 2);
			salida = 1;
			led_v = 0;
			Encender_LED_Verde();
		}
	}
}


void rutina_de_arranque_Y(float ang_Y) { //ang = angulo a girar ; angulo = angulo actual leido por e
	int salida = 0;
	led_v = 1;
	while (!salida) {
		canal = 4;
		lectura_ADC();
		if (angulo_Y < ang_Y - rango_calibracion_Y) {// ang < -2
			lectura_ADC();
			mover_antihorario_Y();
			if (demo == 0) {
				Serial.print("(");
				Serial.print(angulo_Y, 4);
				Serial.println(")");
			}
		}
		else if (angulo_Y > ang_Y + rango_calibracion_Y) { //ang > 2
			lectura_ADC();
			mover_horario_Y();
			if (demo == 0) {
				Serial.print("(");
				Serial.print(angulo_Y, 2);
				Serial.println(")");
			}
		}
		else {
			analogWrite (IN3, 0);
			analogWrite (IN4, 0);
			lectura_ADC();
			Serial.print("ANGULO Y ALCANZADO:");
			Serial.println(angulo_Y, 2);
			salida = 1;
			led_v = 0;
			Encender_LED_Verde();
		}
	}
}


void modo_demo() {
	int s = 0;
	delay(100);
	geolocalizacion();
	delay(100);
	clima();
	delay(100);
	ver_hora();
	delay(100);
	hora = 6;
	minutos = 30;
	conectar_server();
	leer_ADC();
	Serial.println("MODO DEMOSTRACION");
	Serial.println("FECHA");
	Serial.print(dia);
	Serial.print("/");
	Serial.print(mes);
	Serial.println("/19");
	Serial.println("");
	posicion_del_sol();
	Serial.println("");
	Serial.println("");
	Serial.println("CALIBRACION INICIAL");
	rutina_de_arranque_X(90);
	delay(500);
	rutina_de_arranque_Y(0);
	Serial.println("");
	Serial.println("");

	while (!s) {
		ver_hora();
		Serial.println("");
		if (hora < 10)
		Serial.print("0");
		Serial.print(hora);
		Serial.print(":");
		if (minutos < 10)
		Serial.print("0");
		Serial.println(minutos);
		Serial.print("E: ");
		Serial.print(elevacion2[x_tabla], 2);
		Serial.print(" A: ");
		Serial.println(azimuth[x_tabla], 2);
		Serial.print("");
		rutina_de_arranque_X(elevacion2[x_tabla]);
		rutina_de_arranque_Y(azimuth[x_tabla]);
		minutos += 30;
		if (minutos == 60) {
			minutos = 0;
			hora++;
			leer_ADC();
			if (conexion_server)
			enviar_al_cliente(10);
			else {
				conectar_server();
				enviar_al_cliente(10);
			}
		}
		if (hora == 20) {
			hora = 6;
			minutos = 0;
			Serial.print("");
			Serial.print("RUTINA COMPLETADA... REINICIANDO.");
			Serial.print("");
			delay(5000);
			rutina_de_arranque_X(90);
			rutina_de_arranque_Y(0);
		}
		delay(2000);
	}
}


void ver_hora() {
	if (hora == 6 && (minutos >= 30 && minutos < 59))
		x_tabla = 1;
	else if (hora == 7 && (minutos >= 0 && minutos < 30))
		x_tabla = 2;
	else if (hora == 7 && (minutos >= 30 && minutos < 59))
		x_tabla = 3;
	else if (hora == 8 && (minutos >= 0 && minutos < 30))
		x_tabla = 4;
	else if (hora == 8 && (minutos >= 30 && minutos < 59))
		x_tabla = 5;
	else if (hora == 9 && (minutos >= 0 && minutos < 30))
		x_tabla = 6;
	else if (hora == 9 && (minutos >= 30 && minutos < 59))
		x_tabla = 7;
	else if (hora == 10 && (minutos >= 0 && minutos < 30))
		x_tabla = 8;
	else if (hora == 10 && (minutos >= 30 && minutos < 59))
		x_tabla = 9;
	else if (hora == 11 && (minutos >= 0 && minutos < 30))
		x_tabla = 10;
	else if (hora == 11 && (minutos >= 30 && minutos < 59))
		x_tabla = 11;
	else if (hora == 12 && (minutos >= 0 && minutos < 30))
		x_tabla = 12;
	else if (hora == 12 && (minutos >= 30 && minutos < 59))
		x_tabla = 13;
	else if (hora == 13 && (minutos >= 0 && minutos < 30))
		x_tabla = 14;
	else if (hora == 13 && (minutos >= 30 && minutos < 59))
		x_tabla = 15;
	else if (hora == 14 && (minutos >= 0 && minutos < 30))
		x_tabla = 16;
	else if (hora == 14 && (minutos >= 30 && minutos < 59))
		x_tabla = 17;
	else if (hora == 15 && (minutos >= 0 && minutos < 30))
		x_tabla = 18;
	else if (hora == 15 && (minutos >= 30 && minutos < 59))
		x_tabla = 19;
	else if (hora == 16 && (minutos >= 0 && minutos < 30))
		x_tabla = 20;
	else if (hora == 16 && (minutos >= 30 && minutos < 59))
		x_tabla = 21;
	else if (hora == 17 && (minutos >= 0 && minutos < 30))
		x_tabla = 22;
	else if (hora == 17 && (minutos >= 30 && minutos < 59))
		x_tabla = 23;
	else if (hora == 18 && (minutos >= 0 && minutos < 30))
		x_tabla = 24;
	else if (hora == 18 && (minutos >= 30 && minutos < 59))
		x_tabla = 25;
	else if (hora == 19 && (minutos >= 0 && minutos < 30))
		x_tabla = 26;
	else if (hora == 19 && (minutos >= 30 && minutos < 59))
		x_tabla = 27;
	else if (hora == 20 && (minutos >= 0 && minutos < 30))
		x_tabla = 28;
	else if (hora == 20 && (minutos >= 30 && minutos < 59))
		x_tabla = 29;
	else
		x_tabla = 0;
}


void enviar_al_cliente(int intentos) {
	Serial.print("Envio");
	StaticJsonBuffer<2000> jsonBuffer;
	JsonObject& trama = jsonBuffer.createObject();
	WiFiClient client = server.available();
	client.setNoDelay(1);
	trama["latitud"] = lat;
	trama["longitud"] = lng;
	trama["hora"] = hora;
	trama["minutos"] = minutos;
	trama["dia"] = dia;
	trama["mes"] = mes;
	trama["anio"] = anio - 2000;
	trama["localidad"] = localidad;
	trama["pais"] = pais;
	trama["detalle"] = detalle_clima;
	trama["temperatura"] = temp;
	trama["humedad"] = humedad;
	trama["ang_X"] = angulo_X;
	trama["ang_Y"] = angulo_Y;
	trama["elevacion"] = elevacion2[x_tabla];
	trama["azimuth"] = azimuth[x_tabla];
	trama["corriente_bateria"] = corriente_bateria;
	trama["voltage_bateria"] = voltage_bateria;
	trama["porcentaje_bateria"] = porcentaje_bateria;
	trama["corriente_panel"] = corriente_panel;
	Serial.print("... ");
	while (intentos) {
		client = server.available();
		if (client) {
			if (client.connected()) {
				trama.printTo(client);
				client.println("\r");
				Serial.println("OK");
				intentos = 0;
			}
			client.stop(); // terminates the connection with the client
		}
	}
}


void modo_debug() {
	int x;
	/*lat = -31.4419845;
	lng = -64.1923504;
	dia = 24;
	mes = 10;
	anio = 2018;
	localidad = "Cordoba";
	pais = "AR";
	detalle_clima = "Despejado";*/
	delay(100);
	geolocalizacion();
	delay(100);
	clima();
	delay(100);
	ver_hora();
	delay(100);
	posicion_del_sol();
	delay(100);
	conectar_server();
	rutina_de_arranque_X(90);
	rutina_de_arranque_Y(0);

	while (1) {
		Serial.println("PROBANDO LECTURAS ADC");
		delay(300);
		canal = 1;
		lectura_ADC();
		Serial.println("");
		Serial.print(digitalRead(MUX2));
		Serial.print(digitalRead(MUX1));
		Serial.println(digitalRead(MUX0));
		Serial.println("CORRIENTE BATERIA:");
		for (x = 0; x < 20; x++) {
			lectura_ADC();
			Serial.println(corriente_bateria, 4);
			delay(100);
		}
		Serial.println("");
		delay(300);
		canal = 2;
		lectura_ADC();
		Serial.println("");
		Serial.print(digitalRead(MUX2));
		Serial.print(digitalRead(MUX1));
		Serial.println(digitalRead(MUX0));
		Serial.println("TENSION BATERIA:");
		for (x = 0; x < 20; x++) {
			lectura_ADC();
			Serial.println(voltage_bateria, 4);
			delay(100);
		}
		Serial.println("");
		delay(300);
		canal = 3;
		lectura_ADC();
		Serial.println("");
		Serial.print(digitalRead(MUX2));
		Serial.print(digitalRead(MUX1));
		Serial.println(digitalRead(MUX0));
		Serial.println("ANGULO EJE X:");
		for (x = 0; x < 20; x++) {
			lectura_ADC();
			Serial.println(angulo_X, 4);
			delay(100);
		}
		Serial.println("");
		delay(300);
		canal = 4;
		lectura_ADC();
		Serial.println("");
		Serial.print(digitalRead(MUX2));
		Serial.print(digitalRead(MUX1));
		Serial.println(digitalRead(MUX0));
		Serial.println("ANGULO EJE Y:");
		for (x = 0; x < 20; x++) {
			lectura_ADC();
			Serial.println(angulo_Y, 4);
			delay(100);
		}
		Serial.println("");
		delay(300);
		canal = 5;
		lectura_ADC();
		Serial.println("");
		Serial.print(digitalRead(MUX2));
		Serial.print(digitalRead(MUX1));
		Serial.println(digitalRead(MUX0));
		Serial.println("CORRIENTE PANEL:");
		for (x = 0; x < 20; x++) {
			lectura_ADC();
			Serial.println(corriente_panel, 4);
			delay(100);
		}
		Serial.println("");
		//enviar_al_cliente(10);
		delay(3000);
	}
}


void conectar_server() {
	IPAddress ip(192, 168, 0, port_server); // IP address of the server
	IPAddress gateway(192, 168, 0, 1); // gateway of your network
	IPAddress subnet(255, 255, 255, 0); // subnet mask of your network
	WiFi.config(ip, gateway, subnet);
	server.begin();
	if (server.available())
		conexion_server = 1;
	else
		conexion_server = 0;
}


void leer_ADC() {
	canal = 1;
	lectura_ADC();
	canal = 2;
	lectura_ADC();
	canal = 3;
	lectura_ADC();
	canal = 4;
	lectura_ADC();
	canal = 5;
	lectura_ADC();
}