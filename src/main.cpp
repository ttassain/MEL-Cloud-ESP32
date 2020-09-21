#include <Arduino.h>
#include <WiFi.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

HTTPClient http;

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire);

// WiFi
const char* ssid = "WIFI_NAME";
const char* password = "WIFI_PASSWORD";

// Mel Cloud
String contextKey = "YOUR_KEY";
int buildingId = 163047;
int splitIds[] = { 238535, 238531, 238517 };
String splitNames[] = { "Bureau", "Chambre", "Salon" };
double roomTemperature[] = { 0.0, 0.0, 0.0 };
double setTemperature[] = { 0.0, 0.0, 0.0 };
boolean power[] = { false, false, false };
int setFanSpeed[] = { 0, 0, 0 };
int numberOfFanSpeeds[] = { 0, 0, 0 };
int vaneHorizontal[] = { 0, 0, 0 };
int vaneVertical[] = { 0, 0, 0 };

// Mel Cloud Action EffectiveFlags
const int FLAG_POWER = 1;
const int FLAG_OPERATION_MODE = 2;
const int FLAG_DESIRED_TEMPERATURE = 4;
const int FLAG_FAN_SPEED = 8;
const int FLAG_VANES_VERTICAL = 16;
const int FLAG_VANES_HORIZONTAL = 256;

// GPIOs
const int BUTTON_UP = 13;
const int BUTTON_DOWN = 12;
const int BUTTON_RIGHT = 32;
const int BUTTON_LEFT = 33;
const int BUTTON_OK = 14;
const int BUTTON_SET = 26;
const int BUTTON_CLEAR = 25;
const int LED_RED = 2;
const int LED_GREEN = 15;
const int BUZZER = 4;

enum Action { NONE, TEST, ERROR, REFRESH_SCREEN, GET_DEVICES, SET_DEVICE, SET_ALL_DEVICES };

int flag = 0;

unsigned long lastDevicesRefresh = millis();
unsigned long lastMillisButton = 0;
const unsigned long debounceTime = 250;
boolean isBuzy = false;
const int REFRESH_DEVICES_TIME_IN_MILLI = 10 * 60 * 1000;

const int FUNCTION_VANE_VERTICAL = 2;
const int FUNCTION_VANE_HORIZONTAL = 3;
const int FUNCTION_FAN_SPEED = 4;
const int FUNCTION_TEMPERATURE = 5;
const int FUNCTION_POWER = 6;
const int FUNCTION_ALL = 7;

int selectedDevice = 0;
int selectedFunction = FUNCTION_POWER;
Action action = GET_DEVICES;

void printOled(int device, int line, String msg) {
	int l = line * 8;
	int c;
	if (device == 0) c = 0;
	if (device == 1) c = 6 * 7;
	if (device == 2) c = 6 * 15;

	display.setCursor(c, l);

	boolean showArrow = (line == selectedFunction && selectedDevice == device) || (line == selectedFunction && selectedFunction == FUNCTION_ALL);

	display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
	display.write(showArrow ? 16 : 32);

	boolean showSelected = (selectedDevice == device && line == 0) || (line == 0 && selectedFunction == FUNCTION_ALL);
	if (showSelected) {
		display.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
	} else {
		display.setTextColor(SSD1306_WHITE, SSD1306_BLACK);
	}
	display.print(msg);
}

void displayDevices() {
	display.clearDisplay();
	for (int device=0; device <= 2; device++) {
		
		int vaneH = vaneHorizontal[device];
		String vaneHorizontalText;
		if (vaneH == 0) {
			vaneHorizontalText = "AUTO";
		} else if (vaneH == 12) {
			vaneHorizontalText = "MOVE";
		} else {
			vaneHorizontalText = String(vaneH) + " / 5";
		}

		int vaneV = vaneVertical[device];
		String vaneVerticalText;
		if (vaneV == 0) {
			vaneVerticalText = "AUTO";
		} else if (vaneV == 7) {
			vaneVerticalText = "MOVE";
		} else {
			vaneVerticalText = String(vaneV) + " / 5";
		}
	
		printOled(device, 0, String(splitNames[device]));
		printOled(device, 1, String(roomTemperature[device]));
		printOled(device, FUNCTION_VANE_VERTICAL, vaneVerticalText);
		printOled(device, FUNCTION_VANE_HORIZONTAL, vaneHorizontalText);
		printOled(device, FUNCTION_FAN_SPEED, setFanSpeed[device] == 0 ? "AUTO" : String(setFanSpeed[device]) + " / " + String(numberOfFanSpeeds[device]));
		printOled(device, FUNCTION_TEMPERATURE, String(setTemperature[device]));
		printOled(device, FUNCTION_POWER, power[device] ? "ON" : "OFF");
	}

	//display.drawFastHLine(0, 52, 128, SSD1306_WHITE);
	printOled(0, FUNCTION_ALL, "ON/OFF global");

	display.display();
	action = NONE;
}

