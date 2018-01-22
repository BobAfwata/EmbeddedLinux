byte chargerate = 0x07; //charge rate (0x07 = 3A)
unsigned long chargeratetimer_previous = 0;

void SetupI2C()
{
  Wire.begin(); // join i2c bus (address optional for master)  
}

/* GPIO Expansion Port */
void SetupExp()
{
  Wire.beginTransmission(EXTERNAL_GPIO);
  Wire.write(byte(GPIO_CMD_REG1));
  Wire.write(0x00);
  Wire.endTransmission();
  
  Wire.beginTransmission(EXTERNAL_GPIO);
  Wire.write(byte(GPIO_CMD_REG2));
  Wire.write(0x00);
  Wire.endTransmission();  
}

uint8_t ExpReadPins(uint8_t input)
{
  Wire.beginTransmission(EXTERNAL_GPIO);
  if(input)
    Wire.write(byte(GPIO_CMD_REG0));
  else
    Wire.write(byte(GPIO_CMD_REG1));
  Wire.endTransmission();
  
  Wire.requestFrom(EXTERNAL_GPIO, 1);
  
  uint8_t result = 0;
  if(Wire.available())
    result = Wire.read();
  
  return result;
}

uint8_t ExpWritePins(uint8_t value)
{
  Wire.beginTransmission(EXTERNAL_GPIO);
  Wire.write(byte(GPIO_CMD_REG1));
  Wire.write(value);
  Wire.endTransmission();
}

uint8_t ExpSetMode(uint8_t value)
{
  Wire.beginTransmission(EXTERNAL_GPIO);
  Wire.write(byte(GPIO_CMD_REG3));
  Wire.write((byte)value);
  Wire.endTransmission();
}

/* Charge Controller BQ24190 */
void SetupChargeController() //sets up the default charge controller states. 
{
  byte retval = EEPROM.read(CHG_ADDR+CHG_REG00);
  if(retval == 0xFF) //if the charge controller register = 0 then assume it means its blank! 
    LoadEEPROM();
    
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG0));
  Wire.write(EEPROM.read(CHG_ADDR+CHG_REG00)); //Reg0
  Wire.write(EEPROM.read(CHG_ADDR+CHG_REG01)); //Reg1
  Wire.write(EEPROM.read(CHG_ADDR+CHG_REG02)); //Reg2
  Wire.write(EEPROM.read(CHG_ADDR+CHG_REG03)); //Reg3
  Wire.write(EEPROM.read(CHG_ADDR+CHG_REG04)); //Reg4
  Wire.write(EEPROM.read(CHG_ADDR+CHG_REG05)); //Reg5
  Wire.write(EEPROM.read(CHG_ADDR+CHG_REG06)); //Reg6
  Wire.write(EEPROM.read(CHG_ADDR+CHG_REG07)); //Reg7
  Wire.endTransmission();
}

void ChargeControllerShipMode() //turns off the batfet, and the device will go super sleepy!
{
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG7));
  Wire.endTransmission();  
  
  Wire.requestFrom(CHARGE_CONTROLLER, 1);
  uint8_t result = Wire.read();

  result |= _BV(5);

  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG7));
  Wire.write(result);
  Wire.endTransmission();    
}

void ChargeControllerResetWD() //resets the WD and forces states. 
{
  //this bit of code resets the wd on register 1. 
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG1)); 
  Wire.endTransmission();  
  Wire.requestFrom(CHARGE_CONTROLLER, 1); //get wd status. 
  uint8_t result = Wire.read();
  result |= 1 << 6;
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG1));
  Wire.write(result);
  Wire.endTransmission();  
 
  //SetCurrent(EEPROM.read(CHG_ADDR+CHG_REG00));

  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG0)); 
  Wire.endTransmission();  
  Wire.requestFrom(CHARGE_CONTROLLER, 1); //get wd status. 
  result = Wire.read();
  result &= ~(1 << 7);
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG0));
  Wire.write(result);
  Wire.endTransmission();  
  
  
}

void ChargeInputLimiter()
{ 
  //detect if we are in a limited charge state
  //Done by checking charge status register. 
  if ( !(ChargeControllerStatus() & _BV(7)) && (ChargeControllerStatus() & _BV(6)) ) // USB host
  {
    ClearHIZ();
    SetCurrent(5);// USB host OR Adapter port - doing nothing for now because BQ24190 seems to do OK with these situations
  }
  
  else if ( !(ChargeControllerStatus() & _BV(7)) && !(ChargeControllerStatus() & _BV(6)) )
  {
    //Unknown adapter, need to find charging rate
    if ((ChargeControllerStatus() && _BV(2)) && chargerate < 0x07) // Power is good but not charging fast
    {
      chargerate++;
    }

  }
  else
  {
      ForceInputDetection();
  }
}

