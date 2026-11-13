#pragma once

// espidf includes
#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

class PassiveSensor
{
public:
	PassiveSensor(gpio_num_t gpio, adc_channel_t adcChannel, adc_oneshot_unit_handle_t p_adc,
				  adc_atten_t atten = ADC_ATTEN_DB_12, adc_unit_t unit = ADC_UNIT_1);

	void read();

	virtual int get() = 0;

	float getResistance();

	int getVoltageMv();

protected:
	/*
	 *	Private Functions
	 */
	virtual void specificRead() = 0;

	static float calcVoltageDividerR2(int voltageMv, int r1);

	int readSample(const unsigned int sampleSize);

	int convertSampleToVoltage(const int sample);

	/*
	 *	Private Variables
	 */
	bool setup_ = false;

	int voltage_ = 0;
	float resistance_ = 0;

	adc_oneshot_unit_handle_t adc_ = nullptr;

	adc_unit_t unit_;

	adc_channel_t channel_ = ADC_CHANNEL_0;

	adc_oneshot_chan_cfg_t channelConfig_ = {.atten = ADC_ATTEN_DB_12, .bitwidth = ADC_BITWIDTH_DEFAULT};

	adc_cali_handle_t calibrationHandle_;

	gpio_num_t gpio_ = GPIO_NUM_NC;
};
