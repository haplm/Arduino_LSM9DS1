/*
  This file is part of the Arduino_LSM9DS1 library.
  Copyright (c) 2019 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
  
  Modifications by Femme Verbeek, Pijnacker, the Netherlands 23 may 2020, 
  Released to the public domain
  version 2.0.0
  
*/

#include "LSM9DS1.h"

#define LSM9DS1_ADDRESS            0x6b

#define LSM9DS1_WHO_AM_I           0x0f
#define LSM9DS1_CTRL_REG1_G        0x10
#define LSM9DS1_STATUS_REG         0x17
#define LSM9DS1_OUT_X_G            0x18
#define LSM9DS1_CTRL_REG6_XL       0x20
#define LSM9DS1_CTRL_REG8          0x22
#define LSM9DS1_OUT_X_XL           0x28

// magnetometer
#define LSM9DS1_ADDRESS_M          0x1e

#define LSM9DS1_CTRL_REG1_M        0x20
#define LSM9DS1_CTRL_REG2_M        0x21
#define LSM9DS1_CTRL_REG3_M        0x22
#define LSM9DS1_STATUS_REG_M       0x27
#define LSM9DS1_OUT_X_L_M          0x28


LSM9DS1Class::LSM9DS1Class(TwoWire& wire) :
  continuousMode(false), _wire(&wire)
{
}

LSM9DS1Class::~LSM9DS1Class()
{
}

int LSM9DS1Class::begin()
{
  _wire->begin();

  // reset
  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG8, 0x05);
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG2_M, 0x0c);

  delay(10);

  if (readRegister(LSM9DS1_ADDRESS, LSM9DS1_WHO_AM_I) != 0x68) {
    end();

    return 0;
  }

  if (readRegister(LSM9DS1_ADDRESS_M, LSM9DS1_WHO_AM_I) != 0x3d) {
    end();

    return 0;
  }

  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G, 0x78); // 119 Hz, 2000 dps, 16 Hz BW
  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL, 0x70); // 119 Hz, 4G

  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG1_M, 0xb4); // Temperature compensation enable, medium performance, 20 Hz
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG2_M, 0x00); // 4 Gauss
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG3_M, 0x00); // Continuous conversion mode

  return 1;
}

void LSM9DS1Class::setContinuousMode() {
  // Enable FIFO (see docs https://www.st.com/resource/en/datasheet/DM00103319.pdf)
  writeRegister(LSM9DS1_ADDRESS, 0x23, 0x02);
  // Set continuous mode
  writeRegister(LSM9DS1_ADDRESS, 0x2E, 0xC0);

  continuousMode = true;
}

void LSM9DS1Class::setOneShotMode() {
  // Disable FIFO (see docs https://www.st.com/resource/en/datasheet/DM00103319.pdf)
  writeRegister(LSM9DS1_ADDRESS, 0x23, 0x00);
  // Disable continuous mode
  writeRegister(LSM9DS1_ADDRESS, 0x2E, 0x00);

  continuousMode = false;
}

void LSM9DS1Class::end()
{
  writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG3_M, 0x03);
  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G, 0x00);
  writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL, 0x00);

  _wire->end();
}

//************************************      Accelleration      *****************************************

int LSM9DS1Class::readAccel(float& x, float& y, float& z)
{ int16_t data[3];
  if (!readRegisters(LSM9DS1_ADDRESS, LSM9DS1_OUT_X_XL, (uint8_t*)data, sizeof(data))) {
    x = NAN;
    y = NAN;
    z = NAN;
    return 0;
  }
  // See releasenotes   	read =	Unit * Slope * (PFS / 32786 * Data - Offset )
  float scale =  getAccelFS()/32768.0 ;   
  x = accelUnit * accelSlope[0] * (scale * data[0] - accelOffset[0]);
  y = accelUnit * accelSlope[1] * (scale * data[1] - accelOffset[1]);
  z = accelUnit * accelSlope[2] * (scale * data[2] - accelOffset[2]);
  return 1;
}