void ForceInputDetection()
{
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG7)); 
  Wire.endTransmission();  
  Wire.requestFrom(CHARGE_CONTROLLER, 1); //get wd status. 
  uint8_t result = Wire.read();
  result |= 1 << 7;
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG7));  
  Wire.write(result);
  Wire.endTransmission();  
}

void ClearHIZ()
{
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG0)); 
  Wire.endTransmission();  
  Wire.requestFrom(CHARGE_CONTROLLER, 1); //get wd status. 
  uint8_t result = Wire.read();
  result &= 0x7F; //clear HIZ bit
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG0));  
  Wire.write(result);
  Wire.endTransmission();  
}

uint8_t ChargeControllerInputStatus()
{ 
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG0));
  Wire.endTransmission();
  
  Wire.requestFrom(CHARGE_CONTROLLER, 1);
  uint8_t result = Wire.read();
  
  return result;
}

uint8_t ChargeControllerCurrentStatus()
{ 
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG2));
  Wire.endTransmission();
  
  Wire.requestFrom(CHARGE_CONTROLLER, 1);
  uint8_t result = Wire.read();
  
  return result;
}

uint8_t ChargeControllerStatus()
{ 
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG8));
  Wire.endTransmission();
  
  Wire.requestFrom(CHARGE_CONTROLLER, 1);
  uint8_t result = Wire.read();
  
  return result;
}

uint8_t ChargeControllerFaults()
{
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG9));
  Wire.endTransmission();
  
  Wire.requestFrom(CHARGE_CONTROLLER, 1);
  uint8_t result = 0;
  if(Wire.available())
    result = Wire.read();
  
  return result;
}

uint8_t ChargeControllerRegister(uint8_t r) // read from ChargeController
{
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write((byte)r);
  Wire.endTransmission();
  
  Wire.requestFrom(CHARGE_CONTROLLER, 1);
  uint8_t result = 0;
  if(Wire.available())
    result = Wire.read();
  
  return result;
}

void SetChargeControllerRegister(uint8_t reg, uint8_t value)
{ 
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write((byte)reg); 
  Wire.endTransmission();  
  Wire.requestFrom(CHARGE_CONTROLLER, 1); 

  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write((byte)reg);  // 
  Wire.write((byte)value);
  Wire.endTransmission();
}

void SetCurrent(uint8_t value)
{
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG0)); 
  Wire.endTransmission();  // should end Transmission come AFTER requestFrom??
  Wire.requestFrom(CHARGE_CONTROLLER, 1); //get wd status. 
  uint8_t result = Wire.read();
  result |= (value & 0x07);
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG0));  // 

  Wire.write(result);
  Wire.endTransmission();
}

void SetVoltageIn(uint8_t value)
{
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG0)); 
  Wire.endTransmission();  // should end Transmission come AFTER requestFrom??
  Wire.requestFrom(CHARGE_CONTROLLER, 1); // confirm this reads the whole first byte 
  uint8_t result = Wire.read();
  result |= (value & 0x78); 
  Wire.beginTransmission(CHARGE_CONTROLLER);
  Wire.write(byte(CHRG_CMD_REG0));  // 
  Wire.write(result);
  Wire.endTransmission();
}



//Current Measurement INA219  ----------- THIS IS ALL MESSED UP AS IT WANTS 16 bit values and I dont know how to do that.
void SetupCurrentMeasurement()
{
  Wire.beginTransmission(CURRENT_MONITOR);
  Wire.write(byte(0x00));
  Wire.write(byte(0b00000000)); //defaults. needs configing
  Wire.write(byte(0b01000111)); //defaults. needs configing
  Wire.endTransmission();
  
  Wire.beginTransmission(CURRENT_MONITOR);
  Wire.write(byte(0x05));
  Wire.write(byte(0x6A)); //defaults. needs configing
  Wire.write(byte(0xAA)); //defaults. needs configing
  Wire.endTransmission();
}

