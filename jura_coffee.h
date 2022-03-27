#include "esphome.h"

class JuraCoffee : public PollingComponent, public UARTDevice {
  public:
    Sensor *total_sensor = new Sensor();
    Sensor *cappucino_sensor = new Sensor();
    Sensor *latte_macchiato_sensor = new Sensor();
    Sensor *flat_white_sensor = new Sensor();
    Sensor *milk_foam_sensor = new Sensor();
    Sensor *espresso_macchiato_sensor = new Sensor();
    Sensor *cortado_sensor = new Sensor();
    Sensor *ristretto_sensor = new Sensor();
    Sensor *espresso_sensor = new Sensor();
    Sensor *coffee_sensor = new Sensor();
    Sensor *caffe_barista_sensor = new Sensor();
    Sensor *lungo_barista_sensor = new Sensor();
    Sensor *espresso_doppio_sensor = new Sensor();
    Sensor *hot_water_sensor = new Sensor();
    Sensor *grounded_coffee_sensor = new Sensor();
    TextSensor *tray_sensor = new TextSensor();
    TextSensor *tank_sensor = new TextSensor();

    JuraCoffee(UARTComponent *parent) : UARTDevice(parent) {}

    long num_single;
    std::string tray_status, tank_status;

    // Jura communication function taken in entirety from cmd2jura.ino, found at https://github.com/hn/jura-coffee-machine
    String cmd2jura(String outbytes) {
      String inbytes;
      int w = 0;

      while (available()) {
        read();
      }

      outbytes += "\r\n";
      for (int i = 0; i < outbytes.length(); i++) {
        for (int s = 0; s < 8; s += 2) {
          char rawbyte = 255;
          bitWrite(rawbyte, 2, bitRead(outbytes.charAt(i), s + 0));
          bitWrite(rawbyte, 5, bitRead(outbytes.charAt(i), s + 1));
          write(rawbyte);
        }
        delay(8);
      }

      int s = 0;
      char inbyte;
      while (!inbytes.endsWith("\r\n")) {
        if (available()) {
          byte rawbyte = read();
          bitWrite(inbyte, s + 0, bitRead(rawbyte, 2));
          bitWrite(inbyte, s + 1, bitRead(rawbyte, 5));
          if ((s += 2) >= 8) {
            s = 0;
            inbytes += inbyte;
          }
        } else {
          delay(10);
        }
        if (w++ > 500) {
          return "";
        }
      }

      return inbytes.substring(0, inbytes.length() - 2);
    }

    void setup() override {
      this->set_update_interval(5000); // 600000 = 10 minutes // Now 60 seconds
    }

    void loop() override {
    }

    void update() override {
      String result, hexString, substring;
      byte hex_to_byte;
      int trayBit, tankBit;

      // Fetch our line of EEPROM
      result = cmd2jura("RT:0000");
      //ESP_LOGD("main", "RT:0000: %s", result.c_str());
      for (size_t i = 3; i < result.length() - 4; i = i + 4)
      {
        substring = result.substring(i,i+4);
        num_single = strtol(substring.c_str(),NULL,16);
        //ESP_LOGD("main", "EEPROM %i,%i:  %ld", i, i + 4, num_single);
      }

      // Fetch our line of EEPROM
      result = cmd2jura("RT:0010");
      //ESP_LOGD("main", "RT:0010: %s", result.c_str());
      for (size_t i = 3; i < result.length() - 4; i = i + 4)
      {
        substring = result.substring(i,i+4);
        num_single = strtol(substring.c_str(),NULL,16);
        //ESP_LOGD("main", "EEPROM %i,%i:  %ld", i, i + 4, num_single);
      }

      // // Fetch single value of EEPROM
      // result = cmd2jura("RE:0000");
      // ESP_LOGD("main", "RE:0000: %s", result.c_str());

      /* 
      *** RT:0020 ***
      * 3,7    8
      * 7,11   5
      * 11,15  1
      * 15,19  1
      * 19,23  11
      * 23-63  Nothing useful
      */

      /*** EEPROM block RT:0000 ***
       * 3,7    Espresso
       * 7,11   ? (2x Espresso)
       * 11,15  Coffee
       * 15,19  ? (2x Coffee)
       * 19,23  Cappucino
       * 23,27  Latte macchiato
       * 27,31  ?
       * 31,35  Total
       * 35,39  ? (Cleanings)
       * 39,43  ?
       * 43,47  66 = ?
       * 47,51  ?
       * 51,55  ?
       * 55,59  ?
       * 59,63  ? (Cortado)
       */
      result = cmd2jura("RT:0000");

      // Espresso made
      substring = result.substring(3, 7);
      espresso_sensor->publish_state(strtol(substring.c_str(), NULL, 16));

      // Coffee made
      substring = result.substring(11, 15);
      coffee_sensor->publish_state(strtol(substring.c_str(), NULL, 16));

      // Cappucino made
      substring = result.substring(19, 23);
      cappucino_sensor->publish_state(strtol(substring.c_str(), NULL, 16));

      // Latte macchiato made
      substring = result.substring(23, 27);
      latte_macchiato_sensor->publish_state(strtol(substring.c_str(), NULL, 16));

      // Total made
      substring = result.substring(31, 35);
      total_sensor->publish_state(strtol(substring.c_str(), NULL, 16));

      /*** EEPROM block RT:0010 ***
       * 3,7    47 = ?
       * 7,11   20 = ?
       * 11,15  ?
       * 15,19  Milk foam
       * 19,23  Hot water
       * 23,27  26 = ?
       * 27,31  ?
       * 31,35  ?
       * 35,39  ?
       * 39,43  ?
       * 43,47  ?
       * 47,51  10 = ?
       * 51,55  8 = ?
       * 55,59  ?
       * 59,63  ?
      */
      result = cmd2jura("RT:0010");

      // Milk foam made
      substring = result.substring(15, 19);
      milk_foam_sensor->publish_state(strtol(substring.c_str(), NULL, 16));

      // Hot water made
      substring = result.substring(19, 23);
      hot_water_sensor->publish_state(strtol(substring.c_str(), NULL, 16));

      // Tray & water tank status
      // Much gratitude to https://www.instructables.com/id/IoT-Enabled-Coffee-Machine/ for figuring out how these bits are stored
      result = cmd2jura("IC:");
      //ESP_LOGD("main", "Tray / Tank: %s", result.c_str());
      hexString = result.substring(3,5);
      hex_to_byte = strtol(hexString.c_str(),NULL,16);
      trayBit = bitRead(hex_to_byte, 4);
      if (trayBit == 0) { tray_status = "Missing"; } else { tray_status = "Present"; }
      tray_sensor->publish_state(tray_status);

      tankBit = bitRead(hex_to_byte, 13);
      if (tankBit == 1) { tank_status = "Fill Tank"; } else { tank_status = "OK"; }
      tank_sensor->publish_state(tank_status);

      // Get machine type
      String machine;
      machine = cmd2jura("TY:");
      //ESP_LOGD("main", "Machine: %s", machine.c_str());

      // ESP_LOGD("main", "Raw IC result: %s", result.c_str());
      // ESP_LOGD("main", "Substringed: %s", hexString.c_str());
      // ESP_LOGD("main", "Converted_To_Long: %li", hex_to_byte);
      // ESP_LOGD("main", "As Bits: %d%d%d%d%d%d%d%d", read_bit7,read_bit6,read_bit5,read_bit4,read_bit3,read_bit2,read_bit1,read_bit0);
    }
};
