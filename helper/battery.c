/* Copyright 2023 Dual Tachyon
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#include "battery.h"
#include "driver/backlight.h"
#include "misc.h"
#include "settings.h"
#include "ui/battery.h"
#include "ui/menu.h"
#include "ui/ui.h"

uint16_t          gBatteryCalibration[6];
uint16_t          gBatteryCurrentVoltage;
uint16_t          gBatteryCurrent;
uint16_t          gBatteryVoltages[4];
uint16_t          gBatteryVoltageAverage;
uint8_t           gBatteryDisplayLevel;
bool              gChargingWithTypeC;
bool              gLowBattery;
bool              gLowBatteryBlink;
uint16_t          gBatteryCheckCounter;

volatile uint16_t gPowerSave_10ms;


unsigned int BATTERY_VoltsToPercent(const unsigned int voltage_10mV)
{
	const uint16_t crv1600[][2] = {
		{814, 100},
		{756, 24 },
		{729, 7 },
		{597, 0 },
		{0,   0}
	};

	const uint16_t crv2200[][2] = {
		{823, 100},
		{740, 60},
		{707, 21},
		{680, 5},
		{505, 0},
		{0,   0}
	};
	
	const BATTERY_Type_t type = gEeprom.BATTERY_TYPE;
	const uint16_t(*crv)[2];
	uint8_t size;
	if (type == BATTERY_TYPE_2200_MAH) {
		crv = crv2200;
		size = ARRAY_SIZE(crv2200);
	}	
	else {
		crv = crv1600;
		size = ARRAY_SIZE(crv1600);
	}

	const int mulipl = 1000;
	for (int i = 1; i < size; i++) {
		if (voltage_10mV > crv[i][0]) {
			int a = (crv[i - 1][1] - crv[i][1]) * mulipl / (crv[i - 1][0] - crv[i][0]);
			int b = crv[i][1] - a * crv[i][0] / mulipl;
			int p = a * voltage_10mV / mulipl + b;
			return MIN(p, 100);
		}
	}
	return 0;
}

void BATTERY_GetReadings(const bool bDisplayBatteryLevel)
{
	const uint8_t  PreviousBatteryLevel = gBatteryDisplayLevel;
	const uint16_t Voltage              = (gBatteryVoltages[0] + gBatteryVoltages[1] + gBatteryVoltages[2] + gBatteryVoltages[3]) / 4;

	gBatteryDisplayLevel = 0;

	for(uint8_t i = 6; i > 0; i--){
		if (Voltage > gBatteryCalibration[i-1]) {
			gBatteryDisplayLevel = i;
			break;
		}
	}

	gBatteryVoltageAverage = (Voltage * 760) / gBatteryCalibration[3];

	if ((gScreenToDisplay == DISPLAY_MENU) && GetCurrentMenuId() == MENU_VOL)
		gUpdateDisplay = true;

	if (gBatteryCurrent < 501)
	{
		if (gChargingWithTypeC)
		{
			gUpdateStatus  = true;
			gUpdateDisplay = true;
		}

		gChargingWithTypeC = false;
	}
	else
	{
		if (!gChargingWithTypeC)
		{
			gUpdateStatus  = true;
			gUpdateDisplay = true;
			BACKLIGHT_TurnOn();
		}

		gChargingWithTypeC = true;
	}

	if (PreviousBatteryLevel != gBatteryDisplayLevel)
	{
		if (gBatteryDisplayLevel < 2)
		{
			gLowBattery = true;
		}
		else
		{
			gLowBattery = false;

			if (bDisplayBatteryLevel)
				UI_DisplayBattery(gBatteryDisplayLevel, gLowBatteryBlink);
		}

		gLowBatteryCountdown = 0;
	}
}