void SleepCurrentMeasurement()
{
  Wire.beginTransmission(CURRENT_MONITOR);
  Wire.write(byte(0x00));
  Wire.write(byte(0b00000000)); //defaults. needs configing
  Wire.write(byte(0b01000000)); //defaults. needs configing
  Wire.endTransmission(); 
}

uint16_t ReadCurrent()
{
  //0.0001 A
  Wire.beginTransmission(CURRENT_MONITOR);
  Wire.write(byte(0x04));
  Wire.endTransmission();
  delayMicroseconds(4);
  Wire.requestFrom(CURRENT_MONITOR, 2);
  uint16_t result = 0;
  if(Wire.available())
  {
    result = Wire.read();    
    result = (result << 8) | Wire.read();
  }
  return result * 0.1;  //LSB = 0.1mA
}

uint16_t ReadBusVoltage()
{
  Wire.beginTransmission(CURRENT_MONITOR);
  Wire.write(byte(0x02));
  Wire.endTransmission();
  delayMicroseconds(4);
  
  Wire.requestFrom(CURRENT_MONITOR, 2);
  uint16_t result = 0;
  if(Wire.available())
  {
    result = Wire.read();    
    result = (result << 8) | Wire.read();
    result = result >> 3;
  }
  return result * 4; //LSB 4mV 
}

uint16_t ReadPower()
{
  //0.002 W
  Wire.beginTransmission(CURRENT_MONITOR);
  Wire.write(byte(0x03));
  Wire.endTransmission();
  delayMicroseconds(4);
  
  Wire.requestFrom(CURRENT_MONITOR, 2);
  uint16_t result = 0;
  if(Wire.available())
  {
    result = Wire.read();    
    result = (result << 8) | Wire.read();
  }

  return result * 0.2; //LSB 2mW  
}

uint16_t ReadShunt()
{
  //0.002 W
  Wire.beginTransmission(CURRENT_MONITOR);
  Wire.write(byte(0x01));
  Wire.endTransmission();
  delayMicroseconds(4);
  
  Wire.requestFrom(CURRENT_MONITOR, 2);
  uint16_t result = 0;
  if(Wire.available())
  {
    result = Wire.read();    
    result = (result << 8) | Wire.read();
  }

  return result & 0x0FFF; //LSB 0.01mV  
}
/* Battery Fuel Gauge */
void SetupFuelGauge()
{
  Wire.beginTransmission(FUEL_GAUGE);
  Wire.write(FUEL_CMD_REG2);
  Wire.write(0x97);
  Wire.write(0b00011100);  //4% Threshold
  Wire.endTransmission();
}

uint16_t FuelGaugeStatus()
{
  Wire.beginTransmission(FUEL_GAUGE);
  Wire.write(FUEL_CMD_REG3);
  Wire.endTransmission();
  
  Wire.requestFrom(FUEL_GAUGE, 2);
  uint16_t resultH = Wire.read();
  uint8_t resultL = Wire.read();
  
  uint16_t result = (resultH << 8) | resultL;
  return result >> 4;  //12bit Value  
}

uint16_t ReadVoltage()
{
  Wire.beginTransmission(FUEL_GAUGE);
  Wire.write(FUEL_CMD_REG0);
  Wire.endTransmission();
  
  Wire.requestFrom(FUEL_GAUGE, 2);
  uint16_t resultH = Wire.read();
  uint8_t resultL = Wire.read();
  
  uint16_t result = (resultH << 8) | resultL;
  return (result >> 4) * 1.25;  //12bit Value  
}

uint16_t ReadSOC()
{
  Wire.beginTransmission(FUEL_GAUGE);
  Wire.write(FUEL_CMD_REG1);
  Wire.endTransmission();
  
  Wire.requestFrom(FUEL_GAUGE, 2);
  uint16_t resultH = Wire.read();
  uint8_t resultL = Wire.read();
  
  uint16_t result = (resultH << 8) | resultL;
  return result;  //16bit Value  
}

void SleepFuelGauge()
{
  Wire.beginTransmission(FUEL_GAUGE);
  Wire.write(FUEL_CMD_REG2);
  Wire.endTransmission();

  Wire.requestFrom(FUEL_GAUGE, 2);
  uint16_t resultH = Wire.read();
  uint8_t resultL = Wire.read();
  
  Wire.beginTransmission(FUEL_GAUGE);
  Wire.write(FUEL_CMD_REG2);
  Wire.write(resultH);
  Wire.write(resultL | 0b10000000);
  Wire.endTransmission();
}

