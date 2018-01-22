
#include <Wire.h>

// ADXL345 I2C address is 0x53(83)
#define Addr 0x53

void setup()
{
  // Initialise I2C communication as MASTER
  Wire.begin();
  // Initialise serial communication, set baud rate = 9600
  Serial.begin(9600);
  
  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  // Select bandwidth rate register
  Wire.write(0x2C);
  // Normal mode, Output data rate = 100 Hz
  Wire.write(0x0A);
  // Stop I2C transmission
  Wire.endTransmission();
  
  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  // Select power control register
  Wire.write(0x2D);
  // Auto-sleep disable
  Wire.write(0x08);
  // Stop I2C transmission
  Wire.endTransmission();
  
  // Start I2C Transmission
  Wire.beginTransmission(Addr);
  // Select data format register
  Wire.write(0x31);
  // Self test disabled, 4-wire interface, Full resolution, Range = +/-2g
  Wire.write(0x08);
  // Stop I2C transmission
  Wire.endTransmission();
  delay(300);
}

void loop()
{
  unsigned int data[6];
  for(int i = 0; i < 6; i++)
  {
    // Start I2C Transmission
    Wire.beginTransmission(Addr);
    // Select data register
    Wire.write((50 + i));
    // Stop I2C transmission
    Wire.endTransmission();
    
    // Request 1 byte of data
    Wire.requestFrom(Addr, 1);
    
    // Read 6 bytes of data
    // xAccl lsb, xAccl msb, yAccl lsb, yAccl msb, zAccl lsb, zAccl msb
    if(Wire.available() == 1)
    {
      data[i] = Wire.read();
    }
  }
  
  // Convert the data to 10-bits
  int xAccl = (((data[1] & 0x03) * 256) + data[0]);
  if(xAccl > 511)
  {
    xAccl -= 1024;
  }
  int yAccl = (((data[3] & 0x03) * 256) + data[2]);
  if(yAccl > 511)
  {
    yAccl -= 1024;
  }
  int zAccl = (((data[5] & 0x03) * 256) + data[4]);
  if(zAccl > 511)
  {
    zAccl -= 1024;
  }
  
  // Output data to serial monitor
  printf("Acceleration in X-Axis is : ");
  printf(xAccl);
  printf("Acceleration in Y-Axis is : ");
  printf(yAccl);
  printf("Acceleration in Z-Axis is : ");
  printf(zAccl);
  delay(300);
}
