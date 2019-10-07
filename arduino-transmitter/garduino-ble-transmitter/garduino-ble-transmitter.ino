#include <SoftwareSerial.h>
#include <ArduinoJson.h>
#include <SimpleDHT.h>
#include <Adafruit_BMP280.h>

const int DHT11_PIN = 2;
const int LIGHT_SENSOR_PIN = A0;
const int MOISTURE_PIN_0 = A1;
const int MOISTURE_PIN_1 = A2;
const int MOISTURE_PIN_2 = A3;

const int MOISTURE_CALIB_MIN = 0;
const int MOISTURE_CALIB_MAX = 455;

const int LIGHT_CALIB_MIN = 750;
const int LIGHT_CALIB_MAX = 0;

const double ALTITUDE_DEGGENDORF = 314.0;

SoftwareSerial software_serial(10, 11); // RX, TX
Adafruit_BMP280 bmp; // I2C
SimpleDHT11 dht11(DHT11_PIN);

enum {
    BMP280_1,
    BMP280_2,
    DHT11,
    MOISTURE12,
    MOISTURE3_LIGHT
} SENDER_STATE;

void setup()
{
    Serial.begin(9600);
    if (!bmp.begin())
    {
        Serial.println(F("Could not find a valid BMP280 sensor, check wiring!"));
    }
    software_serial.begin(9600);
    SENDER_STATE = BMP280_1;
}

void loop()
{
    //read from the HM-10 and print in the Serial
    if (software_serial.available())
        Serial.write(software_serial.read());

    //read from the Serial and print to the HM-10
    if (Serial.available())
        software_serial.write(Serial.read());

    Serial.println("============= SAMPLING =============");

    Serial.println("Sample DHT11...");
    byte dht11_temperature = 0;
    byte humidity = 0;
    int err = SimpleDHTErrSuccess;
    if ((err = dht11.read(&dht11_temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
    {
        Serial.print("Read DHT11 failed, err=");
        Serial.println(err);
        delay(1000);
    }

    Serial.println("Sample BMP280...");
    double bmp280_temperature = bmp.readTemperature();
    double pressure_hpa = calc_relative_sealevel_pressure(bmp.readPressure() / 100.0,
                                                          bmp280_temperature, ALTITUDE_DEGGENDORF);

    Serial.println("Sample ME110 sensors...");
    int moisture_value_0 = read_moisture(MOISTURE_PIN_0);
    int moisture_value_1 = read_moisture(MOISTURE_PIN_1);
    int moisture_value_2 = read_moisture(MOISTURE_PIN_2);

    Serial.println("Sample light sensor...");
    int light_intensity = read_light_intensity(LIGHT_SENSOR_PIN);

    Serial.println("============= OUTPUT =============");

    String sensor_values = "{";

    Serial.print("temperature = ");
    Serial.print(bmp280_temperature);
    Serial.println(" °C");

    Serial.print("pressure = ");
    Serial.print(pressure_hpa);
    Serial.println(" hPa");

    Serial.print("relative humidity = ");
    Serial.print(humidity);

    Serial.print(" % RH @ ");
    Serial.print(dht11_temperature);
    Serial.println(" °C");

    Serial.print("soil moisture 0 = ");
    Serial.print(moisture_value_0);
    sensor_values += String(moisture_value_0) + F(",");
    Serial.println(" %");
    Serial.print("soil moisture 1 = ");
    Serial.print(moisture_value_1);
    sensor_values += String(moisture_value_1) + F(",");
    Serial.println(" %");
    Serial.print("soil moisture 2 = ");
    Serial.print(moisture_value_2);
    sensor_values += String(moisture_value_2) + F(",");
    Serial.println(" %");

    Serial.print("light intensity = ");
    Serial.print(light_intensity);
    sensor_values += String(light_intensity) + F(",");
    Serial.println(" %");

    Serial.println("=========== OUTPUT VIA BLE ===========");
    StaticJsonDocument<200> json_output;

    // hack for small BLE messages
    switch (SENDER_STATE)
    {
    case BMP280_1:
        json_output["t"] = round(bmp280_temperature * 100) / 100;
        SENDER_STATE = BMP280_2;
        break;
    case BMP280_2:
        json_output["p"] = round(pressure_hpa * 100) / 100;
        SENDER_STATE = DHT11;
        break;
    case DHT11:
        json_output["rh"] = humidity;
        json_output["t2"] = dht11_temperature;
        SENDER_STATE = MOISTURE12;
        break;
    case MOISTURE12:
        json_output["s0"] = moisture_value_0;
        json_output["s1"] = moisture_value_1;
        SENDER_STATE = MOISTURE3_LIGHT;
        break;
    case MOISTURE3_LIGHT:
        json_output["s2"] = moisture_value_2;
        json_output["li"] = light_intensity;
        SENDER_STATE = BMP280_1;
        break;
    
    default:
        break;
    }
    serializeJson(json_output, software_serial);

    delay(1500);
}

int read_light_intensity(int pin)
{
    return static_cast<int>(map(analogRead(pin), LIGHT_CALIB_MIN, LIGHT_CALIB_MAX, 0, 100));
}

int read_moisture(int pin)
{
    return static_cast<int>(map(analogRead(pin), MOISTURE_CALIB_MIN, MOISTURE_CALIB_MAX, 0, 100));
}

double calc_relative_sealevel_pressure(double absolute_pressure_hpa, double temperature, double altitude)
{
    return absolute_pressure_hpa * pow(1 - ((0.0065 * altitude) / (temperature + (0.0065 * altitude) + 273.15)), -5.257);
}