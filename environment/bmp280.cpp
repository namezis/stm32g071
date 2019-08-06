#include <stdint.h>
#include "bmp280.h"
#include "i2c.h"
// Calibration routines taken from Bosch BMP280 datasheet.
void bmp280::begin(i2c * I2C)
{
    this->I2C = I2C;    
    readCalibrationData();
}
int32_t bmp280::readPressure() // returns Pressure * 100
{
    readTemperature();  //Pressure reading needs the temperature reading to calculate pressure
  
    
    uint8_t PressureMSB,PressureLSB, PressureXLSB;
    readRegister(0xF7, &PressureMSB);
    readRegister(0xF8, &PressureLSB);
    readRegister(0xF9, &PressureXLSB);
    //Convert temperature data bytes to 20-bits within 32 bit integer
    uint32_t adc_P = (((uint32_t)PressureMSB<<16)+((uint32_t)PressureLSB<<8)+((uint32_t)PressureXLSB))>>4;
    int32_t var1,var2;
    uint32_t p;
    var1 = (((int32_t)t_fine) >> 1) - (int32_t)64000;
    var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((int32_t)dig_P6);
    var2 = var2 + ((var1*((int32_t)dig_P5)) << 1);
    var2 = (var2 >> 2) + (((int32_t)dig_P4) << 16);
    var1 = (((dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((int32_t)dig_P2)*var1)>>1)) >> 18;
    var1 = ((((32768+var1))*((int32_t)dig_P1)) >> 15);
    if (var1 =0)
    {
        return 0;
    }
    p = (((uint32_t)(((int32_t)1048576)-adc_P) - (var2 >> 12)))*3125;
    if (p < 0x80000000)
    {
        p = (p << 1) / ((uint32_t)var1);
    }
    else
    {
        p = (p / (uint32_t)var1)*2;
    }
    var1 = (((int32_t)dig_P9) * ((int32_t)(((p >> 3)) >> 13))) >> 12;
    var2 = (((int32_t)(p >> 2)) * ((int32_t) dig_P8)) >> 13;
    p = (uint32_t)((int32_t)p + ((var1 + var2 + dig_P7) >> 4));
    return p;    
}
int32_t bmp280::readTemperature() // returns Temperature * 100
{
    int32_t var1,var2,T;
    //function to read the temperature in BMP280, calibrate it and return the value in Celsius
    writeRegister (0xF4, 0x2E);// Initializing the write register for temperature
    uint8_t status =0x08;
    //Start the i2c transmission
    while(status &0x08){
        readRegister(0xF3, &status);
    }
    //Temperature reading transmitted in 3 parts - MSB, LSB & XLSB
    uint8_t TemperatureMSB,TemperatureLSB, TemperatureXLSB;
    readRegister(0xFA, &TemperatureMSB);
    readRegister(0xFB, &TemperatureLSB);
    readRegister(0xFC, &TemperatureXLSB);
    
    //Convert temperature data bytes to 20-bits within 32 bit integer
    int32_t adc_T = (((long)TemperatureMSB<<16)+((long)TemperatureLSB<<8)+(long)TemperatureXLSB)>>4;
    
    var1 = ((((adc_T >> 3) - ((int32_t)dig_T1 << 1 ))) * ((int32_t) dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)dig_T1)) * ((adc_T >> 4) - ((int32_t)dig_T1))) >> 12) * ((int32_t)dig_T3)) >> 14;
    t_fine = var1 + var2; // store to member variable so that pressure routine can access temperature data
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

int bmp280::readRegister(uint8_t RegNum, uint8_t *Value)
{
    //reads a series of bytes, starting from a specific register
    int nack;
    nack = I2C->read(BMP280_R_ADDRESS,RegNum,Value,1); // read a byte from the register and store in buffer
    return(nack);
}
int bmp280::writeRegister(uint8_t RegNum, uint8_t Value)
{
    //sends a byte to a specific register
    uint8_t Buffer[2];    
    Buffer[0]= Value;    
    int nack;
    nack = I2C->write(BMP280_R_ADDRESS, RegNum, Buffer,1);
    return(nack);
}
void bmp280::readCalibrationData()
{
    uint8_t CalibrationData[26];
    int i;
#define CAL_START 0x88 //Defining the address where the calibration data should start 
    for(i=0;i<26;i++)
    {
        //Read calibration table
        readRegister(CAL_START+i, &CalibrationData[i]);
        
    }        
    dig_T1= (uint16_t ((CalibrationData[1])<<8) + uint16_t (CalibrationData[0]));
    dig_T2= (uint16_t ((CalibrationData[3])<<8) + uint16_t (CalibrationData[2]));
    dig_T3= (uint16_t ((CalibrationData[5])<<8) + uint16_t (CalibrationData[4]));
    dig_P1= (uint16_t ((CalibrationData[7])<<8) + uint16_t (CalibrationData[6]));
    dig_P2= (uint16_t ((CalibrationData[9])<<8) + uint16_t (CalibrationData[8]));
    dig_P3= (uint16_t ((CalibrationData[11])<<8)+ uint16_t (CalibrationData[10]));
    dig_P4= (uint16_t ((CalibrationData[13])<<8)+ uint16_t (CalibrationData[12]));
    dig_P5= (uint16_t ((CalibrationData[15])<<8)+ uint16_t (CalibrationData[14]));
    dig_P6= (uint16_t ((CalibrationData[17])<<8)+ uint16_t (CalibrationData[16]));
    dig_P7= (uint16_t ((CalibrationData[19])<<8)+ uint16_t (CalibrationData[18]));
    dig_P8= (uint16_t ((CalibrationData[21])<<8)+ uint16_t (CalibrationData[20]));
    dig_P9= (uint16_t (CalibrationData [23]<<8)+  uint16_t (CalibrationData[22]));
}
    
    