boolean isInterruptReady() {
	if (millis() - lastMillisButton > debounceTime && !isBuzy) {
		lastMillisButton = millis();
		return true;
	} else {
		return false;
	}
}

void buttonUpInterrupt() {
	if (isInterruptReady()) {
		if (selectedFunction == FUNCTION_ALL) {
			selectedFunction = FUNCTION_POWER;
		} else if (selectedFunction == FUNCTION_POWER) {
			selectedFunction = FUNCTION_TEMPERATURE;
		} else if (selectedFunction == FUNCTION_TEMPERATURE) {
			selectedFunction = FUNCTION_FAN_SPEED;
		} else if (selectedFunction == FUNCTION_FAN_SPEED) {
			selectedFunction = FUNCTION_VANE_HORIZONTAL;
		} else if (selectedFunction == FUNCTION_VANE_HORIZONTAL) {
			selectedFunction = FUNCTION_VANE_VERTICAL;
		} else if (selectedFunction == FUNCTION_VANE_VERTICAL) {
			selectedFunction = FUNCTION_ALL;
		}
		action = REFRESH_SCREEN;
	}
}

void buttonDownInterrupt() {
	if (isInterruptReady()) {
		if (selectedFunction == FUNCTION_ALL) {
			selectedFunction = FUNCTION_VANE_VERTICAL;
		} else if (selectedFunction == FUNCTION_POWER) {
			selectedFunction = FUNCTION_ALL;
		} else if (selectedFunction == FUNCTION_TEMPERATURE) {
			selectedFunction = FUNCTION_POWER;
		} else if (selectedFunction == FUNCTION_FAN_SPEED) {
			selectedFunction = FUNCTION_TEMPERATURE;
		} else if (selectedFunction == FUNCTION_VANE_HORIZONTAL) {
			selectedFunction = FUNCTION_FAN_SPEED;
		} else if (selectedFunction == FUNCTION_VANE_VERTICAL) {
			selectedFunction = FUNCTION_VANE_HORIZONTAL;
		}
		action = REFRESH_SCREEN;
	}
}

void buttonRightInterrupt() {
	if (isInterruptReady()) {
		if (selectedFunction == FUNCTION_ALL) {
			// TODO changer de sous menu
		} else {
			selectedDevice = selectedDevice + 1;
			if (selectedDevice > 2) {
				selectedDevice = 0;
			}
			action = REFRESH_SCREEN;
		}
	}
}

void buttonLeftInterrupt() {
	if (isInterruptReady()) {
		if (selectedFunction == FUNCTION_ALL) {
			// TODO changer de sous menu
		} else {
			selectedDevice = selectedDevice - 1;
			if (selectedDevice < 0) {
				selectedDevice = 2;
			}
			action = REFRESH_SCREEN;
		}
	}
}

void buttonOkInterrupt() {
	if (isInterruptReady()) {

		// TODO pour test
		action = TEST;
	}
}

void buttonSetInterrupt() {
	if (isInterruptReady()) {
		if (selectedFunction == FUNCTION_POWER) {
			power[selectedDevice] = power[selectedDevice] ? false : true;
			flag += FLAG_POWER;
			action = SET_DEVICE;
		} else if (selectedFunction == FUNCTION_TEMPERATURE) {
			setTemperature[selectedDevice] = setTemperature[selectedDevice] + 1.00;
			if (setTemperature[selectedDevice] > 26) {
				setTemperature[selectedDevice] = 16;
			}
			flag += FLAG_DESIRED_TEMPERATURE;
			action = SET_DEVICE;
		} else if (selectedFunction == FUNCTION_FAN_SPEED) {
			setFanSpeed[selectedDevice] = setFanSpeed[selectedDevice] + 1;
			if (setFanSpeed[selectedDevice] > numberOfFanSpeeds[selectedDevice]) {
				setFanSpeed[selectedDevice] = 0;
			}
			flag += FLAG_FAN_SPEED;
			action = SET_DEVICE;
		} else if (selectedFunction == FUNCTION_VANE_HORIZONTAL) {
			// TODO a faire
			// Position de 1 a 5, 12 = balayage 0 = auto
		} else if (selectedFunction == FUNCTION_VANE_VERTICAL) {
			// TODO a faire
			// Position de 1 a 5, 7 = balayage, 0 = auto
		} else if (selectedFunction == FUNCTION_ALL) {
			for (int device=0; device <= 2; device++) {
				power[device] = true;
			}
			flag += FLAG_POWER;
			action = SET_ALL_DEVICES;
		} 
	}
}

