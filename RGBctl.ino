// RGBctl - Bluetooth RGB controller software for arduino
// TODO:
// - fades not working properly
// - sequences not supported at all

#include <SoftwareSerial.h>
#include <avr\wdt.h>

#define REDPIN 3
#define GREENPIN 5
#define BLUEPIN 6
#define COM btCom

#define ZEROCUTOFF 1 // TODO: find out why sending low chars shits the bed
//#define _DEBUG
#define WATCHDOG

//SoftwareSerial btCom(12, 11);
SoftwareSerial btCom(4, 7);
unsigned char r, g, b; // current colors
char sequence[64];


void setup() {
#ifdef WATCHDOG
	wdt_disable();
	delay(3000); // just in case i mess up and end up in a watchdog reset loop :)
	wdt_enable(WDTO_8S); // if using nano then flash optiboot bootloader in case the watchdog reset crashes
#endif

	COM.begin(9600);
#ifdef _DEBUG
	Serial.begin(9600);
	COM.println("RGBctl started\n");
#endif
	pinMode(REDPIN, OUTPUT);
	pinMode(GREENPIN, OUTPUT);
	pinMode(BLUEPIN, OUTPUT);
	r = 0; g = 0; b = 0;
	sequence[0] = '\0';
	setRGB(0, 0, 0);
}

void refreshRGB() {
#ifdef _DEBUG
	Serial.print(r);
	Serial.print(":");
	Serial.print(g);
	Serial.print(":");
	Serial.print(b);
	Serial.print("\n");
#endif
	analogWrite(REDPIN, r);
	analogWrite(GREENPIN, g);
	analogWrite(BLUEPIN, b);
}

// set colors without fade, set to -1 if you want to leave that color unchanged
void setRGB(unsigned char newR, unsigned char newG, unsigned char newB) {
	r = newR > ZEROCUTOFF ? newR : 0;
	g = newG > ZEROCUTOFF ? newG : 0;
	b = newB > ZEROCUTOFF ? newB : 0;
	refreshRGB();
}

void fadeRGB(unsigned char newR, unsigned char newG, unsigned char newB, unsigned char delayMs) {
	newR = newR > ZEROCUTOFF ? newR : 0;
	newG = newG > ZEROCUTOFF ? newG : 0;
	newB = newB > ZEROCUTOFF ? newB : 0;
	do {
		if (newR != r) {
			if (newR > r) {	r += 1; }
			else { r -= 1; }
		}

		if (newG != g) {
			if (newG > g) { g += 1; }
			else { g -= 1; }
		}

		if (newB != b) {
			if (newB > b) { b += 1; }
			else { b -= 1; }
		}
		refreshRGB();
		delay(delayMs);
	}while ((newR != r) && (newG != g) && (newB != b));
}

void increaseBrightness(unsigned char value) {
	// TODO: check for overflow?
#ifdef _DEBUG
	Serial.print("+ ");
	Serial.print(value);
	Serial.print("\n");
#endif
	r += value;
	g += value;
	b += value;
	refreshRGB();
}

void decreaseBrightness(unsigned char value) {
	// TODO: check for underflow?
#ifdef _DEBUG
	Serial.print("- ");
	Serial.print(value);
	Serial.print("\n");
#endif
	r -= value;
	g -= value;
	b -= value;
	refreshRGB();
}

void loop() {
#ifdef WATCHDOG
	wdt_reset();
#endif
	if (COM.available() > 0) {
		delay(10); // let the buffer fill a bit
		unsigned char c = COM.read(), tr, tg, tb;
		unsigned char buf;
		unsigned char packet[6];
		switch (c) {
		case 'f': // fade
				COM.readBytes(packet, 4);
				tr = packet[0];
				tg = packet[1];
				tb = packet[2];

				fadeRGB(tr, tg, tb, packet[3]);
				break;
		case 's': // set
				COM.readBytes(packet, 3);
				tr = packet[0];
				tg = packet[1];
				tb = packet[2];
				setRGB(tr, tg, tb);
				break;
#ifdef _DEBUG
		case 'c': // clear
				setRGB(0, 0, 0);
				break;
		case 'r': // red 100%
				setRGB(255, 0, 0);
				break;
		case 'g': // green 100%
				setRGB(0, 255, 0);
				break;
		case 'b': // blue 100%
				setRGB(0, 0, 255);
				break;
		case 'a': // all 100%
				setRGB(255, 255, 255);
				break;
		case 'o':
				COM.println("OK\n");
				break;
#endif
		case '+': // brightness -
				COM.readBytes(packet, 1);
				increaseBrightness(packet[0]);
				break;
		case '-': // brightness -
				COM.readBytes(packet, 1);
				decreaseBrightness(packet[0]);
				break;
		case 'v':
				COM.print(r);
				COM.print(g);
				COM.print(b);
				break;
		case 'q': // sequence data
			COM.write(sequence);
			break;
		case 'w':
			COM.readBytesUntil('\0', sequence, sizeof(sequence));
			break;
		default:
			break;
		}
	}
}