int LSM9DS1Class::accelAvailable()
{
  if (continuousMode) {
    // Read FIFO_SRC. If any of the rightmost 8 bits have a value, there is data.
    if (readRegister(LSM9DS1_ADDRESS, 0x2F) & 63) {
      return 1;
    }
  } else {
    if (readRegister(LSM9DS1_ADDRESS, LSM9DS1_STATUS_REG) & 0x01) {
      return 1;
    }
  }
  return 0;
}

//Store zero-point calibration measurement as offset, 
//Dimension analysis: 
//The measurement is stripped from it's unit and sensitivity it was measured with.
//This enables independent calibration and changing the unit later
//In a combined calibration this value must be set before setting the Slope
void LSM9DS1Class::setAccelOffset(float x, float y, float z) 
{  accelOffset[0] = x /(accelUnit * accelSlope[0]);  
   accelOffset[1] = y /(accelUnit * accelSlope[1]);
   accelOffset[2] = z /(accelUnit * accelSlope[2]);
}
//Slope is already dimensionless, so it can be stored as is.
void LSM9DS1Class::setAccelSlope(float x, float y, float z) 
{  accelSlope[0] = x ;
   accelSlope[1] = y ;
   accelSlope[2] = z ;
}

int LSM9DS1Class::setAccelODR(uint8_t range) //Sample Rate 0:off, 1:10Hz, 2:50Hz, 3:119Hz, 4:238Hz, 5:476Hz, 6:952Hz, 7:NA
{  range = range << 5;
   if (range==0b11100000) range =0; 
   uint8_t setting = ((readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL) & 0b00011111) | range);
   return writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL,setting) ;
}
float LSM9DS1Class::getAccelODR()
{  float Ranges[] ={0.0, 10.0, 50.0, 119.0, 238.0, 476.0, 952.0, 0.0 };
   uint8_t setting = readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL)  >> 5;
   return Ranges [setting];
}

float LSM9DS1Class::setAccelBW(uint8_t range) //0,1,2,3 Override autoBandwidth setting see doc.table 67
{ range = range & 0b00000011;
  uint8_t RegIs = readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL) & 0b11111000;
  RegIs = RegIs | 0b00000100 | range ;
  return writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL,RegIs) ;
}

float LSM9DS1Class::getAccelBW() //Bandwidth setting 0,1,2,3  see documentation table 67
{ float autoRange[] ={0.0, 408.0, 408.0, 50.0, 105.0, 211.0, 408.0, 0.0 };
  float BWXLRange[] ={ 408.0, 211.0, 105.0, 50.0 };
  uint8_t RegIs = readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL);
  if (bitRead(2,RegIs))  return BWXLRange [RegIs & 0b00000011];    
  else return autoRange [ RegIs >> 5 ];
}

int LSM9DS1Class::setAccelFS(uint8_t range) // 0: ±2g ; 1: ±16g ; 2: ±4g ; 3: ±8g  
{	range = (range & 0b00000011) << 3;
	uint8_t setting = ((readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL) & 0xE7) | range);
	return writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL,setting) ;
}

float LSM9DS1Class::getAccelFS() // Full scale (output = 2.0,  16.0 , 4.0 , 8.0)  
{ float ranges[] ={2.0, 16.0, 4.0, 8.0}; //g
  uint8_t setting = (readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG6_XL) & 0x18) >> 3;
  return ranges[setting] ;
}

//************************************      Gyroscope      *****************************************

int LSM9DS1Class::readGyro(float& x, float& y, float& z)
{ int16_t data[3];
  if (!readRegisters(LSM9DS1_ADDRESS, LSM9DS1_OUT_X_G, (uint8_t*)data, sizeof(data))) 
  { x = NAN;
    y = NAN;
    z = NAN;
    return 0;
  }
  float scale = getGyroFS() / 32768.0;
  x = gyroUnit * gyroSlope[0] * (scale * data[0] - gyroOffset[0]);
  y = gyroUnit * gyroSlope[1] * (scale * data[1] - gyroOffset[1]);
  z = gyroUnit * gyroSlope[2] * (scale * data[2] - gyroOffset[2]);
  return 1;
}

