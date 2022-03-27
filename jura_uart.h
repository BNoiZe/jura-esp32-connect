#include "esphome.h"

class JuraUartReadLine : public Component, public UARTDevice, public TextSensor {
    public:
        JuraUartReadLine(UARTComponent *parent) : UARTDevice(parent) {}

    void setup() override {
        // nothing to do here
    }

    void loop() override {
        int s = 0;
        int w = 0;
        char inbyte;
        String inbytes;

        while(!inbytes.endsWith("\r\n") && w <= 500) {
            if (available()) {
                byte rawbyte = read();
                bitWrite(inbyte, s+ 0, bitRead(rawbyte, 2));
                bitWrite(inbyte, s+ 1, bitRead(rawbyte, 5));
                if ((s += 2) > 8) {
                    s = 0;
                    inbytes += inbyte;
                }
            } else {
                delay(10);
            }

            w++;
        }

        ESP_LOGD("uart", "%s", inbytes.c_str());
    }
};