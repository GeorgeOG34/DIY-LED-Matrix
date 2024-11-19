#include <arduinoFFT.h>
#include <Adafruit_NeoPixel.h>
#include <string>
#include <BLEDevice.h>


// LED Strip setup
#define LED_STRIP_1_PIN 16
#define LED_STRIP_2_PIN 17
#define LED_STRIP_3_PIN 18
#define LED_STRIP_4_PIN 19
#define LED_STRIP_5_PIN 21
#define PIXELS_PER_STRIP 30
Adafruit_NeoPixel ledStrip1(PIXELS_PER_STRIP, LED_STRIP_1_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ledStrip2(PIXELS_PER_STRIP, LED_STRIP_2_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ledStrip3(PIXELS_PER_STRIP, LED_STRIP_3_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ledStrip4(PIXELS_PER_STRIP, LED_STRIP_4_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ledStrip5(PIXELS_PER_STRIP, LED_STRIP_5_PIN, NEO_GRB + NEO_KHZ800);

Adafruit_NeoPixel ledStrips[5];

int squareShape[8] = {1, 2, 3, 31, 33, 61, 62, 63};
int firstShape[] = {31,32,33,35,36,37,39,40,41,62,65,67,70,92,95,97,100,121,122,123,125,126,127,130}; // Spells out IOT on the matrix
// Mic Setup
int const MIC_PIN = 15;
unsigned int sample;

// BLE Setup
BLEUUID serviceID("f05fd76c-2716-4f2c-b8a2-6aeeb421670d");
BLEUUID writableVisualisationModeCharID("e03b9efb-a036-4585-a50b-bd38b7b95c5b");
BLEUUID writableAnalysisCharID("48a703f9-1a7f-4d1f-8e2c-0957d3f47740"); //Writable char to set the analysis mode
BLEUUID writableColourCharID("da3f3e57-b5f1-49ed-9a5f-8267cd36b932"); //Writable char to set the default colour
BLEUUID writableShapeCharID("cf18dca6-b528-4034-830e-8175f76aaacb"); //Writable char to set the shape

class MyCallbacks: public BLECharacteristicCallbacks {
  String savedValue = "";
  String visulisationMode = "LEVELS";
  String analysisMode = "PEAK_TO_PEAK_AMPLITUDE";
  String colour = "red";
  String shape = "none";

  //this method will be call to perform writes to characteristic
  void onWrite(BLECharacteristic *pCharacteristic) {
    if (writableVisualisationModeCharID.equals(pCharacteristic->getUUID())) { //is it our characteristic ?
      savedValue = pCharacteristic->getValue(); //get the data associated with it .
      visulisationMode = pCharacteristic->getValue();
    } else if (writableAnalysisCharID.equals(pCharacteristic->getUUID())) {
      analysisMode = pCharacteristic->getValue();
    } else if (writableColourCharID.equals(pCharacteristic->getUUID())) {
      colour = pCharacteristic->getValue();
    } else if (writableShapeCharID.equals(pCharacteristic->getUUID())) {
      shape = pCharacteristic->getValue();
    }
  }
  public:
    String getSavedValue() {
      return savedValue;
    }
    String getVisulisationMode() {
      return visulisationMode;
    }
    String getColour() {
      return colour;
    }
    String getAnalysisMode() {
      return analysisMode;
    }
    String getShape() {
      return shape;
    }
};

class ServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("Client connected");
  }

  void onDisconnect(BLEServer* pServer) {
    Serial.println("Client disconnected, restarting advertising...");
    BLEDevice::startAdvertising();
  }
};


MyCallbacks cb;
ServerCallbacks sb;

// variables

char currentAudioEffect[] = "AMPLITUDE";
// String currentVisualEffect = "LEVELS";
int currentBrightness = 100;
int currentColour[3] = {255, 0, 0};
bool shouldUpdateVisuals;

long cumulativeAmpSample = 0;
long totalSamples = 0;
int successivePeakCount = 0;

void setup() {
  Serial.begin(115200);
  // mic setup
  analogReadResolution(12);

  // led strips setup
  shouldUpdateVisuals = true;
  ledStrips[0] = ledStrip1;
  ledStrips[1] = ledStrip2;
  ledStrips[2] = ledStrip3;
  ledStrips[3] = ledStrip4;
  ledStrips[4] = ledStrip5;

  ////////////// BLE setup
  BLEDevice::init("gmo22");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(serviceID);
  pServer->setCallbacks(&sb);

  // VISUALISATION CHARACTERISTIC //
  BLECharacteristic *writeVisulisationModeCharacteristic = pService->createCharacteristic(
  writableVisualisationModeCharID,
  BLECharacteristic::PROPERTY_WRITE
  );
  writeVisulisationModeCharacteristic ->setCallbacks(&cb);

  // ANALYSIS MODE CHARACTERISTIC //
  BLECharacteristic *writableAnalysisCharacteristic = pService->createCharacteristic(
  writableAnalysisCharID,
  BLECharacteristic::PROPERTY_WRITE
  );
  writableAnalysisCharacteristic ->setCallbacks(&cb);

  // DEFAULT COLOUR CHARACTERISTIC //
  BLECharacteristic *writableColourCharacteristic = pService->createCharacteristic(
  writableColourCharID,
  BLECharacteristic::PROPERTY_WRITE
  );
  writableColourCharacteristic ->setCallbacks(&cb);

  // SHAPE CHARACTERISTIC //
  BLECharacteristic *writableShapeCharacteristic = pService->createCharacteristic(
  writableShapeCharID,
  BLECharacteristic::PROPERTY_WRITE
  );
  writableShapeCharacteristic ->setCallbacks(&cb);

  pService->start(); //Starts the service

  // Advertise
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(serviceID);
  pAdvertising->setScanResponse(true);

  pAdvertising->setMinPreferred(0x06); // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);

  BLEDevice::startAdvertising();

  //Enable the LED
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite (LED_BUILTIN, HIGH);

  setMatrixColour(currentColour);
}


void loop() {
  double analysisValue = 1000;
  double currentMaxValue = 4095;

  if (cb.getAnalysisMode() == "PEAK_TO_PEAK_AMPLITUDE") {
    analysisValue = getPeakToPeakAmplitude();
    currentMaxValue = 4095;
  } else if (cb.getAnalysisMode() == "FFT_DOMINANT") {
    analysisValue = getDominantFrequencyFft();
    currentMaxValue = 2000;
  }


  setCurrentPixels();
  if (cb.getShape() != "none" && cb.getAnalysisMode() != "FFT_BUCKETS") {
    displayCurrentShape();
    bouncePixels();
  } else {
    setMatrixColour(currentColour);
  }

  if (shouldUpdateVisuals && cb.getAnalysisMode() != "FFT_BUCKETS") {
    updateCurrentColour();
    if (cb.getVisulisationMode() == "SWITCH") {
      colourSwitchVisualisation(analysisValue, currentMaxValue);
    }
    if (cb.getVisulisationMode() == "FLASH") {
      flashVisualisation(analysisValue, currentMaxValue);
    }
    if (cb.getVisulisationMode() == "LEVELS") {
      showLedMatrix();
      levelsVisualisation(analysisValue, currentMaxValue);
    }
  } else if (cb.getAnalysisMode() == "FFT_BUCKETS"){
    updateCurrentColour();
    frequencyBucketsFftModeAndEffect();
  }
}



#define SAMPLES 512           // number of samples must be a power of 2
#define SAMPLING_FREQUENCY 4000  // Hz (samples per second)

double vReal[SAMPLES];
double vImag[SAMPLES];

uint8_t currentPixels[150];

ArduinoFFT<double> FFT = ArduinoFFT<double>(vReal, vImag, SAMPLES, SAMPLING_FREQUENCY);

/*
 * this function get samples from mic outpput over at set intervals and then using FFT to calculate a list of
 * frequencies and their corresponding magnitude using FFT. From that we then return the frequency with the highest magnitude.
**/
double getDominantFrequencyFft() {
  for (int i = 0; i < SAMPLES; i++) {
    vReal[i] = analogRead(MIC_PIN) - 2000; // -2000 as the mic is output is centered at 2000 rather than 0. FFT works better when its centered at 0
    vImag[i] = 0;
    delayMicroseconds(1000000 / SAMPLING_FREQUENCY); // equivalent to 1 per 250microseconds - allowing us to detect up to 2000Hz
  }

  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD); //this smooths out our data
  FFT.compute(FFT_FORWARD); // this runs the FFT
  FFT.complexToMagnitude();

  double peakFrequency; // = dominant / one with biggest magnitude
  double peakMagnitude;

  FFT.majorPeak(&peakFrequency, &peakMagnitude);  // Frequency and its magnitude

  return peakFrequency;
}



/*
 * this function get samples from mic output over at set intervals and then using FFT to calculate a list of
 * frequencies and their corresponding magnitude using FFT.
 * We then group these frequencies and total up each groups magnitude.
 * We then represent the groups magnitude as a percentage of the total magnitude on a led strip.
**/
void frequencyBucketsFftModeAndEffect() {
  for (int i = 0; i < SAMPLES; i++) {
    vReal[i] = analogRead(MIC_PIN) - 2000; // -2000 as the mic is output is centered at 2000 rather than 0. FFT works better when its centered at 0
    vImag[i] = 0;
    delayMicroseconds(1000000 / SAMPLING_FREQUENCY);
  }

  FFT.windowing(FFT_WIN_TYP_HAMMING, FFT_FORWARD);
  FFT.compute(FFT_FORWARD);
  FFT.complexToMagnitude();
  double buckets[4] = {0, 0, 0, 0};

  for (int i = 0; i < (SAMPLES / 2); i++) {
    double frequency = (i * (SAMPLING_FREQUENCY / SAMPLES));
    double magnitude = (vReal[i]);
    double frequencyTier = (frequency / (SAMPLING_FREQUENCY / 2));
    if (frequencyTier < 0.26 ) {
      buckets[0] += magnitude;
    } else if ( frequencyTier > 0.25 && frequencyTier < 0.51) {
      buckets[1] += magnitude;
    } else if ( frequencyTier > 0.50 && frequencyTier < 0.76) {
      buckets[2] += magnitude;
    } else {
      buckets[3] += magnitude;
    }
  }

  double totalMagnitude = buckets[0] + buckets[1] + buckets[2] + buckets[3];
  int colour[] = {0, 0, 0};
  setMatrixColour(colour);
  // Serial.print("totalmag: ");
  // Serial.println(totalMagnitude);
  setStrip(1, currentColour, (int) ((buckets[0] / totalMagnitude) * 30));
  setStrip(2, currentColour, (int) ((buckets[1] / totalMagnitude) * 30));
  setStrip(3, currentColour, (int) ((buckets[2] / totalMagnitude) * 30));
  setStrip(4, currentColour, (int) ((buckets[3] / totalMagnitude) * 30));
  setMatrixBrightness(255);
  showLedMatrix();
}

/*
* the SWITCH visualisation works like LEVELS but changes colours instead of brightness
*/
void colourSwitchVisualisation(double analysisValue, double currentMaxValue) {
  int percentageOfMaxValue = (int) ((analysisValue / currentMaxValue) * 100);
  if (percentageOfMaxValue < 24) {
        int colour[] = {0, 0, 255};
        currentColour[0] = 0;
        currentColour[1] = 0;
        currentColour[2] = 255;
      } else if (percentageOfMaxValue > 23 && percentageOfMaxValue < 40) {
        int colour[] = {0, 255, 0};
        currentColour[0] = 0;
        currentColour[1] = 255;
        currentColour[2] = 0;
        // setMatrixColour(colour);
      } else if (percentageOfMaxValue > 39 && percentageOfMaxValue < 55) {
        int colour[] = {255, 0, 0};
        currentColour[0] = 255;
        currentColour[1] = 0;
        currentColour[2] = 0;
      } else {
        int colour[] = {252, 15, 192};
        currentColour[0] = 252;
        currentColour[1] = 15;
        currentColour[2] = 192;
      }
      if (cb.getShape() != "none") {
        displayCurrentShape();
      } else {
        setMatrixColour(currentColour);
      }

      setMatrixBrightness(255);
      showLedMatrix();
}

/*
* Flash visualisation uses same categories as LEVELS except only the highest tier has a brightness above 1
*/
void flashVisualisation(double analysisValue, double currentMaxValue) {
  int percentageOfMaxValue = (int) ((analysisValue / currentMaxValue) * 100);
  int brightness = 1;
  if (percentageOfMaxValue > 55) {
    brightness = 255;
  } else {
    brightness = 1;
  }
  setMatrixBrightness(brightness);
  showLedMatrix();
}

/*
* Levels visualisation takes puts the input into 4 categoes as a percentage of the maximum possible value that could've been inputed
* each level gets brighter the higher the value.
*/
void levelsVisualisation(double analysisValue, double currentMaxValue) {
  int percentageOfMaxValue = (int) ((analysisValue / currentMaxValue) * 100);
  int brightness = 0;
  if (percentageOfMaxValue < 24) {
    brightness = 1;
  } else if (percentageOfMaxValue > 23 && percentageOfMaxValue < 40) {
    brightness = 50;
  } else if (percentageOfMaxValue > 39 && percentageOfMaxValue < 55) {
    brightness = 100;
  } else {
    brightness = 255;
  }
  setMatrixBrightness(brightness);

  showLedMatrix();
}

/*
* Sets the brightness of the entire matrix
*/
void setMatrixBrightness(int brightness) {
  ledStrip1.setBrightness(brightness);
  ledStrip2.setBrightness(brightness);
  ledStrip3.setBrightness(brightness);
  ledStrip4.setBrightness(brightness);
  ledStrip5.setBrightness(brightness);
}


/*
* Set the colour of the entire matrix overriding currentPixels
*/
void setMatrixColour(int colour[]) {
  for (int i = 0; i < (sizeof(ledStrips) / sizeof(ledStrips[0])); i++) {
    setStrip(i + 1, colour, PIXELS_PER_STRIP);
  }
}

/*
* Calls the show function on the Adafruit_NeoPixel objects which updates the strip to whatever arrangement of
* colours and brightness have been set
*/
void showLedMatrix() {
  for (int i = 0; i < (sizeof(ledStrips) / sizeof(ledStrips[0])); i++) {
    ledStrips[i].show();
  }
}

/*
* updates the updateCurrentColour based on preset values
*/
void updateCurrentColour() {
  if (cb.getColour() == "green") {
      currentColour[0] = 0;
      currentColour[1] = 255;
      currentColour[2] = 0;
  } else if (cb.getColour() == "red") {
    currentColour[0] = 255;
    currentColour[1] = 0;
    currentColour[2] = 0;
  } else if (cb.getColour() == "blue") {
    currentColour[0] = 0;
    currentColour[1] = 0;
    currentColour[2] = 255;
  }
}

/*
* this is to be used instead of setMatrixColour when a shape is active. (e.g. not the whole matrix is lit up)
*/
void displayCurrentShape() {
  int  i = 0;
  bool displayed = false;

  int blank[3] = {0, 0, 0};

  setMatrixColour(blank);

  while(!displayed) {
    int row = 0;
    int column = 0;

    if ( i  < (sizeof(currentPixels) / sizeof(currentPixels[0]))) {
      if (currentPixels[i] < 0) {
        displayed = true;
        break;
      }
     row = ((int) (currentPixels[i] / 30)) + 1;
    } else {
      displayed = true;
      break;
    }
    // Serial.print("row");
    // Serial.println(row);
    // Serial.println(currentPixels[i]);
     switch(row) {
      case 1:
        ledStrip1.setPixelColor(currentPixels[i] - 1, ledStrip1 .Color(currentColour[0], currentColour[1], currentColour[2]));
        break;
      case 2:
        ledStrip2.setPixelColor(currentPixels[i] - 31, ledStrip1.Color(currentColour[0], currentColour[1], currentColour[2]));
        break;
      case 3:
        ledStrip3.setPixelColor(currentPixels[i] - 61, ledStrip1.Color(currentColour[0], currentColour[1], currentColour[2]));
        break;
      case 4:
        ledStrip4.setPixelColor(currentPixels[i] - 91, ledStrip1.Color(currentColour[0], currentColour[1], currentColour[2]));
        break;
      case 5:
        ledStrip5.setPixelColor(currentPixels[i] - 121, ledStrip1.Color(currentColour[0], currentColour[1], currentColour[2]));
        break;
      default:
        break;
    }
    i = i + 1;
  }
  // Serial.println("break");
  showLedMatrix();
}

/*
* This updates the currentPixels to be that of the shape
* most of this code is only called when the user changes shape
*/
String activeShape = "none";
void setCurrentPixels() {
  int numOfPixels = 0;
  if (cb.getShape() == "square") {
    numOfPixels = (sizeof(squareShape) / sizeof(squareShape[0]));
    if (activeShape!= "square") {
      for(int i = 0; i < numOfPixels; i++) {
        currentPixels[i] = squareShape[i];
      }
      activeShape = "square";
    }
    currentPixels[numOfPixels] = -1;
  } else if (cb.getShape() == "first") {
    numOfPixels = (sizeof(firstShape) / sizeof(firstShape[0]));
    if (activeShape!= "first") {
      for(int i = 0; i < numOfPixels; i++) {
        currentPixels[i] = firstShape[i];
      }
      activeShape = "first";
    }
    currentPixels[numOfPixels] = -1;
  } else {
    numOfPixels = (sizeof(currentPixels) / sizeof(currentPixels[0]));
    if (activeShape!= "none") {
      for(int i = 0; i < numOfPixels; i++) {
        if (currentPixels[i] > -1) {
          currentPixels[i] = -1;
        }
      }
    }
    activeShape = "none";
  }
}

/*
* this method figures out if the current shape is at the edge of the matrix and if it is then it goes the other way
*/
bool direction = true;
void bouncePixels() {
  int numOfPixels = (sizeof(currentPixels) / sizeof(currentPixels[0]));
  bool move = true;
  for(int i = 0; i < numOfPixels; i++) {
    if (currentPixels[i] < 0) {
      break;
    }
    if (direction) {
      switch(currentPixels[i]) {
        case 29:
          move = false;
          break;
        case 59:
          move = false;
          break;
        case 89:
          move = false;
          break;
        case 119:
          move = false;
          break;
        case 149:
          move = false;
          break;
        default:
          break;
      }
    } else if (!direction) {
      switch(currentPixels[i]){
        case 0:
          move = false;
          break;
        case 30:
          move = false;
          break;
        case 60:
          move = false;
          break;
        case 90:
          move = false;
          break;
        case 120:
          move = false;
          break;
        default:
          break;
      }
    }
  }
  if (direction && move) {
    shiftCurrentPixelsRight();
  } else if(!direction && move) {
    shiftCurrentPixelsLeft();
  } else if (!move) {
    direction = !direction;
  }
}


/*
* This method adds 1 from every value in currentPixels,
* giving the visual effect of shifting moving to the right
*/
void shiftCurrentPixelsRight() {
  int numOfPixels = (sizeof(currentPixels) / sizeof(currentPixels[0]));
  for(int i = 0; i < numOfPixels; i++) {
    if (currentPixels[i] < 0) {
      break;
    }
    currentPixels[i] = currentPixels[i] + 1;
  }
}

/*
* This method subtracts 1 from every value in currentPixels,
* giving the visual effect of shifting moving to the left
*/
void shiftCurrentPixelsLeft() {
  int numOfPixels = (sizeof(currentPixels) / sizeof(currentPixels[0]));
  for(int i = 0; i < numOfPixels; i++) {
    if (currentPixels[i] < 0) {
      break;
    }
    currentPixels[i] = currentPixels[i] - 1;
  }
}

/*
 * Method sets a specific strip a colour
**/
void setStrip(int ledStripNumber, int colour[3], int pixelsPerStrip) {
  int pixel = 0;
  for (int pixel = 0; pixel < pixelsPerStrip; pixel++) {
    switch(ledStripNumber) {
      case 1:
        ledStrip1.setPixelColor(pixel, ledStrip1.Color(colour[0], colour[1], colour[2]));
        break;
      case 2:
        ledStrip2.setPixelColor(pixel, ledStrip2.Color(colour[0], colour[1], colour[2]));
        break;
      case 3:
        ledStrip3.setPixelColor(pixel, ledStrip3.Color(colour[0], colour[1], colour[2]));
        break;
      case 4:
        ledStrip4.setPixelColor(pixel, ledStrip4.Color(colour[0], colour[1], colour[2]));
        break;
      case 5:
        ledStrip5.setPixelColor(pixel, ledStrip5.Color(colour[0], colour[1], colour[2]));
        break;
      default:
        break;

    }
  }
}


/*
 * This method returns the peak to peak amplitude gotten by the mic
 * this is done by getting the highest and lowest values in a period of 50ms
**/
double getPeakToPeakAmplitude() {
  unsigned long startMillis = millis();
  unsigned int peakToPeakAmplitude = 0;

  unsigned int signalMax = 0;
  unsigned int signalMin = 4095; // 4095 is the max 12bit output from the 4095 so the min value should be less.

  while (millis() - startMillis < 50)
  {
    sample = analogRead(MIC_PIN);
    if (sample < 4095)
    {
      if (sample > signalMax)
      {
        signalMax = sample;
      }
      else if (sample < signalMin)
      {
        signalMin = sample;

      }
    }
  }
  peakToPeakAmplitude = signalMax - signalMin;

  return (double) peakToPeakAmplitude;
}