void buttonClearInterrupt() {
	if (isInterruptReady()) {
		if (selectedFunction == FUNCTION_POWER) {
			power[selectedDevice] = power[selectedDevice] ? false : true;
			flag += FLAG_POWER;
			action = SET_DEVICE;
		} else if (selectedFunction == FUNCTION_TEMPERATURE) {
			setTemperature[selectedDevice] = setTemperature[selectedDevice] - 1.00;
			if (setTemperature[selectedDevice] < 16) {
				setTemperature[selectedDevice] = 25;
			}
			flag += FLAG_DESIRED_TEMPERATURE;
			action = SET_DEVICE;
		} else if (selectedFunction == FUNCTION_FAN_SPEED) {
			setFanSpeed[selectedDevice] = setFanSpeed[selectedDevice] - 1;
			if (setFanSpeed[selectedDevice] < 0) {
				setFanSpeed[selectedDevice] = numberOfFanSpeeds[selectedDevice];
			}
			flag += FLAG_FAN_SPEED;
			action = SET_DEVICE;
		} else if (selectedFunction == FUNCTION_VANE_HORIZONTAL) {
			// TODO a faire
			// Position de 1 a 5, 12 = balayage
		}  else if (selectedFunction == FUNCTION_VANE_VERTICAL) {
			// TODO a faire
			// Position de 1 a 5, 7 = balayage
		} else if (selectedFunction == FUNCTION_ALL) {
			for (int device=0; device <= 2; device++) {
				power[device] = false;
			}
			flag += FLAG_POWER;
			action = SET_ALL_DEVICES;
		}
	}
}

void bip() {
	digitalWrite(BUZZER, LOW);
    delay(10);
    digitalWrite(BUZZER, HIGH);
}

void setBuzy(boolean buzy) {
	isBuzy = buzy;
	digitalWrite(LED_GREEN, buzy ? LOW : HIGH);
	digitalWrite(LED_RED, buzy ? HIGH : LOW);
}

