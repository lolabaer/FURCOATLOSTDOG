#pragma once

#define ENABLE_DFPLAYER_HW
#define DEBUG_LOGGING true

#define RX_PIN 17
#define TX_PIN 16


#include <time.h>
#include <HardwareSerial.h>
#include "pin_manager.h"
#include "DFPlayerMini_Fast.h"
#include "wled.h"

// Some new MCUs (-S2, -C3) don't have HardwareSerial(2)
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(4, 2, 0)
  #if SOC_UART_NUM < 3
  #error DFplayer usage is not possible on your MCU, as it does not have HardwareSerial(2)
  #endif
#endif

static HardwareSerial mySerial(1);
//#define mySerial Serial1

/*
 * Usermods allow you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * 
 * This is an example for a v2 usermod.
 * v2 usermods are class inheritance based and can (but don't have to) implement more functions, each of them is shown in this example.
 * Multiple v2 usermods can be added to one compilation easily.
 * 
 * Creating a usermod:
 * This file serves as an example. If you want to create a usermod, it is recommended to use usermod_v2_empty.h from the usermods folder as a template.
 * Please remember to rename the class and file to a descriptive name.
 * You may also use multiple .h and .cpp files.
 * 
 * Using a usermod:
 * 1. Copy the usermod into the sketch folder (same folder as wled00.ino)
 * 2. Register the usermod by adding #include "usermod_filename.h" in the top and registerUsermod(new MyUsermodClass()) in the bottom of usermods_list.cpp
 */

#ifdef ENABLE_DFPLAYER_HW
  DFPlayerMini_Fast dfmp3;
#endif

//class name. Use something descriptive and leave the ": public Usermod" part :)
class MyDfPlayerMini : public Usermod {

  private:

    //Private class members. You can declare variables and functions only accessible to your usermod here
    unsigned long mLastTime = 0;
    
    // set your config variables to their boot default value (this can also be done in readFromConfig() or a constructor if you prefer)
    uint8_t mNrFilesInFolder;

    bool initDone = false;

    // These config variables have defaults set inside readFromConfig()
    uint8_t mDfPlayerVolume;
    bool mDfPlayerRandom;
    bool mDfPlayerLoopFolder;
    uint8_t mDfPlayerFolder;
    uint8_t mDfPlayerPreviousFolder;
    uint8_t mDfPlayerTrackNr;
    bool mDfPlayerStart;
    uint8_t mUartPins[2];

    static const char _dfplayer[];
    static const char _folder[];
    static const char _random[];
    static const char _loopFolder[];
    static const char _volume[]; 
    static const char _tracknr[];
    static const char _start[];


    // local function
    void SetVolume()
    {
      if (mDfPlayerVolume >= 0 && mDfPlayerVolume <= 30) 
      {
        #ifdef ENABLE_DFPLAYER_HW
          dfmp3.volume(mDfPlayerVolume);
        #endif

        #ifdef DEBUG_LOGGING
          int16_t vol = -1;
          unsigned int i = 0;
          while( (i < 3) && (vol == -1))
          {
            #ifdef ENABLE_DFPLAYER_HW
              vol = dfmp3.currentVolume();
            #else
              vol =1; // simulated value
            #endif
            ++i;
            Serial.print(F("Vol: ") );
            Serial.println(vol);
          }
        #endif 
      }
    }

    int GetNewTrackNr()
    {
      int trackNr = mDfPlayerTrackNr;
      if (mNrFilesInFolder > 1)
      {
        
        if (mDfPlayerRandom)
        {
          /* initialize random seed: */
          srand (time(NULL));
          trackNr = (rand() % mNrFilesInFolder) + 1;
          while (mDfPlayerTrackNr == trackNr)
          {
            /* generate random number between 1 and the number of file sin the dir */
            trackNr = (rand() % mNrFilesInFolder) + 1;
          }
        } else
        {
          (trackNr < 255) ? ++trackNr : 1;
          if (trackNr > mNrFilesInFolder)
          {
            trackNr = 1;
          };
        }
      }
 
      return trackNr;
    }