void WakeFuelGauge()
{
  Wire.beginTransmission(FUEL_GAUGE);
  Wire.write(FUEL_CMD_REG2);
  Wire.endTransmission();

  Wire.requestFrom(FUEL_GAUGE, 2);
  uint16_t resultH = Wire.read();
  uint8_t resultL = Wire.read();
  
  Wire.beginTransmission(FUEL_GAUGE);
  Wire.write(FUEL_CMD_REG2);
  Wire.write(resultH);
  Wire.write(resultL | ~(0b10000000));
  Wire.endTransmission();
}

uint8_t FuelGaugeAlert()
{
  Wire.beginTransmission(FUEL_GAUGE);
  Wire.write(FUEL_CMD_REG2);
  Wire.endTransmission();

  Wire.requestFrom(FUEL_GAUGE, 2);
  uint16_t resultH = Wire.read();
  uint8_t resultL = Wire.read();
  
  return resultL & 0b00100000;
}

/* RTC */

void SetupRTC()
{
  Wire.beginTransmission(RTC_CLOCK);
  Wire.write(RTC_CMD_REG1); // reset register pointer  
  Wire.write(0x00);   
  Wire.endTransmission();
  
}

void WriteRTCTime(time_t t)
{
  tmElements_t tm;
  breakTime(t, tm);

  Wire.beginTransmission(RTC_CLOCK);
  Wire.write(RTC_CMD_REG2); // reset register pointer  
  Wire.write(DEC2BCD(tm.Second)) ;   
  Wire.write(DEC2BCD(tm.Minute));
  Wire.write(DEC2BCD(tm.Hour));      // sets 24 hour format
  Wire.write(DEC2BCD(tm.Day));
  Wire.write(DEC2BCD(tm.Wday));   
  Wire.write(DEC2BCD(tm.Month));
  Wire.write(DEC2BCD(tmYearToY2k(tm.Year))); 
  Wire.endTransmission();
  
}

time_t ReadRTCTime()
{
  tmElements_t tm;
  
  Wire.beginTransmission(RTC_CLOCK);
  Wire.write(RTC_CMD_REG2); // reset register pointer  
  Wire.endTransmission();
  
  Wire.requestFrom(RTC_CLOCK, 7);
  
  byte s = Wire.read() & 0x7f;
  tm.Second = BCD2DEC(s);
  byte m = Wire.read() & 0x7f;
  tm.Minute = BCD2DEC(m);
  byte h = Wire.read() & 0x3f;
  tm.Hour = BCD2DEC(h);
  byte d = Wire.read() & 0x3f;
  tm.Day = BCD2DEC(d);
  byte wd = Wire.read() & 0x07;
  tm.Wday = BCD2DEC(wd);
  byte mth = Wire.read() & 0x1f;
  tm.Month = BCD2DEC(mth);
  byte y = Wire.read();
  tm.Year = y2kYearToTm(BCD2DEC(y));
 
  return makeTime(tm);
}

void SetRTCTimer(uint8_t minutes)
{
  Wire.beginTransmission(RTC_CLOCK);
  Wire.write(RTC_CMD_REG1); // reset register pointer  
  Wire.write(0b00000011);   
  Wire.endTransmission();
  
  Wire.beginTransmission(RTC_CLOCK);
  Wire.write(RTC_CMD_REG15); // reset register pointer  
  Wire.write(minutes);   
  Wire.endTransmission();

  Wire.beginTransmission(RTC_CLOCK);
  Wire.write(RTC_CMD_REG14); // reset register pointer  
  Wire.write(0b10000011);   //Timer Enabled for 1 minute counter 11 is min 10 is seconds
  Wire.endTransmission();
}

void ClearRTCTimer()
{
  Wire.beginTransmission(RTC_CLOCK);
  Wire.write(RTC_CMD_REG1); // reset register pointer  
  Wire.write(0b00000000);   
  Wire.endTransmission();
}

/* LED Controller */

void SleepLED()
{
  Wire.beginTransmission(LED_CONTROLLER);
  Wire.write(LED_CMD_REG_MODE1);
  Wire.write(0b00110001);
  Wire.write(0b00010000);
  Wire.endTransmission();
}

void WakeUpLED()
{
  Wire.beginTransmission(LED_CONTROLLER);
  Wire.write(LED_CMD_REG_MODE1);
  Wire.write(0b10100001);
  Wire.write(0b00010000);
  Wire.endTransmission();
}
