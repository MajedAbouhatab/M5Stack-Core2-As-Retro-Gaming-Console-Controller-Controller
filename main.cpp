#if defined(ARDUINO_ESP8266_ESP01)

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);
int CountA = 0, CountD = 0, PinO = 0, PinA = 1, PinD = 2, PinS = 3;
bool MessageReceived = false;

void ToggleWait(int B, int S)
{
    digitalWrite(B, !digitalRead(B));
    delay(S);
}

void setup(void)
{
    for (int i = 0; i < 4; i++)
    {
        pinMode(i, FUNCTION_3); //********** CHANGE PIN FUNCTION  TO GPIO **********
        pinMode(i, OUTPUT);
        digitalWrite(i, 1);
    }
    WiFi.softAP("ESP8266-Access-Point", "ESP8266-Access-Point");

    server.on(
        "/", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL,
        [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
        {
            data[len] = '\0';
            char *strtokIndx;
            strtokIndx = strtok((char *)data, ",");
            CountA = atoi(strtokIndx);
            strtokIndx = strtok(NULL, ",");
            CountD = atoi(strtokIndx);
            MessageReceived = true;
            request->send(200);
        });

    server.begin();
}

void loop(void)
{
    if (MessageReceived)
    {
        MessageReceived = false;
        // Rest
        ToggleWait(PinS, 0);
        ToggleWait(PinO, 1000);
        ToggleWait(PinS, 0);
        ToggleWait(PinO, 1000);

        // Press to change selected game
        for (int i = 0; i < CountD * 2; i++)
            ToggleWait(PinD, 100);

        for (int i = 0; i < CountA * 2; i++)
            ToggleWait(PinA, 100);

        CountA = CountD = 0;

        // Press Start
        for (int i = 0; i < 2; i++)
            ToggleWait(PinS, 100);

        // Clean up all pins
        for (int i = 0; i < 4; i++)
            digitalWrite(i, 1);
    }
}

#elif defined(ARDUINO_M5STACK_Core2)

#include <M5Core2.h>
#include "images.h"
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <Free_Fonts.h>
#define KEY_W (65)
#define KEY_H (65)
#define ROWS (2)
#define COLS (5)

WiFiClient client;
HTTPClient http;

Gesture swipeRight("", 80, DIR_RIGHT, 30, true), swipeLeft("", 80, DIR_LEFT, 30, true);
Button *SpeedDialButtons[4], *NumPadButtons[COLS * ROWS];

int GameNumber, ScreenNumber = 1, FavGame[] = {26, 206, 41, 243};
char keymap[ROWS][COLS] = {{'0', '1', '2', '3', '4'}, {'5', '6', '7', '8', '9'}};

void NumPad(void)
{
    GameNumber = 0;
    M5.Lcd.setTextSize(1);
    int i = 0, x, y;
    // Create buttons from array
    for (int r = 0; r < ROWS; r++)
        for (int c = 0; c < COLS; c++)
        {
            x = (2 + (c * KEY_W));
            y = (110 + (r * KEY_H));
            String key = String(keymap[r][c]);
            M5.Lcd.drawString(key, x + 15, y + 15);
            M5.Lcd.drawRoundRect(x, y, KEY_W - 10, KEY_H - 10, 20, DARKGREY);
            NumPadButtons[i++] = new Button(x, y + 5, KEY_W, KEY_H, false, key.c_str());
        }
    M5.Lcd.setTextSize(3);
    ScreenNumber = 0;
}

void SpeedDial(void)
{
    M5.lcd.drawJpg(Image1, sizeof(Image1), 0, 0);
    M5.lcd.drawJpg(Image2, sizeof(Image2), 160, 0);
    M5.lcd.drawJpg(Image3, sizeof(Image3), 0, 120);
    M5.lcd.drawJpg(Image4, sizeof(Image4), 160, 120);
    // Create buttons from array
    for (int i = 0; i < 4; i++)
        SpeedDialButtons[i] = new Button(((i & 1) * 160), ((i & 2) / 2 * 120), 160, 120, false, String(i).c_str());
    ScreenNumber = 1;
}

void GetGame(int A)
{
    int D = 0;
    char buffer[10];
    A--;
    if (A >= 40)
        D = A / 40;
    A %= 40;
    sprintf(buffer, "%d,%d", A, D);
    http.begin(client, "http://192.168.4.1");
    http.POST(buffer);
    http.end();
}

void ScreenEvent(Event &e)
{
    Button &b = *e.button;
    switch (e.type)
    {
    case E_TAP:
        if (ScreenNumber == 1)
            GetGame(FavGame[String(b.getName()).toInt()]);
        else if (ScreenNumber == 0)
            if (b.getName()[0] >= '0' && b.getName()[0] <= '9')
            {
                GameNumber = GameNumber * 10 + String(b.getName()).toInt();
                M5.Lcd.fillRect(0, 0, 320, 100, BLACK);
                M5.Lcd.drawString(String(GameNumber), 70, 10);
            }
            else
            {
                if (GameNumber > 0 && GameNumber < 621)
                    GetGame(GameNumber);
                GameNumber = 0;
                M5.Lcd.fillRect(0, 0, 320, 100, BLACK);
            }
        break;

    case E_GESTURE:
        if (e.gesture->direction == DIR_LEFT && ScreenNumber == 1)
        {
            M5.Lcd.clearDisplay();
            for (int i = 0; i < 4; i++)
                delete (SpeedDialButtons[i]);
            NumPad();
        }
        else if (e.gesture->direction == DIR_RIGHT && ScreenNumber == 0)
        {
            M5.Lcd.clearDisplay();
            for (int i = 0; i < COLS * ROWS; i++)
                delete (NumPadButtons[i]);
            SpeedDial();
        }
        break;
    }
}

void setup(void)
{
    WiFi.begin("ESP8266-Access-Point", "ESP8266-Access-Point");
    M5.begin(true, false, false, false);
    M5.Axp.SetLed(0);
    M5.Lcd.setFreeFont(FF3);
    M5.Lcd.setTextColor(BLUE);
    M5.Lcd.setTextSize(2);
    M5.Lcd.printf("\r\nLooking For\r\nNES...");
    while (WiFi.status() != WL_CONNECTED)
        if (millis() > 3000)
            ESP.restart();
    M5.Lcd.clearDisplay();
    SpeedDial();
    M5.Buttons.addHandler(ScreenEvent, E_TAP | E_GESTURE);
}

void loop(void)
{
    M5.update();
    if (WiFi.status() != WL_CONNECTED)
        ESP.restart();
}

#else
#error Unsupported board selection.
#endif
