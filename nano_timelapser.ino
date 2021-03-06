/*
 * OH2MP Timelapser for Arduino Nano
 *
 * See the README.md for information about the hardware.
*/

#include <Wire.h>
#include <Servo.h>
#include <LiquidCrystal.h>
#define EI_ARDUINO_INTERRUPTED_PIN
#include <EnableInterrupt.h>

/* Hitec HS-311 min and max – check your servo specs */
#define MIN_US 575
#define MAX_US 2460

/* Maximum duration for the timer */
#define MAX_DURATION 7200

/* The pins used with rotary encoder and the servo */
#define ROT0  A1
#define ROT1  A2
#define SW    A3
#define SERVO 13

/* Variables for rotary encoder and its switch interrupts. These must be volatile. */
volatile byte rotflag = 0;
volatile byte reading = 0;
volatile byte button = 0;
volatile unsigned long button_time = 0;
volatile unsigned long rot_time = 0;

LiquidCrystal lcd(7, 6, 5, 4, 3, 2);
Servo the_servo;

char txt[9];
short pos = 0;
short startpos = 0;
short endpos = 0;
short rotenc_dir = 0;
byte phase = 0;
byte runmode = 0;
int duration = 10;
int interval = 0;
unsigned long last_rotation = 0;
int start_time = 0;
int eta = 0;
int last_eta = 0;


/* ------------------------------------------------------------------------------- */
// This is a little bit "rude" way to reset but it just works and is an easy way
// to clear all variables, free memory etc.

void reset_nano() {
    asm volatile ("jmp 0");
}


/* ------------------------------------------------------------------------------- */
// This interrupt is called when the switch of the rotary encoder is pressed.
// THis has a 50ms debouncing.

void button_isr() {
    cli();
    if (millis() - button_time > 50) {
        button = 1;
        button_time = millis();
    }
    sei();
}

/* ------------------------------------------------------------------------------- */
// This interrupt is called when the rotary encoder is turned. Detect the direction.

void rotenc_isr() {
    cli();
    reading = PINC & 7;
    if (millis() - rot_time > 20) {
        if (reading == 2 && rotflag) rotenc_dir = -1;
        if (reading == 4 && rotflag) rotenc_dir = 1;
        rot_time = millis();
    }
    if (reading == 0) {
        rotflag = 1;
    } else {
        rotflag = 0;
    }
    sei();
}

/* ------------------------------------------------------------------------------- */
// When button is pressed, go to next phase an do the actions
// Phases:
//  0 = Start, nothing done yet
//  1 = Set the end angle for rotation
//  2 = Set the start angle for rotation
//  3 = Set the duration for rotation
//  4 = Waiting for start command
//  5 = Running
//  6 = Reset and start again

void button_pressed() {
    phase++;
    button = rotenc_dir = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    switch (phase) {
        case 1:
            lcd.print(F("End degr"));
            lcd.setCursor(0, 1);
            lcd.print(F("ees   0\xDF"));
            break;
        case 2:
            endpos = the_servo.readMicroseconds();
            lcd.print(F("Start de"));
            sprintf(txt, "gr. %3d\xDF", pos - 90);
            lcd.setCursor(0, 1);
            lcd.print(txt);
            break;
        case 3:
            startpos = the_servo.readMicroseconds();
            lcd.print(F("Duration"));
            lcd.setCursor(0, 1);
            lcd.print(F("    0:10"));
            if (abs(startpos - endpos) < 100) runmode = 3;
            break;
        case 4:
            lcd.setCursor(0, 0);
            lcd.print(F(" Press t"));
            lcd.setCursor(0, 1);
            lcd.print(F("o start "));
            break;
        case 5:
            interval = int((duration * 1000.0) / abs(endpos - startpos));
            pos = startpos;
            runmode = 1;
            lcd.clear();
            lcd.print(F("Running"));
            start_time = millis() / 1000;
            break;
        case 6:
            asm volatile ("jmp 0");
            break;
    }
}

/* ------------------------------------------------------------------------------- */
// When rotary encoder is rotated, do the things depending on the phase.
// The time set works like eg. in a microwave oven. The time step depends on the
// magnitude of the time set.