    void StartStopTrack()
    {
      if (mDfPlayerStart) // bool holding start or stop command
      {
        #ifdef ENABLE_DFPLAYER_HW
          dfmp3.playFolder(mDfPlayerFolder, mDfPlayerTrackNr); 
        #endif
      } else 
      {
        #ifdef ENABLE_DFPLAYER_HW
          dfmp3.stop();
        #endif
      }

      #ifdef DEBUG_LOGGING
        if (mDfPlayerStart)
        {
          #ifdef DEBUG_LOGGING
            Serial.print(F("Start folder: ") );
            Serial.print(mDfPlayerFolder);
            Serial.print(F(" ,track: ") );
            Serial.println(mDfPlayerTrackNr);
          #endif
        } else
        {
          Serial.println(F("Stop"));  
        }
      #endif
    }

    int GetNrFilesInFolder()
    {
      int nrFiles = -1;
      int i = 0;
      while ((i < 5) && (nrFiles == -1))
      {
        #ifdef ENABLE_DFPLAYER_HW
          nrFiles = dfmp3.numTracksInFolder( mDfPlayerFolder); 
        #else
          nrFiles = 10; // Some simulated value
        #endif
        ++i;
        delay(10);
        #ifdef DEBUG_LOGGING
          Serial.print(F("Files: "));
          Serial.println(nrFiles );
        #endif
      }
      // If valid number found, return it, otherwise '1' as default
      return (nrFiles != -1 ? nrFiles : 1);
    }

    void ResetMp3Player()
    {
      #ifdef ENABLE_DFPLAYER_HW
        dfmp3.reset();
      #endif
    }

  public:

    // methods called by WLED (can be inlined as they are called only once but if you call them explicitly define them out of class)

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * readFromConfig() is called prior to setup()
     * You can use it to initialize variables, sensors or similar.
     */
    void setup() override
    {
      initDone = true;
      mDfPlayerPreviousFolder = mDfPlayerFolder;

      #ifdef ENABLE_DFPLAYER_HW
        mySerial.begin (9600, SERIAL_8N1, mUartPins[1], mUartPins[0]);
        #ifdef DEBUG_LOGGING
          Serial.println(F("DfPlayer setup"));
        #endif

        if (!dfmp3.begin(mySerial, DEBUG_LOGGING /*debug*/, 100 /*timeout ms*/))      
        {
          #ifdef DEBUG_LOGGING
            Serial.println(F("DfPlayer failed"));
          #endif
        } else
        {
          #ifdef DEBUG_LOGGING
            Serial.println(F("DfPlayer online"));
          #endif
        }
      #endif

      SetVolume();
      mNrFilesInFolder = GetNrFilesInFolder();
    }


    /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
    void connected() override
    {
      #ifdef DEBUG_LOGGING
        Serial.println(F("Connected!"));
        Serial.print(F("IP: "));
        Serial.println(WiFi.localIP());
      #endif
    };


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
    void loop() override
    {
        #ifdef ENABLE_DFPLAYER_HW 
          static bool isPlaying = dfmp3.isPlaying(); 
        #else
          static bool isPlaying = false;
        #endif

        if (millis() - mLastTime > 3000) 
        {         
          if (mDfPlayerStart)
          {
            #ifdef ENABLE_DFPLAYER_HW 
              bool tmpIsPlaying = dfmp3.isPlaying();
            #else
              bool tmpIsPlaying = false;
            #endif

            #ifdef DEBUG_LOGGING
              Serial.println(tmpIsPlaying);
            #endif
            
            if (isPlaying != tmpIsPlaying)  // state chance detected
            {
              #ifdef DEBUG_LOGGING
                Serial.print(F("State: "));
                Serial.print(isPlaying);
                Serial.print(F("->"));
                Serial.println(tmpIsPlaying);
              #endif

              isPlaying = tmpIsPlaying;

              if (!tmpIsPlaying) // state change playing -> stopped
              {
                if (!mDfPlayerLoopFolder) 
                {
                  mDfPlayerStart = false; // stop playing
                }
                else
                {
                  // Loop over all tracks in folder only if loopfolder is requested
                  if (mDfPlayerLoopFolder)
                  {
                    // Play single file, random track if requested.
                    // Random takes precedence over selecting tracknr.
                    mDfPlayerTrackNr = GetNewTrackNr();
                    #ifdef DEBUG_LOGGING
                      Serial.println(F("Loop") );
                    #endif
                  }
                }
                StartStopTrack(); // Check if new track needs to be started
              }
            }
          }

          mLastTime = millis();
        }
    }


