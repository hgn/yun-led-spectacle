#include <Bridge.h>
#include <YunServer.h>
#include <YunClient.h>
#include <FastLED.h>

#define NUM_LEDS 240
#define DATA_PIN 13

#define BRIGHTNESS  200
#define FRAMES_PER_SECOND 60

#define COLOR_ORDER GRB
#define CHIPSET     WS2812B

CRGB leds[NUM_LEDS];

#undef DEBUG

#ifdef DEBUG
#define DBG_PRINT(x, ...) Console.print(x, ##__VA_ARGS__)
#define DBG_PRINTLN(x, ...) Console.println(x, ##__VA_ARGS__)
#else
#define DBG_PRINT(x, ...)
#define DBG_PRINTLN(x, ...)
#endif

enum {
    TYPE_UNKNOWN = 0,
    TYPE_MODE = 1,
    TYPE_SIGNAL = 2
};

enum {
    MODE_DISCO = 1,
    MODE_AMBIENT,
    MODE_AMBIENT_DYNAMIC,
    MODE_EXP,
};

enum {
    SIGNAL_ALARM = 1,
    SIGNAL_GREEN_PULSE,
};

#define DEFAULT_MODE MODE_AMBIENT

YunServer server;

void setup_debug()
{
#ifdef DEBUG
    Console.begin();

    while (!Console) {
        ; // wait for Console port to connect
    }
#endif
}

void setup()
{
    Bridge.begin();
    setup_debug();

    server.listenOnLocalhost();
    server.begin();

    FastLED.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>
        (leds, NUM_LEDS).setCorrection(TypicalLEDStrip);
}



int process_client(YunClient client, int *what)
{
    int type = 0;
    String command = client.readStringUntil('/');

    if (command == "signal") {
        type = signalCommand(client, what);
    }
    if (command == "mode") {
        type = modeCommand(client, what);
    }

    if (type == TYPE_UNKNOWN)
        type = TYPE_MODE;

    return type;
}

int signalCommand(YunClient client, int *what)
{
    String mode = client.readStringUntil('\r');
    if (mode == "alarm") {
        *what = SIGNAL_ALARM;
    } else if (mode == "green-pulse") {
        *what = SIGNAL_GREEN_PULSE;
    } else {
        // invalid, no default signal
        return TYPE_UNKNOWN;
    }

    return TYPE_SIGNAL;
}

int modeCommand(YunClient client, int *what)
{
    String mode = client.readStringUntil('\r');
    if (mode == "disco") {
        *what = MODE_DISCO;
    } else if (mode == "ambient") {
        *what = MODE_AMBIENT;
    } else if (mode == "ambient-dynamic") {
        *what = MODE_AMBIENT_DYNAMIC;
    } else if (mode == "exp") {
        *what = MODE_EXP;
    }

    return TYPE_MODE;
}

void mode_disco(void)
{
    static uint8_t hue = 0;
    FastLED.showColor(CHSV(hue++, 255, 255));
    delay(50);
    static uint8_t i = 0;
    if (i % 4 == 0) {
        for (int i = 0; i < NUM_LEDS; i++) {
            leds[i] = CRGB::White;
        }
        FastLED.show();
        delay(50);
    }
    i++;
}

void signal_alarm(void)
{
    int loop_count = 5;

    while (loop_count-- > 0) {
        for (int i = 0; i < NUM_LEDS; i++)
            leds[i] = CRGB::Red;

        FastLED.show();
        delay(100);
        for (int i = 0; i < NUM_LEDS; i++)
            leds[i] = CRGB::White;

        FastLED.show();
        delay(100);
    }
}

void signal_green_pulse(void)
{
    int loop_count = 3, i, j, k;

    while (loop_count-- > 0) {

        for (i = 0; i < NUM_LEDS; i++)
            leds[i] = CRGB::Green;
            FastLED.show(); delay(50);

        for (j = 0; j < 16; j++) {
          for (i = 0; i < NUM_LEDS; i++) {
            leds[i].fadeLightBy(16);
          }
          FastLED.show(); delay(50);
        }
    }
}

void mode_ambient(void)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Orange;
    }
    FastLED.show();
    delay(500);
}

void mode_ambient_dynamic(void)
{
    int wait = 150;
    int i, c, color_start = 32, color_end = 40;

    for (c = color_start; c < color_end; c++) {
        for(i = 0; i < NUM_LEDS; i++) {
            leds[i].setHSV(c, 255, 255);
        }
        FastLED.show();
        delay(wait);
    }

    for (c = color_end; c > color_start; c--) {
        for(i = 0; i < NUM_LEDS; i++) {
            leds[i].setHSV(c, 255, 255);
        }
        FastLED.show();
        delay(wait);
    }
}

void mode_exp(void)
{
    static uint8_t hue = 0;

    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CHSV(hue++, 255, 255);
    }
    FastLED.show();
    delay(50);
}

void handle_signal(int signal_no)
{
    switch (signal_no) {
        case SIGNAL_ALARM:
            Console.println("signal alarm");
            signal_alarm();
            break;
        case SIGNAL_GREEN_PULSE:
            Console.println("signal green-pulse");
            signal_green_pulse();
            break;
    };
}

static int mode = DEFAULT_MODE;

void loop()
{
    int type, new_mode;
    bool new_data = false;

    YunClient client = server.accept();
    if (client) {
        type = process_client(client, &new_mode);
        new_data = true;
        client.stop();
    }

    if (new_data) {
        switch (type) {
            case TYPE_SIGNAL:
                handle_signal(new_mode);
                break;
            case TYPE_MODE:
                mode = new_mode;
                break;
        };
        new_data = false;
    }

    switch (mode) {
        case MODE_DISCO:
            Console.println("mode disco");
            mode_disco();
            break;
        case MODE_AMBIENT:
            Console.println("mode ambient");
            mode_ambient();
            break;
        case MODE_AMBIENT_DYNAMIC:
            Console.println("mode ambient-dynamic");
            mode_ambient_dynamic();
            break;
        case MODE_EXP:
            Console.println("mode exp");
            mode_exp();
            break;
    }
}