void rotenc_turned() {
    if (phase == 1 || phase == 2) {
        pos += rotenc_dir;
        if (pos > 175) {
            pos = 175;
        }
        if (pos < 5) {
            pos = 5;
        }
        the_servo.write(pos);
        lcd.setCursor(0, 1);
        if (phase == 1) sprintf(txt, "ees %3d\xDF", pos - 90);
        if (phase == 2) sprintf(txt, "gr  %3d\xDF", pos - 90);
        lcd.print(txt);
    }
    if (phase == 3) {
        uint16_t multiplier;
        char txt[9];
        if (duration >= 3600) multiplier = 300;
        if (duration >= 1200 && duration < 3600) multiplier = 60;
        if (duration >= 180 && duration < 1200) multiplier = 30;
        if (duration < 180) multiplier = 10;
        duration += rotenc_dir * multiplier;
        if (duration < 10) {
            duration = MAX_DURATION;
        }
        if (duration > MAX_DURATION) {
            duration = 10;
        }
        if (duration >= 3600) {
            sprintf(txt, "%2d:%02d:%02d", int(duration / 3600), int(duration % 3600 / 60), duration % 60);
        } else {
            sprintf(txt, "   %2d:%02d", duration / 60, duration % 60);
        }
        lcd.setCursor(0, 1);
        lcd.print(txt);
    }
    button = rotenc_dir = 0;
}

/* ------------------------------------------------------------------------------- */
// Setup, like in every sketch it is for initializing. The code of this function
// should be quite self explanatory.

void setup() {
    pinMode(ROT0, INPUT_PULLUP);
    pinMode(ROT1, INPUT_PULLUP);
    pinMode(SW, INPUT_PULLUP);
    pinMode(SERVO, OUTPUT);

    enableInterrupt(SW, button_isr, RISING);
    enableInterrupt(ROT0, rotenc_isr, FALLING);
    enableInterrupt(ROT1, rotenc_isr, FALLING);

    the_servo.attach(13, MIN_US, MAX_US);
    pos = the_servo.read();

    lcd.begin(8, 2);
    lcd.clear();
    lcd.display();
    lcd.setCursor(0, 0);
    lcd.print(F("OH2MP Ti"));
    lcd.setCursor(0, 1);
    lcd.print(F("melapser"));
}

/* ------------------------------------------------------------------------------- */
// The loop. There are several runmodes and do the things depending on runmode.
// Runmodes:
//  0 = still setting up the angles and time.
//  1 = Running. Show the remaining time on the LCD.
//  2 = Ready. Wait for switch press for to reset.
//  3 = Cancelled. This happens if you set start and end angles the same.

void loop() {
    switch (runmode) {
        case 1:
            if (millis() - last_rotation >= interval) {
                last_rotation = millis();
                if (endpos > startpos) {
                    pos++;
                    if (pos >= endpos) runmode = 2;
                }
                if (endpos < startpos) {
                    pos--;
                    if (pos <= endpos) runmode = 2;
                }
                the_servo.writeMicroseconds(pos);
            }
            eta = duration - (millis() / 1000 - start_time);
            if (eta != last_eta) {
                last_eta = eta;
                if (eta >= 3600) {
                    sprintf(txt, "%2d:%02d:%02d", int(eta / 3600), int(eta % 3600 / 60), eta % 60);
                } else {
                    sprintf(txt, "   %2d:%02d", eta / 60, eta % 60);
                }
                lcd.setCursor(0, 1);
                lcd.print(txt);
            }
            break;
        case 2:
            lcd.clear();
            lcd.print(F("Ready."));
            phase = 5;
            runmode = startpos = endpos = 0;
            break;
        case 3:
            lcd.clear();
            lcd.print(F("Cancelle"));
            lcd.setCursor(0, 1);
            lcd.print(F("d."));
            phase = 5;
            runmode = startpos = endpos = 0;
            break;
        default:
            if (button) button_pressed();
            if (rotenc_dir == 1 || rotenc_dir == -1) rotenc_turned();
    }
}

/* ------------------------------------------------------------------------------- */