int LSM9DS1Class::gyroAvailable()
{
  if (readRegister(LSM9DS1_ADDRESS, LSM9DS1_STATUS_REG) & 0x02) {
    return 1;
  }
  return 0;
}
//Store zero-point calibration measurement as offset, 
//Dimension analysis: 
//The measurement is stripped from it's unit and sensitivity it was measured with.
//This enables independent calibration and changing the unit later
//In a combined calibration this value must be set before setting the Slope
void LSM9DS1Class::setGyroOffset(float x, float y, float z) 
{  gyroOffset[0] = x /(gyroUnit * gyroSlope[0]); 
   gyroOffset[1] = y /(gyroUnit * gyroSlope[1]);
   gyroOffset[2] = z /(gyroUnit * gyroSlope[2]);
}
//Slope is already dimensionless, so it can be stored as is.
void LSM9DS1Class::setGyroSlope(float x, float y, float z) 
{  gyroSlope[0] = x ;
   gyroSlope[1] = y ;
   gyroSlope[2] = z ;
}
  
int LSM9DS1Class::setGyroODR(uint8_t range) // 0:off, 1:10Hz, 2:50Hz, 3:119Hz, 4:238Hz, 5:476Hz, 6:952Hz, 7:NA
{  range = range << 5;
   if (range==0b11100000) range =0; 
   uint8_t setting = ((readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G) & 0b00011111) | range);
   return writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G,setting) ;	
}

float LSM9DS1Class::getGyroODR()
{  float Ranges[] ={0.0, 10.0, 50.0, 119.0, 238.0, 476.0, 952.0, 0.0 };  //Hz
   uint8_t setting = readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G)  >> 5;
   return Ranges [setting]; //   return 119.0F;
}

int LSM9DS1Class::setGyroBW(uint8_t range)
{  range = range & 0b00000011;
   uint8_t setting = readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G) & 0b11111100;
   return writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G,setting | range) ;	
}

#define ODRrows 8
#define BWcols 4
float BWtable[ ODRrows ][ BWcols ] =      // acc to  table 47 dataheet 
       {  { 0,  0,  0,  0   }, 
          { 0,  0,  0,  0   },
          { 16, 16, 16, 16  },
          { 14, 31, 31, 31  },
          { 14, 29, 63, 78  },
          { 21, 28, 57, 100 },
          { 33, 40, 58, 100 },
          { 0,  0,  0,  0   }   };

float LSM9DS1Class::getGyroBW()
{  uint8_t setting = readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G) ;
   uint8_t ODR = setting >> 5;
   uint8_t BW = setting & 0b00000011;
   return BWtable[ODR][BW];
}
   
int LSM9DS1Class::setGyroFS(uint8_t range) // (0: 245 dps; 1: 500 dps; 2: 1000  dps; 3: 2000 dps)
{    range = (range & 0b00000011) << 3;	
	 uint8_t setting = ((readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G) & 0xE7) | range );
	 return writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G,setting) ;
}

float LSM9DS1Class::getGyroFS() //   
{ float Ranges[] ={245.0, 500.0, 1000.0, 2000.0}; //dps
  uint8_t setting = (readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG1_G) & 0x18) >> 3;
  return Ranges[setting] ;
}

//************************************      Magnetic field      *****************************************

int LSM9DS1Class::readMagneticField(float& x, float& y, float& z)
{  int16_t data[3];

  if (!readRegisters(LSM9DS1_ADDRESS_M, LSM9DS1_OUT_X_L_M, (uint8_t*)data, sizeof(data))) {
    x = NAN;
    y = NAN;
    z = NAN;
    return 0;
  }
  float scale = getMagnetFS() / 32768.0;
  x = magnetUnit * magnetSlope[0] * (scale * data[0] - magnetOffset[0]);
  y = magnetUnit * magnetSlope[1] * (scale * data[1] - magnetOffset[1]);
  z = magnetUnit * magnetSlope[2] * (scale * data[2] - magnetOffset[2]);
  return 1;
}

