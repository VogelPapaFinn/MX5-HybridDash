#pragma once

// Project includes
#include "Sensor.hpp"

// espidf includes
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

class PassiveSensor : public Sensor
{
public:
	PassiveSensor(SENSOR::TYPE type, gpio_num_t gpio, adc_channel_t adcChannel, adc_oneshot_unit_handle_t p_adc, adc_unit_t unit = ADC_UNIT_2);

	void read();

	int get() override;

	double getResistance() const;

protected:
	/*
	 *	Private Functions
	 */
	virtual void specificRead();

	static double calcVoltageDividerR2(int voltageMv, int r1);

	void notifyAboutNewValue() override;

	/*
	 *	Private Variables
	 */
	bool setup_ = false;

	double resistance_ = 0.0;

	int lastVoltage_ = 0;

	int voltage_ = 0;

	adc_oneshot_unit_handle_t adc_ = nullptr;

	adc_unit_t unit_;

	adc_channel_t channel_ = ADC_CHANNEL_0;

	adc_oneshot_chan_cfg_t channelConfig_ = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT};

	adc_cali_handle_t calibrationHandle_;

	gpio_num_t gpio_ = GPIO_NUM_NC;
};