    /*
     * addToJsonInfo() can be used to add custom entries to the /json/info part of the JSON API.
     * Creating an "u" object allows you to add custom key/value pairs to the Info section of the WLED web UI.
     * Below it is shown how this could be used for e.g. a light sensor
     */
    void addToJsonInfo(JsonObject& root)
    {
    }


    /*
     * addToJsonState() can be used to add custom entries to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     */
    void addToJsonState(JsonObject& root) override
    {
      if (!initDone) 
      {
        #ifdef DEBUG_LOGGING
          Serial.println(F("Init not done"));
        #endif

        return;  // prevent crash on boot applyPreset()
      }

      JsonObject usermod = root[FPSTR(_dfplayer)];
      if (usermod.isNull()) usermod = root.createNestedObject(FPSTR(_dfplayer));
      usermod[FPSTR(_folder)] = mDfPlayerFolder; 
      usermod[FPSTR(_random)] = mDfPlayerRandom;
      usermod[FPSTR(_loopFolder)] = mDfPlayerLoopFolder;
      usermod[FPSTR(_volume)] = mDfPlayerVolume;
      usermod[FPSTR(_tracknr)] = mDfPlayerTrackNr;
      usermod[FPSTR(_start)] = mDfPlayerStart;
    }


    /*
     * readFromJsonState() can be used to receive data clients send to the /json/state part of the JSON API (state object).
     * Values in the state object may be modified by connected clients
     * 
     * Example JSON call to change dfplayer settings
     * curl-7.42.0-win64>curl -X POST "http://192.168.1.183/json/state" -d {"dfplayer":{"volume":12,"play":true,"loopfolder":false,"random":true,"tracknr":2,"folder":4}} -H "Content-Type: application/json"
     * 
     */
    void readFromJsonState(JsonObject& root) override
    {
      if (!initDone) 
      {
        #ifdef DEBUG_LOGGING
          Serial.println(F("Init not done"));
        #endif
        
        return;  // prevent crash on boot applyPreset()
      }

      JsonObject dfPlayerJson = root[FPSTR(_dfplayer)];
      if (!dfPlayerJson.isNull()) 
      {
        if (dfPlayerJson["reset"].is<bool>()) 
        {
          #ifdef DEBUG_LOGGING
            Serial.print(F("Reset: "));
            Serial.println(dfPlayerJson["reset"].as<bool>());
          #endif
          if (dfPlayerJson["reset"].as<bool>() == true)
          ResetMp3Player();
          return; // Do not process other commands
        }

        if (dfPlayerJson[FPSTR(_volume)].is<int>()) 
        {
          mDfPlayerVolume = dfPlayerJson[FPSTR(_volume)].as<int>();
          #ifdef DEBUG_LOGGING
            Serial.print(F("Set vol: "));
            Serial.println(mDfPlayerVolume);
          #endif
          SetVolume();
        }

        if (dfPlayerJson[FPSTR(_random)].is<bool>()) 
        {
          mDfPlayerRandom = dfPlayerJson[FPSTR(_random)].as<bool>();
          #ifdef DEBUG_LOGGING
            Serial.print(F("Random: "));
            Serial.println(mDfPlayerRandom);
          #endif
        } else
        {
          mDfPlayerRandom = false;
        }

        if (dfPlayerJson[FPSTR(_loopFolder)].is<bool>()) 
        {
          mDfPlayerLoopFolder = dfPlayerJson[FPSTR(_loopFolder)].as<bool>();
          #ifdef DEBUG_LOGGING
            Serial.print(F("LoopFolder: "));
            Serial.println(mDfPlayerLoopFolder);
          #endif
        } else
        {
          mDfPlayerLoopFolder = false;
        }

        if (dfPlayerJson[FPSTR(_folder)].is<int>()) 
        {
          mDfPlayerFolder = dfPlayerJson[FPSTR(_folder)].as<int>();
          int filesInFolder = GetNrFilesInFolder();
          if (filesInFolder != -1) mNrFilesInFolder = filesInFolder;
          #ifdef DEBUG_LOGGING
            Serial.print(F("Folder: "));
            Serial.print(mDfPlayerFolder);
            Serial.print(", Files: ");
            Serial.println(mNrFilesInFolder);
          #endif
        }

        if (dfPlayerJson[FPSTR(_tracknr)].is<int>()) 
        {
          // only assign TrackNr if not random is true
          mDfPlayerTrackNr = (mDfPlayerRandom ? GetNewTrackNr() : dfPlayerJson[FPSTR(_tracknr)].as<int>() );
          #ifdef DEBUG_LOGGING
            Serial.print(F("TrackNr: "));
            Serial.println(mDfPlayerTrackNr);
          #endif
        }

        if (dfPlayerJson["play"].is<bool>()) 
        {
          mDfPlayerStart = dfPlayerJson["play"].as<bool>();
          #ifdef DEBUG_LOGGING
            Serial.print(F("Play: "));
            Serial.println(mDfPlayerStart);
          #endif
          StartStopTrack();
        }
      } else
      {
        #ifdef DEBUG_LOGGING
          Serial.print(F("Not found "));
        #endif
      }
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
    void addToConfig(JsonObject& root) override
    {
      #ifdef ENABLE_DFPLAYER_HW
      #endif

      JsonObject top = root.createNestedObject(FPSTR(_dfplayer));
      top[FPSTR(_folder)] = mDfPlayerFolder; //save these vars persistently whenever settings are saved
      top[FPSTR(_random)] = mDfPlayerRandom;
      top[FPSTR(_loopFolder)] = mDfPlayerLoopFolder;
      top[FPSTR(_volume)] = mDfPlayerVolume;
      top[FPSTR(_tracknr)] = mDfPlayerTrackNr;
      top[FPSTR(_start)] = mDfPlayerStart;
      JsonArray pinArray = top.createNestedArray("pin");
      pinArray.add(mUartPins[0]);
      pinArray.add(mUartPins[1]); 
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
     bool readFromConfig(JsonObject& root) override
    {
      JsonObject top = root[FPSTR(_dfplayer)];
      if (top.isNull()) 
      {
        #ifdef DEBUG_LOGGING
          Serial.print(FPSTR(_dfplayer));
          Serial.println(F(": no config found"));
        #endif
      }
  
      bool configComplete = true;
      configComplete &= getJsonValue(top[FPSTR(_folder)], mDfPlayerFolder,1);
      configComplete &= getJsonValue(top[FPSTR(_random)], mDfPlayerRandom, false);
      configComplete &= getJsonValue(top[FPSTR(_loopFolder)], mDfPlayerLoopFolder, false);
      configComplete &= getJsonValue(top[FPSTR(_volume)], mDfPlayerVolume, 15);
      configComplete &= getJsonValue(top[FPSTR(_tracknr)], mDfPlayerTrackNr, 1);
      configComplete &= getJsonValue(top[FPSTR(_tracknr)], mDfPlayerStart, false);
      // "pin" fields have special handling in settings page (or some_pin as well)
      configComplete &= getJsonValue(top["TX"][0], mUartPins[0], TX_PIN);
      configComplete &= getJsonValue(top["RX"][1], mUartPins[1], RX_PIN);

      return configComplete;
    }

    /*
     * appendConfigData() is called when user enters usermod settings page
     * it may add additional metadata for certain entry fields (adding drop down is possible)
     * be careful not to add too much as oappend() buffer is limited to 3k
     */
    void appendConfigData()
    {
    }


    /*
     * handleOverlayDraw() is called just before every show() (LED strip update frame) after effects have set the colors.
     * Use this to blank out some LEDs or set them to a different color regardless of the set effect mode.
     * Commonly used for custom clocks (Cronixie, 7 segment)
     */
    void handleOverlayDraw()
    {
    }

    /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId()
    {
      return USERMOD_ID_DFPLAYER_MP3;
    }

   //More methods can be added in the future, this example will then be extended.
   //Your usermod will remain compatible as it does not need to implement all methods from the Usermod base class!
};

const char MyDfPlayerMini::_dfplayer[]   PROGMEM = "dfplayer";
const char MyDfPlayerMini::_folder[]     PROGMEM = "folder";
const char MyDfPlayerMini::_random[]     PROGMEM = "random";
const char MyDfPlayerMini::_loopFolder[] PROGMEM = "loopfolder";
const char MyDfPlayerMini::_volume[]     PROGMEM = "volume";
const char MyDfPlayerMini::_tracknr[]    PROGMEM = "tracknr";
const char MyDfPlayerMini::_start[]      PROGMEM = "start";
