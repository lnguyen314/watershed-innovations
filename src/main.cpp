/******************************************************************************
 * @file    main.cpp
 * @author  Dua Nguyen
 * @brief     Functional testing: reading current value, voltage value, power value from INA module
 * time update, display content on screen, press button to go next screen, reSet timer.
 * TODO: long description
 * @date     Oct. 2017
 * @date modified 2017/11/29
 * @version 1.0.0
 * Copyright(C) 2017
 * All rights reserved.
 */
 /*****************************************************************************
 * INA219 module to measure battery and PV basic functionality including initialization, Calibration,
 * reading current value, voltage value, power value and energy in period time

 Hardware setup:
 INA219 for battery and PV
 INA219 --------- Nucleo L432 board
 3.3V --------------------- 3.3V
 SDA ----------------------- I2C_SDA (PB_7)
 SCL ----------------------- I2C_SCL (PB_6)
 GND ---------------------- GND
 Note: using 2 INA219 modules for measuring battery and PV. so user have to set different address
 from 0x40 to 0x4f. in this project, we set 0x40 for battery and 0x41 for PV
 */
 /*****************************************************************************
 * LCD Adafruit_oled 128x64 basic functionality including initialization, show a specific logo,
 * show information about current, voltage, power and energy value.
 Hardware setup:
 LCD Adafruit_oled 128x64
 LCD Adafruit_oled 128x64 --------- Nucleo L432 board
 3.3V --------------------- 3.3V
 SDA ----------------------- I2C_SDA (PB_7)
 SCL ----------------------- I2C_SCL (PB_6)
 GND ---------------------- GND
 note: LCD I2C address : 0x78
 */
 /*****************************************************************************
 * KeyboardController functionality including changing the status of system such as
   changing menu screean, starting timer, stopping timer, selecting inverter mode
   Hardware setup:
   SELECT_BUTTON_PIN ------------PB_0
   SET_BUTTON_PIN ---------------PB_1
   INVERTER_ON_PIN --------------PB_3
  */
 /*****************************************************************************
  * RTCtimer functionality including count a period time to calculate energy of battery
    and PV
 *****************************************************************************/
#define MBED_CONF_FILESYSTEM_PRESENT 1
#include <IOPins.h>
#include <mbed.h>
#include <rtos.h>
#include <LCDController.h>
#include <INAReader.h>
#include <RTCTimer.h>
#include <Button.h>
#include <EventHandling.h>
#include <EnergyStorage.h>
#include <PowerOnSelfTest.h>
#include <MCP23008.hpp>
#ifndef UNIT_TEST
#define SHUNT_RES_VALUE 0.016
#define MAX_CURRENT_VALUE 3.2
#define MAX_VOLTAGE_VALUE 16
#define TEST_MCP_OUTPUT_VALUE 0xF5
/*initialization lcd object*/
I2CPreInit i2c_object(I2C_SDA, I2C_SCL);
LCDController lcdcontroller(i2c_object);

/*initialization current, voltage measuring object*/
INAReader battery_measurement(I2C_SDA, I2C_SCL, 0x40);

/*initialization keyboard object*/
Button selecting(SELECT_BUTTON_PIN);
Button setting(SET_BUTTON_PIN);
Button enable_inverter(INVERTER_ON_PIN);

/*initialization event handling object*/
EventHandling event_handling;

/*initialization realtime clock object */
RTC_Timer rtc_timer;
/*initialization energy storage object*/
EnergyStorage energy_storage(SDIO_MOSI, SDIO_MISO, SDIO_SCK, SDIO_CS);
DigitalOut led(PB_5);
MCP23008 expander(I2C_SDA, I2C_SCL, 0);
PowerOnSelfTest POST;
DigitalOut led_error(PB_4);
int main() {
    led_error = 1;
    lcdcontroller.PostDisplay(POST.POST_INA219(battery_measurement.PowerOnSelfTest()));
    expander.set_output_pins(expander.Pin_All);
    expander.write_outputs(TEST_MCP_OUTPUT_VALUE);
    lcdcontroller.PostDisplay(POST.POST_IOExpander(TEST_MCP_OUTPUT_VALUE == expander.read_outputs()));
    while (false == POST.GetResult()) {
        led_error = !led_error;
        wait(0.5);
    }
    wait(0.5);
    /*Display logo watershed on screen*/
    lcdcontroller.ShowLogo();
    /*calibrate ina219 with Shunt resistor value, max current value, max voltage value*/
    battery_measurement.Calibrate(SHUNT_RES_VALUE, MAX_CURRENT_VALUE, MAX_VOLTAGE_VALUE);
    /*verify/read the latest energy stored value.*/
    energy_storage.ReloadEnergy();
    /*Wait for 3 seconds*/
    wait(3);
    energy_storage.Init();
    while (true) {
        /*Reading values from INA module*/
        battery_measurement.Scan();
        /*updating realtime clock*/
        rtc_timer.Update();
        /*updating real power for energy calculation*/
        energy_storage.UpdatePower(battery_measurement.GetPower()/1000);
        /*Scan triggers*/
        event_handling.SwitchMenuTrigger(selecting.GetShortPress());
        event_handling.TimerIsOnTrigger(setting.GetShortPress());
        event_handling.TimerResetTrigger(setting.GetLongPress());
        event_handling.InverterTurnOnTrigger(enable_inverter.GetShortPress());
        /*handling events*/
        if (event_handling.GetTimerIsOn()) {
            rtc_timer.On();
        } else {
            rtc_timer.Off();
        }
        if (event_handling.GetTimerReset()) {
            rtc_timer.Reset();
        } else {
          /* do nothing*/
        }
        /*updating properties of lcd object */
        lcdcontroller.SetBattVolt(battery_measurement.GetVolt());
        lcdcontroller.SetBattCurr(battery_measurement.GetCurr());
        lcdcontroller.SetBattPower(battery_measurement.GetPower());
        lcdcontroller.SetBattEnergy(energy_storage.GetEnergy());
        lcdcontroller.SetTime(rtc_timer.GetHour(), rtc_timer.GetMinute(), rtc_timer.GetSecond());
        /*selecting screen to display*/
        lcdcontroller.UpdateScreen(event_handling.GetMenuIndex());
        led = !led;
    }
}
#endif /*UNIT_TEST*/
