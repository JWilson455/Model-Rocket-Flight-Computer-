#include <Adafruit_APDS9960.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_LIS3MDL.h>
#include <Adafruit_LSM6DS3TRC.h>
#include <Adafruit_SHT31.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_GPS.h>
#include <RTClib.h>
#include <SPI.h>
#include <SD.h>
#include <PDM.h>

// ================= SENSORS =================
Adafruit_APDS9960 apds9960;
Adafruit_BMP280 bmp280;
Adafruit_LIS3MDL lis3mdl;
Adafruit_LSM6DS3TRC imu;
Adafruit_SHT31 sht30;

// ================= GPS =================
Adafruit_GPS GPS(&Serial1);

// ================= RTC =================
RTC_PCF8523 rtc;

// ================= SD =================
File logfile;
const int SD_CS = 10;

// ================= MIC =================
extern PDMClass PDM;
short sampleBuffer[256];
volatile int samplesRead = 0;

// ================= VARIABLES =================
uint16_t r, g, b, c;
float temperature, pressure, altitude;
float magnetic_x, magnetic_y, magnetic_z;
float accel_x, accel_y, accel_z;
float gyro_x, gyro_y, gyro_z;
float humidity;
int32_t mic;

// ==================================================
void setup() {
  delay(2000);
  Serial.begin(115200);
  while (!Serial) delay(10);   // BENCH TEST ONLY

  Serial.println("\n=== Feather Sense + GPS + SD BENCH TEST ===");

  // ---------- Sensors ----------
  apds9960.begin();
  apds9960.enableProximity(true);
  apds9960.enableColor(true);

  bmp280.begin();
  lis3mdl.begin_I2C();
  imu.begin_I2C();
  sht30.begin();

  // ---------- GPS ----------
  Serial1.begin(9600);
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ);
  // Antenna command REMOVED for compatibility

  // ---------- RTC ----------
  if (!rtc.begin()) {
    Serial.println("RTC not found");
  }
  if (!rtc.initialized()) {
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
  }

  // ---------- SD ----------
  if (!SD.begin(SD_CS)) {
    Serial.println("SD init FAILED");
  } else {
    Serial.println("SD init OK");
    logfile = SD.open("flight.csv", FILE_WRITE);
    if (logfile) {
      logfile.println(
        "time,lat,lon,gps_alt,temp,press,alt,ax,ay,az,gx,gy,gz,hum,magx,magy,magz,mic"
      );
      logfile.flush();
    }
  }

  // ---------- Mic ----------
  PDM.onReceive(onPDMdata);
  PDM.begin(1, 16000);

  Serial.println("Setup complete");
}

// ==================================================
void loop() {

  // ---------- GPS UPDATE ----------
  while (GPS.available()) {
    GPS.read();
  }
  if (GPS.newNMEAreceived()) {
    GPS.parse(GPS.lastNMEA());
  }

  // ---------- APDS9960 ----------
  while (!apds9960.colorDataReady()) delay(1);
  apds9960.getColorData(&r, &g, &b, &c);

  // ---------- ENV ----------
  temperature = bmp280.readTemperature();
  pressure    = bmp280.readPressure();
  altitude    = bmp280.readAltitude(1013.25);

  // ---------- MAG ----------
  lis3mdl.read();
  magnetic_x = lis3mdl.x;
  magnetic_y = lis3mdl.y;
  magnetic_z = lis3mdl.z;

  // ---------- IMU ----------
  sensors_event_t accel, gyro, temp;
  imu.getEvent(&accel, &gyro, &temp);

  accel_x = accel.acceleration.x;
  accel_y = accel.acceleration.y;
  accel_z = accel.acceleration.z;
  gyro_x  = gyro.gyro.x;
  gyro_y  = gyro.gyro.y;
  gyro_z  = gyro.gyro.z;

  // ---------- HUM ----------
  humidity = sht30.readHumidity();

  // ---------- MIC ----------
  samplesRead = 0;
  mic = getPDMwave(4000);

  DateTime now = rtc.now();

  // ---------- SERIAL OUTPUT ----------
  Serial.println("\n--- DATA ---");
  Serial.print("Time: "); Serial.println(now.timestamp());
  Serial.print("GPS Fix: "); Serial.println(GPS.fix);
  Serial.print("Lat: "); Serial.println(GPS.latitude, 6);
  Serial.print("Lon: "); Serial.println(GPS.longitude, 6);
  Serial.print("GPS Alt: "); Serial.println(GPS.altitude);

  Serial.print("Temp: "); Serial.println(temperature);
  Serial.print("Pressure: "); Serial.println(pressure);
  Serial.print("Alt: "); Serial.println(altitude);

  Serial.print("Accel: ");
  Serial.print(accel_x); Serial.print(", ");
  Serial.print(accel_y); Serial.print(", ");
  Serial.println(accel_z);

  Serial.print("Gyro: ");
  Serial.print(gyro_x); Serial.print(", ");
  Serial.print(gyro_y); Serial.print(", ");
  Serial.println(gyro_z);

  Serial.print("Mag: ");
  Serial.print(magnetic_x); Serial.print(", ");
  Serial.print(magnetic_y); Serial.print(", ");
  Serial.println(magnetic_z);

  Serial.print("Humidity: "); Serial.println(humidity);
  Serial.print("Mic: "); Serial.println(mic);

  // ---------- SD LOG ----------
  if (logfile) {
    logfile.print(now.timestamp()); logfile.print(",");
    logfile.print(GPS.latitude, 6); logfile.print(",");
    logfile.print(GPS.longitude, 6); logfile.print(",");
    logfile.print(GPS.altitude); logfile.print(",");
    logfile.print(temperature); logfile.print(",");
    logfile.print(pressure); logfile.print(",");
    logfile.print(altitude); logfile.print(",");
    logfile.print(accel_x); logfile.print(",");
    logfile.print(accel_y); logfile.print(",");
    logfile.print(accel_z); logfile.print(",");
    logfile.print(gyro_x); logfile.print(",");
    logfile.print(gyro_y); logfile.print(",");
    logfile.print(gyro_z); logfile.print(",");
    logfile.print(humidity); logfile.print(",");
    logfile.print(magnetic_x); logfile.print(",");
    logfile.print(magnetic_y); logfile.print(",");
    logfile.print(magnetic_z); logfile.print(",");
    logfile.println(mic);
    logfile.flush();
  }

  delay(500);
}

// ================= MIC HELPERS =================
int32_t getPDMwave(int32_t samples) {
  short minwave = 30000;
  short maxwave = -30000;

  while (samples > 0) {
    if (!samplesRead) {
      yield();
      continue;
    }
    for (int i = 0; i < samplesRead; i++) {
      minwave = min(sampleBuffer[i], minwave);
      maxwave = max(sampleBuffer[i], maxwave);
      samples--;
    }
    samplesRead = 0;
  }
  return maxwave - minwave;
}

void onPDMdata() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;
}