int LSM9DS1Class::magneticFieldAvailable()
{ //return (readRegister(LSM9DS1_ADDRESS_M, LSM9DS1_STATUS_REG_M) & 0x08)==0x08;
  if (readRegister(LSM9DS1_ADDRESS_M, LSM9DS1_STATUS_REG_M) & 0x08) {
    return 1;
  }
  return 0;
}
//Store zero-point calibration measurement as offset, 
//Dimension analysis: 
//The measurement is stripped from it's unit and sensitivity it was measured with.
//This enables independent calibration and changing the unit later
//In a combined calibration this value must be set before setting the Slope
void LSM9DS1Class::setMagnetOffset(float x, float y, float z) 
{  magnetOffset[0] = x /(magnetUnit * magnetSlope[0]); 
   magnetOffset[1] = y /(magnetUnit * magnetSlope[1]);
   magnetOffset[2] = z /(magnetUnit * magnetSlope[2]);
}
//Slope is already dimensionless, so it can be stored as is.
void LSM9DS1Class::setMagnetSlope(float x, float y, float z) 
{  magnetSlope[0] = x ;
   magnetSlope[1] = y ;
   magnetSlope[2] = z ;
}
	
int LSM9DS1Class::setMagnetFS(uint8_t range) // 0=400.0; 1=800.0; 2=1200.0 , 3=1600.0  (µT)
{    range = (range & 0b00000011) << 5;	
	 return writeRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG2_M,range) ;
}

float LSM9DS1Class::getMagnetFS() //   
{ const float Ranges[] ={400.0, 800.0, 1200.0, 1600.0}; //
  uint8_t setting = readRegister(LSM9DS1_ADDRESS, LSM9DS1_CTRL_REG2_M)  >> 5;
  return  Ranges[setting] ;
}

int LSM9DS1Class::setMagnetODR(uint8_t range)  // range (0..7) corresponds to {0.625,1.25,2.5,5.0,10.0,20.0,40.0,80.0}Hz
{ range = (range & 0b00000111) << 2;
   uint8_t setting = ((readRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG1_M) & 0b11100011) | range);
   return writeRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG1_M,setting) ;	 
}

float LSM9DS1Class::getMagnetODR()  // Output {0.625, 1.25, 2.5, 5.0, 10.0, 20.0, 40.0 , 80.0}; //Hz
{ const float ranges[] ={0.625, 1.25,2.5, 5.0, 10.0, 20.0, 40.0 , 80.0}; //Hz
  uint8_t setting = (readRegister(LSM9DS1_ADDRESS_M, LSM9DS1_CTRL_REG1_M) & 0b00011100) >> 2;
  return ranges[setting];
}

//************************************      Private functions      *****************************************


int LSM9DS1Class::readRegister(uint8_t slaveAddress, uint8_t address)
{
  _wire->beginTransmission(slaveAddress);
  _wire->write(address);
  if (_wire->endTransmission() != 0) {
    return -1;
  }

  if (_wire->requestFrom(slaveAddress, 1) != 1) {
    return -1;
  }

  return _wire->read();
}

int LSM9DS1Class::readRegisters(uint8_t slaveAddress, uint8_t address, uint8_t* data, size_t length)
{
  _wire->beginTransmission(slaveAddress);
  _wire->write(0x80 | address);
  if (_wire->endTransmission(false) != 0) {
    return -1;
  }

  if (_wire->requestFrom(slaveAddress, length) != length) {
    return 0;
  }

  for (size_t i = 0; i < length; i++) {
    *data++ = _wire->read();
  }

  return 1;
}

int LSM9DS1Class::writeRegister(uint8_t slaveAddress, uint8_t address, uint8_t value)
{
  _wire->beginTransmission(slaveAddress);
  _wire->write(address);
  _wire->write(value);
  if (_wire->endTransmission() != 0) {
    return 0;
  }

  return 1;
}

#ifdef ARDUINO_ARDUINO_NANO33BLE
LSM9DS1Class IMU(Wire1);
#else
LSM9DS1Class IMU(Wire);
#endif
