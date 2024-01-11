#pragma once

#include "wled.h"

#include "../../lib/ESP32-A2DP/src/BluetoothA2DPSink.h"

// This is an empty v2 usermod template. Please see the file usermod_v2_example.h in the EXAMPLE_v2 usermod folder for documentation on the functions you can use!

class A2dp : public Usermod
{
private:
  bool internalDac = false;
  unsigned long testULong = 42424242;
  float testFloat = 42.42;
  String btName = "Forty-Two";

  // These config variables have defaults set inside readFromConfig()
  int testInt;
  long testLong;
  int8_t i2sPins[3] = {26, 25, 22};

  bool enabled;
  static const char _a2dp[];
  static const char _name[];

  const i2s_config_t internalDacConfig = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
      .sample_rate = 44100,                         // corrected by info from bluetooth
      .bits_per_sample = (i2s_bits_per_sample_t)16, // the DAC module will only take the 8bits from MSB
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_STAND_MSB,
      .intr_alloc_flags = 0, // default interrupt priority
      .dma_buf_count = 8,
      .dma_buf_len = 64,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = false,
      //.mclk_multiple = 0,
      //.bits_per_chan = 0
  };

  i2s_pin_config_t externalDacPins;

public:
  A2dp(const char *name, bool enabled) : Usermod(name, enabled) {} // WLEDMM

  // non WLED related methods, may be used for data exchange between usermods (non-inline methods should be defined out of class)

  /**
   * Enable/Disable the usermod
   */
  inline void enable(bool enable) { enabled = enable; }

  /**
   * Get usermod enabled/disabled state
   */
  inline bool isEnabled() { return enabled; }

  // in such case add the following to another usermod:
  //  in private vars:
  //   #ifdef USERMOD_EXAMPLE
  //   MyExampleUsermod* UM;
  //   #endif
  //  in setup()
  //   #ifdef USERMOD_EXAMPLE
  //   UM = (MyExampleUsermod*) usermods.lookup(USERMOD_ID_EXAMPLE);
  //   #endif
  //  somewhere in loop() or other member method
  //   #ifdef USERMOD_EXAMPLE
  //   if (UM != nullptr) isExampleEnabled = UM->isEnabled();
  //   if (!isExampleEnabled) UM->enable(true);
  //   #endif

  // methods called by WLED (can be inlined as they are called only once but if you call them explicitly define them out of class)

  /*
   * setup() is called once at boot. WiFi is not yet connected at this point.
   * readFromConfig() is called prior to setup()
   * You can use it to initialize variables, sensors or similar.
   */
  void setup()
  {
    externalDacPins = {
        .bck_io_num = i2sPins[0],
        .ws_io_num = i2sPins[1],
        .data_out_num = i2sPins[2],
        .data_in_num = I2S_PIN_NO_CHANGE};

    // do your set-up here
    // Serial.println("Hello from my usermod!");
    initDone = true;
  }

  /*
   * connected() is called every time the WiFi is (re)connected
   * Use it to initialize network interfaces
   */
  void connected()
  {
    Serial.println("Connected to WiFi!");
  }

  /*
   * loop() is called continuously. Here you can check for events, read sensors, etc.
   *
   * Tips:
   * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
   *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
   *
   * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
   *    Instead, use a timer check as shown here.
   */
  void loop()
  {
    // if usermod is disabled or called during strip updating just exit
    // NOTE: on very long strips strip.isUpdating() may always return true so update accordingly
    if (!enabled || strip.isUpdating())
      return;

    // do your magic here
    if (millis() - lastTime > 1000)
    {
      Serial.println("I'm alive!");
      lastTime = millis();
    }
  }

  /*
   * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
   * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
   * Below it is shown how this could be used for e.g. a light sensor
   */
  void addToJsonInfo(JsonObject &root)
  {
    JsonObject user = root["u"];
    if (user.isNull())
      user = root.createNestedObject("u");

    JsonArray infoArr = user.createNestedArray(FPSTR(_name));

    String uiDomString = F("<button class=\"btn btn-xs\" onclick=\"requestJson({");
    uiDomString += FPSTR(_name);
    uiDomString += F(":{");
    uiDomString += FPSTR("enabled");
    uiDomString += enabled ? F(":false}});\">") : F(":true}});\">");
    uiDomString += F("<i class=\"icons");
    uiDomString += enabled ? F(" on") : F(" off");
    uiDomString += F("\">&#xe08f;</i>");
    uiDomString += F("</button>");
    infoArr.add(uiDomString);
  }
  /*
   * addToConfig() can be used to add custom persistent settings to the cfg.json file in the "um" (usermod) object.
   * It will be called by WLED when settings are actually saved (for example, LED settings are saved)
   * If you want to force saving the current state, use serializeConfig() in your loop().
   *
   * CAUTION: serializeConfig() will initiate a filesystem write operation.
   * It might cause the LEDs to stutter and will cause flash wear if called too often.
   * Use it sparingly and always in the loop, never in network callbacks!
   *
   * addToConfig() will make your settings editable through the Usermod Settings page automatically.
   *
   * Usermod Settings Overview:
   * - Numeric values are treated as floats in the browser.
   *   - If the numeric value entered into the browser contains a decimal point, it will be parsed as a C float
   *     before being returned to the Usermod.  The float data type has only 6-7 decimal digits of precision, and
   *     doubles are not supported, numbers will be rounded to the nearest float value when being parsed.
   *     The range accepted by the input field is +/- 1.175494351e-38 to +/- 3.402823466e+38.
   *   - If the numeric value entered into the browser doesn't contain a decimal point, it will be parsed as a
   *     C int32_t (range: -2147483648 to 2147483647) before being returned to the usermod.
   *     Overflows or underflows are truncated to the max/min value for an int32_t, and again truncated to the type
   *     used in the Usermod when reading the value from ArduinoJson.
   * - Pin values can be treated differently from an integer value by using the key name "pin"
   *   - "pin" can contain a single or array of integer values
   *   - On the Usermod Settings page there is simple checking for pin conflicts and warnings for special pins
   *     - Red color indicates a conflict.  Yellow color indicates a pin with a warning (e.g. an input-only pin)
   *   - Tip: use int8_t to store the pin value in the Usermod, so a -1 value (pin not set) can be used
   *
   * See usermod_v2_auto_save.h for an example that saves Flash space by reusing ArduinoJson key name strings
   *
   * If you need a dedicated settings page with custom layout for your Usermod, that takes a lot more work.
   * You will have to add the setting to the HTML, xml.cpp and set.cpp manually.
   * See the WLED Soundreactive fork (code and wiki) for reference.  https://github.com/atuline/WLED
   *
   * I highly recommend checking out the basics of ArduinoJson serialization and deserialization in order to use custom settings!
   */
  void addToConfig(JsonObject &root)
  {
    Usermod::addToConfig(root);
    JsonObject top = root[FPSTR(_a2dp)]; // WLEDMM

    // save these vars persistently whenever settings are saved
    // top["great"] = userVar0;
    top["internalDac"] = internalDac;
    // top["testInt"] = testInt;
    // top["testLong"] = testLong;
    // top["testULong"] = testULong;
    // top["testFloat"] = testFloat;
    top["btName"] = btName;

    // ESP.restart();
    JsonArray pinArray = top.createNestedArray("pin");
    pinArray.add(i2sPins[0]);
    pinArray.add(i2sPins[1]);
    pinArray.add(i2sPins[2]);
  }

  /*
   * readFromConfig() can be used to read back the custom settings you added with addToConfig().
   * This is called by WLED when settings are loaded (currently this only happens immediately after boot, or after saving on the Usermod Settings page)
   *
   * readFromConfig() is called BEFORE setup(). This means you can use your persistent values in setup() (e.g. pin assignments, buffer sizes),
   * but also that if you want to write persistent values to a dynamic buffer, you'd need to allocate it here instead of in setup.
   * If you don't know what that is, don't fret. It most likely doesn't affect your use case :)
   *
   * Return true in case the config values returned from Usermod Settings were complete, or false if you'd like WLED to save your defaults to disk (so any missing values are editable in Usermod Settings)
   *
   * getJsonValue() returns false if the value is missing, or copies the value into the variable provided and returns true if the value is present
   * The configComplete variable is true only if the "exampleUsermod" object and all values are present.  If any values are missing, WLED will know to call addToConfig() to save them
   *
   * This function is guaranteed to be called on boot, but could also be called every time settings are updated
   */
  bool readFromConfig(JsonObject &root)
  {
    // default settings values could be set here (or below using the 3-argument getJsonValue()) instead of in the class definition or constructor
    // setting them inside readFromConfig() is slightly more robust, handling the rare but plausible use case of single value being missing after boot (e.g. if the cfg.json was manually edited and a value was removed)

    bool configComplete = Usermod::readFromConfig(root);
    JsonObject top = root[FPSTR(_a2dp)]; // WLEDMM

    configComplete &= getJsonValue(top["btName"], btName);
    // configComplete &= getJsonValue(top["great"], userVar0);
    configComplete &= getJsonValue(top["internalDac"], internalDac);
    // configComplete &= getJsonValue(top["testULong"], testULong);
    // configComplete &= getJsonValue(top["testFloat"], testFloat);

    // A 3-argument getJsonValue() assigns the 3rd argument as a default value if the Json value is missing
    // configComplete &= getJsonValue(top["testInt"], testInt, 42);
    // configComplete &= getJsonValue(top["testLong"], testLong, -42424242);

    // "pin" fields have special handling in settings page (or some_pin as well)
    configComplete &= getJsonValue(top["pin"][0], i2sPins[0], -1);
    configComplete &= getJsonValue(top["pin"][1], i2sPins[1], -1);
    configComplete &= getJsonValue(top["pin"][2], i2sPins[2], -1);

    return configComplete;
  }

  /*
   * appendConfigData() is called when user enters usermod settings page
   * it may add additional metadata for certain entry fields (adding drop down is possible)
   * be careful not to add too much as oappend() buffer is limited to 3k
   */
  void appendConfigData()
  {
    // addInfo('a2dp:great',1,'<i>(this is a great config value)</i>');
    // addInfo('a2dp:btName',1,'enter any string you want');
    // dd=addDropdown('a2dp','testInt');
    // addOption(dd,'Nothing',0);
    // addOption(dd,'Everything',42);

    oappend(SET_F("addInfo('"));
    oappend(String(FPSTR(_a2dp)).c_str());
    oappend(SET_F(":pin[]"));
    oappend(SET_F("',1,'bck');"));


    oappend(SET_F("addInfo('"));
    oappend(String(FPSTR(_a2dp)).c_str());
    oappend(SET_F(":pin[]"));
    oappend(SET_F("',1,'ws');"));

    oappend(SET_F("addInfo('"));
    oappend(String(FPSTR(_a2dp)).c_str());
    oappend(SET_F(":pin[]"));
    oappend(SET_F("',1,'data');"));



    oappend(SET_F("function insertAfter(a,b){b.parentNode.insertBefore(a,b.nextSibling)}var asd=document.createElement('i');asd.style.color='orange';asd.innerText='(change requires reboot!)';insertAfter(asd,document.getElementsByTagName('h3')[0]);asd=document.createElement('hr');asd.className='sml';insertAfter(asd,document.getElementsByTagName('h3')[0]);"));

    oappend(SET_F("addInfo('"));
    oappend(String(FPSTR(_a2dp)).c_str());
    oappend(SET_F(":internalDac"));
    oappend(SET_F("',1,'Use internal Digital-to-analog converter');"));

    oappend(SET_F("addInfo('"));
    oappend(String(FPSTR(_a2dp)).c_str());
    oappend(SET_F(":btName"));
    oappend(SET_F("',1,'Name to appear as');"));
    // oappend(SET_F("addInfo('"));
    // oappend(String(FPSTR(_a2dp)).c_str());
    // oappend(SET_F(":internalDac"));
    // oappend(SET_F("',1,'<i>(this is a great config value)</i>');"));

    // oappend(SET_F("dd=addDropdown('"));
    // oappend(String(FPSTR(_a2dp)).c_str());
    // oappend(SET_F("','testInt');"));
    // oappend(SET_F("addOption(dd,'Nothing',0);"));
    // oappend(SET_F("addOption(dd,'Everything',42);"));
  }

  /*
   * handleOverlayDraw() is called just before every show() (LED strip update frame) after effects have set the colors.
   * Use this to blank out some LEDs or set them to a different color regardless of the set effect mode.
   * Commonly used for custom clocks (Cronixie, 7 segment)
   */
  void handleOverlayDraw()
  {
    // strip.setPixelColor(0, RGBW32(0,0,0,0)) // set the first pixel to black
  }

  /**
   * handleButton() can be used to override default button behaviour. Returning true
   * will prevent button working in a default way.
   * Replicating button.cpp
   */
  bool handleButton(uint8_t b)
  {
    yield();
    // ignore certain button types as they may have other consequences
    if (!enabled || buttonType[b] == BTN_TYPE_NONE || buttonType[b] == BTN_TYPE_RESERVED || buttonType[b] == BTN_TYPE_PIR_SENSOR || buttonType[b] == BTN_TYPE_ANALOG || buttonType[b] == BTN_TYPE_ANALOG_INVERTED)
    {
      return false;
    }

    bool handled = false;
    // do your button handling here
    return handled;
  }

  /**
   * onStateChanged() is used to detect WLED state change
   * @mode parameter is CALL_MODE_... parameter used for notifications
   */
  void onStateChange(uint8_t mode)
  {
    // do something if WLED state changed (color, brightness, effect, preset, etc)
  }

  uint16_t getId()
  {
    return USERMOD_ID_A2DP;
  }
};

const char A2dp::_a2dp[] PROGMEM = "a2dp";
const char A2dp::_name[] PROGMEM = "a2dp";
