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

enum {
    TYPE_UNKNOWN = 0,
    TYPE_MODE = 1,
    TYPE_SIGNAL = 2
};

enum {
    MODE_DISCO = 1,
    MODE_AMBIENT,
    MODE_EXP,
};

enum {
    SIGNAL_ALARM = 1,
};

#define DEFAULT_MODE MODE_AMBIENT

YunServer server;

void setup_debug()
{
#if 0
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

void mode_ambient(void)
{
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = CRGB::Orange;
    }
    FastLED.show();
    delay(500);
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
        case MODE_EXP:
            Console.println("mode exp");
            mode_exp();
            break;
    }

}
