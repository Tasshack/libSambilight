//##### SETTINGS ############

#define BAUDRATE   921600   // Serial port speed
#define NUM_LEDS   40     // Number of leds
#define BRIGHTNESS   255    // Maximum brightness
#define LED_TYPE   WS2812  // Led strip type for FastLED
#define COLOR_ORDER  GRB    // Led color order
#define PIN_DATA   14     // Led data output pin
//#define PIN_CLOCK 12      // Led data clock pin (uncomment if you're using a 4-wire LED type)

//###########################




#if defined(ESP8266)
#define FASTLED_ESP8266_RAW_PIN_ORDER
#endif

#include <Arduino.h>
#include <FastLED.h>

CRGB leds[NUM_LEDS];
uint8_t* ledsRaw = (uint8_t*)leds;

const uint8_t magic[] = { 'A','d','a' };
#define MAGICSIZE  sizeof(magic)
#define HICHECK    (MAGICSIZE)
#define LOCHECK    (MAGICSIZE + 1)
#define CHECKSUM   (MAGICSIZE + 2)

enum processModes_t { Header, Data } mode = Header;

int16_t c, outPos, bytesRemaining;
unsigned long t, lastByteTime;
uint8_t headPos, hi, lo, chk;

void setup() {
#if defined(PIN_CLOCK) && defined(PIN_DATA)
    FastLED.addLeds<LED_TYPE, PIN_DATA, PIN_CLOCK, COLOR_ORDER>(leds, NUM_LEDS);
#elif defined(PIN_DATA)
    FastLED.addLeds<LED_TYPE, PIN_DATA, COLOR_ORDER>(leds, NUM_LEDS);
#else
#error "No LED output pins defined. Check your settings at the top."
#endif

    FastLED.setBrightness(BRIGHTNESS);
    FastLED.show();

#if defined(ESP8266)
    Serial.setRxBufferSize(2048);
#endif

    Serial.begin(BAUDRATE);

#if defined(ESP8266)
    delay(500);
    Serial.swap(); // RX pin will be GPIO13
    delay(500);
#endif

    lastByteTime = millis();
}

void loop() {
    t = millis();

    if ((c = Serial.read()) >= 0) {
        lastByteTime = t;

        switch (mode) {
        case Header:
            if (headPos < MAGICSIZE) {
                if (c == magic[headPos]) { headPos++; }
                else { headPos = 0; }
            }
            else {
                switch (headPos) {
                case HICHECK:
                    hi = c;
                    headPos++;
                    break;
                case LOCHECK:
                    lo = c;
                    headPos++;
                    break;
                case CHECKSUM:
                    chk = c;
                    if (chk == (hi ^ lo ^ 0x55)) {
                        bytesRemaining = 3L * (256L * (long)hi + (long)lo + 1L);
                        outPos = 0;
                        memset(leds, 0, NUM_LEDS * sizeof(struct CRGB));
                        mode = Data;
                    }
                    headPos = 0;
                    break;
                }
            }
            break;
        case Data:
            if (outPos < sizeof(leds)) {
                ledsRaw[outPos++] = c;
            }
            bytesRemaining--;

            if (bytesRemaining == 0) {
                mode = Header;
                while (Serial.available() > 0) {
                    Serial.read();
                }
                FastLED.show();
            }
            break;
        }
    }
    else if (((t - lastByteTime) >= (uint32_t)120 * 60 * 1000 && mode == Header) || ((t - lastByteTime) >= (uint32_t)1000 && mode == Data)) {
        memset(leds, 0, NUM_LEDS * sizeof(struct CRGB));
        FastLED.show();
        mode = Header;
        lastByteTime = t;
    }
}