void initWifi() {
	// Connexion au Wifi
	Serial.print("Connecting to WiFi...");
	WiFi.begin(ssid, password);
		while (WiFi.status() != WL_CONNECTED) {
		delay(500);
	}
	Serial.println("ok !");
	Serial.printf("SSID: %s, IP: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

void initButtonsAndLeds() {
	pinMode(BUTTON_UP, INPUT_PULLUP);
	pinMode(BUTTON_DOWN, INPUT_PULLUP);
	pinMode(BUTTON_RIGHT, INPUT_PULLUP);
	pinMode(BUTTON_LEFT, INPUT_PULLUP);
	pinMode(BUTTON_OK, INPUT_PULLUP);
	pinMode(BUTTON_SET, INPUT_PULLUP);
	pinMode(BUTTON_CLEAR, INPUT_PULLUP);

	attachInterrupt(BUTTON_UP, buttonUpInterrupt, FALLING);
	attachInterrupt(BUTTON_DOWN, buttonDownInterrupt, FALLING);
	attachInterrupt(BUTTON_RIGHT, buttonRightInterrupt, FALLING);
	attachInterrupt(BUTTON_LEFT, buttonLeftInterrupt, FALLING);
	attachInterrupt(BUTTON_OK, buttonOkInterrupt, FALLING);
	attachInterrupt(BUTTON_SET, buttonSetInterrupt, FALLING);
	attachInterrupt(BUTTON_CLEAR, buttonClearInterrupt, FALLING);

	pinMode(LED_RED, OUTPUT);
	pinMode(LED_GREEN, OUTPUT);
}

void initOled() {
	// OLED Display SSD1306 (SCL -> GPIO22, SDA -> GPIO21, Vin -> 3.3V, GND -> GND)
	// https://randomnerdtutorials.com/esp32-ssd1306-oled-display-arduino-ide/
	if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
		Serial.println(F("SSD1306 allocation failed"));
		for(;;);
	}
	display.clearDisplay();
	display.setTextSize(1);
	display.cp437(true); // https://en.wikipedia.org/wiki/Code_page_437
	display.display();
}

void setup() {
	Serial.begin(115200);
	while (!Serial) {
		; // wait for serial port to connect. Needed for native USB port only
	}
	
	initButtonsAndLeds();

	setBuzy(true);

	pinMode(BUZZER, OUTPUT);
	digitalWrite(BUZZER, HIGH);

	initOled();

	initWifi();

	setBuzy(false);
}

void getDevice(int device) {
	String deviceId = String(splitIds[device]);

	String url = "https://app.melcloud.com/Mitsubishi.Wifi.Client/Device/Get?id=" + deviceId + "&buildingID=" + buildingId;
	http.begin(url);
	http.addHeader("Accept", "application/json");
	http.addHeader("Content-Type", "application/json");
	http.addHeader("X-MitsContextKey", contextKey);
	int httpCode = http.GET();
 
	if (httpCode == 200) {
		String payload = http.getString();
		// Serial.println(payload);

		DynamicJsonDocument doc(3000);
		deserializeJson(doc, payload);

		roomTemperature[device] = doc["RoomTemperature"];
		setTemperature[device] = doc["SetTemperature"];
		power[device] = doc["Power"];
		setFanSpeed[device] = doc["SetFanSpeed"];
		numberOfFanSpeeds[device] = doc["NumberOfFanSpeeds"];
		vaneHorizontal[device] = doc["VaneHorizontal"];
		vaneVertical[device] = doc["VaneVertical"];
		// nextCommunication[device] = doc["NextCommunication"];
		// lastCommunication[device] = doc["LastCommunication"];
		// operationMode[device] = doc["OperationMode"];			// 1=HEATING  3=COOLING  8=AUTOMATIC

		action = REFRESH_SCREEN;
	} else {
		Serial.println("Error on HTTP request");
		action = ERROR;
	}
	http.end();
}

void setDevice(int device) {
	DynamicJsonDocument doc(1024);
	
	doc["DeviceID"] = splitIds[device];
	doc["EffectiveFlags"] = flag;
	doc["Power"] = power[device];
	doc["RoomTemperature"] = roomTemperature[device];
	doc["SetTemperature"] = setTemperature[device];
	doc["SetFanSpeed"] = setFanSpeed[device];
	doc["NumberOfFanSpeeds"] = numberOfFanSpeeds[device];
	doc["VaneHorizontal"] = vaneHorizontal[device];
	doc["VaneVertical"] = vaneVertical[device];
	doc["HasPendingCommand"] = true;

	String payload;
	serializeJson(doc, payload);
	Serial.println(payload);

	String url = "https://app.melcloud.com/Mitsubishi.Wifi.Client/Device/SetAta";
	http.begin(url);
	http.addHeader("Accept", "application/json");
	http.addHeader("Content-Type", "application/json");
	http.addHeader("X-MitsContextKey", contextKey);
	int httpCode = http.POST(payload);
	
	if (httpCode == 200) {
		action = GET_DEVICES;
	} else {
		Serial.println("Error on HTTP request");
		action = ERROR;
	}
}

void setSelectedDevice() {
	setBuzy(true);
	setDevice(selectedDevice);
	flag = 0;
	setBuzy(false);
}

void setAllDevices() {
	setBuzy(true);
	for (int device=0; device <= 2; device++) {
		setDevice(device);
	}
	flag = 0;
	setBuzy(false);
}

void getAllDevices() {
	setBuzy(true);
	Serial.print("getAllDevices...");
	for (int device=0; device <= 2; device++) {
		getDevice(device);
	}
	Serial.println("done !");
	action = REFRESH_SCREEN;
	setBuzy(false);
}

void test() {
	bip();
	action = GET_DEVICES;
}

void loop() {

	// TODO calculé le flag par ajout de bit pour cumuler les commandes
	// TODO décalé l'envoi du setDevice pour passer plusieur commande en meme temps

	switch (action) {
		case REFRESH_SCREEN:
			displayDevices();
			break;
		case GET_DEVICES:
			getAllDevices();
			break;
		case SET_DEVICE:
			setSelectedDevice();
			break;
		case SET_ALL_DEVICES:
			setAllDevices();
			break;
		case ERROR:
			bip();
			break;
		case TEST:
			test();
			break;
		case NONE:
			// Ne rien faire !
			break;
	}

	// Check pour un refresh tout les 10 minutes
	if (millis() - lastDevicesRefresh > REFRESH_DEVICES_TIME_IN_MILLI && action == NONE) {
		lastDevicesRefresh = millis();
		action = GET_DEVICES;
	}

  	delay(debounceTime);
}
