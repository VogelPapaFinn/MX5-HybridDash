/* --- Includes --- */
#include "SensorManager/SensorManager.h"

/* --- Private Defines & Macros --- */

/* --- Private Variables, Typedefs etc. --- */
// ADC stuff
static adc_oneshot_unit_handle_t adc2Handle_;
static adc_oneshot_unit_init_cfg_t adc2InitConfig_;
static bool initAdc2Failed_ = false;

// Oil pressure stuff
static bool oilPressure_ = false;
static adc_cali_handle_t adc2OilCaliHandle_;
static bool initAdc2OilChannelFailed_ = false;

// Fuel level stuff
static int fuelLevelInPercent_ = 0;
static float fuelLevelResistance_ = 0.0f;
static adc_cali_handle_t adc2FuelCaliHandle_;
static bool initAdc2FuelChannelFailed_ = false;

// Water temperature stuff
static float waterTemperature_ = 0.0f;
static float waterTemperatureResistance_ = 0.0f;
static adc_cali_handle_t adc2WaterCaliHandle_;
static bool initAdc2WaterChannelFailed_ = false;

// Temporary stuff so I don't forget anything to implement
int speed_ = -1;
int rpm_ = -1;
int tempSensor1_ = -1;
int tempSensor2_ = -1;

//! \brief Calculates the resistance of a voltage divider
//! \param preR1VoltageV The original voltage the divider works with [in Volts] e.g. 3.3V
//! \param voltageMV The measured voltage between R1 and R2 [in Millivolts]
//! \param r1 The resistance of R1
//! \retval The calculated resistance of R2 as float
float calculateVoltageDividerR2(const float preR1VoltageV, const int voltageMV, const int r1) {
    const float vIn = preR1VoltageV;
    const float vOut = (float) voltageMV / 1000.0f;
    const float r = (float) r1;

    // R2 = R1 * (voltageMV / preR1VoltageV - voltageMV)
    return r * (vOut / (vIn - vOut));
}

//! \brief Calculates the fuel level in PERCENT from the measured R2 resistance.
//! It uses a non-linear function as the fuel level sensor output is not proportional
//! to the fuel level.
//! \retval The fuel level in PERCENT as int
//! TODO: Implement the non-linear function!
int calculateFuelLevelFromResistance() {
    float resistance = fuelLevelResistance_;

    // Remove the resistance offset
    resistance -= FUEL_LEVEL_OFFSET;

    // Check if its < 0
    if (resistance < 0.0f) resistance = 0.0f;

    // Check if its > FUEL_LEVEL_TO_PERCENTAGE - FUEL_LEVEL_OFFSET
    if (resistance > FUEL_LEVEL_TO_PERCENTAGE - FUEL_LEVEL_OFFSET) resistance = FUEL_LEVEL_TO_PERCENTAGE - FUEL_LEVEL_OFFSET;

    // Then convert it to percent and return it
    return (int) ((resistance / FUEL_LEVEL_TO_PERCENTAGE) * 100.0f);
}

//! \brief Calculates the water temperature in degree Celsius from the measured R2 resistance.
//! It uses a non-linear function as the water temp sensor output is not proportional
//! to the water temperature.
//! \retval The water temperature in degree Celsius as float
//! TODO: Implement the non-linear function!
float calculateWaterTemperatureFromResistance() {
    float resistance = waterTemperatureResistance_;

    // Remove the resistance offset
    resistance -= FUEL_LEVEL_OFFSET;

    // Check if its < 0
    if (resistance < 0.0f) resistance = 0.0f;

    // Then convert it to percent and return it
    return (resistance / FUEL_LEVEL_TO_PERCENTAGE * 100.0f);
}

/* --- Function implementations --- */
int sensorManagerInit(void) {
    // Initialize the ADC2
    const adc_oneshot_unit_init_cfg_t adc2InitConfig = {
            .unit_id = ADC_UNIT_2,
            .ulp_mode = ADC_ULP_MODE_DISABLE};
    if (adc_oneshot_new_unit(&adc2InitConfig, &adc2Handle_) != ESP_OK) {
        // Init was NOT successful!
        initAdc2Failed_ = true;

        // Logging
        loggerWarn("Failed to initialize ADC2!");

        // Initialization failed
        return 0;
    }

    // Configure oil pressure GPIO
    gpio_set_direction(GPIO_OIL_PRESSURE, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_OIL_PRESSURE, GPIO_PULLDOWN_ONLY);

    /* --- Configure the oil pressure ADC2 channel --- */

    // Create the config
    const adc_oneshot_chan_cfg_t adc2OilConfig = {
            .bitwidth = ADC_BITWIDTH_12,
            .atten = ADC_ATTEN_DB_2_5,
    };
    initAdc2OilChannelFailed_ = adc_oneshot_config_channel(adc2Handle_, ADC_CHANNEL_OIL_PRESSURE, &adc2OilConfig) != ESP_OK;

    // Logging
    if (initAdc2OilChannelFailed_) {
        loggerError("Failed to initialize ADC2 channel: %d", ADC_CHANNEL_OIL_PRESSURE);
    }

    // Create the calibration curve config
    const adc_cali_curve_fitting_config_t oilCaliConfig = {
            .unit_id = ADC_UNIT_2,
            .chan = ADC_CHANNEL_OIL_PRESSURE,
            .atten = ADC_ATTEN_DB_2_5,
            .bitwidth = ADC_BITWIDTH_12,
    };

    // Create calibration curve fitting
    if (adc_cali_create_scheme_curve_fitting(&oilCaliConfig, &adc2OilCaliHandle_) != ESP_OK) {
        // Mark the oil channel as failed
        initAdc2OilChannelFailed_ = true;

        // Logging
        loggerError("'adc_cali_create_scheme_curve_fitting' for the oil pressure channel FAILED");
    }

    /* --- Configure the oil pressure ADC2 channel --- */

    /* --- Configure the fuel level ADC2 channel --- */

    // Configure oil pressure GPIO
    gpio_set_direction(GPIO_FUEL_LEVEL, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_FUEL_LEVEL, GPIO_PULLDOWN_ONLY);

    // Create the config
    const adc_oneshot_chan_cfg_t adc2FuelConfig = {
            .bitwidth = ADC_BITWIDTH_12,
            .atten = ADC_ATTEN_DB_2_5,
    };
    initAdc2FuelChannelFailed_ = adc_oneshot_config_channel(adc2Handle_, ADC_CHANNEL_FUEL_LEVEL, &adc2FuelConfig) != ESP_OK;

    // Logging
    if (initAdc2FuelChannelFailed_) {
        loggerError("Failed to initialize ADC2 channel: %d", ADC_CHANNEL_FUEL_LEVEL);
    }

    // Create the calibration curve config
    const adc_cali_curve_fitting_config_t fuelCaliConfig = {
            .unit_id = ADC_UNIT_2,
            .chan = ADC_CHANNEL_FUEL_LEVEL,
            .atten = ADC_ATTEN_DB_2_5,
            .bitwidth = ADC_BITWIDTH_12,
    };

    // Create calibration curve fitting
    if (adc_cali_create_scheme_curve_fitting(&fuelCaliConfig, &adc2FuelCaliHandle_) != ESP_OK) {
        // Mark the fuel channel as failed
        initAdc2FuelChannelFailed_ = true;

        // Logging
        loggerError("'adc_cali_create_scheme_curve_fitting' for the fuel level channel FAILED");
    }

    /* --- Configure the fuel level ADC2 channel --- */

    /* --- Configure the water temperature ADC2 channel --- */

    // Configure oil pressure GPIO
    gpio_set_direction(GPIO_WATER_TEMPERATURE, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_WATER_TEMPERATURE, GPIO_PULLDOWN_ONLY);

    // Create the config
    const adc_oneshot_chan_cfg_t adc2WaterConfig = {
            .bitwidth = ADC_BITWIDTH_12,
            .atten = ADC_ATTEN_DB_12,
    };
    initAdc2WaterChannelFailed_ = adc_oneshot_config_channel(adc2Handle_, ADC_CHANNEL_WATER_TEMPERATURE, &adc2WaterConfig) != ESP_OK;

    // Logging
    if (initAdc2WaterChannelFailed_) {
        loggerError("Failed to initialize ADC2 channel: %d", ADC_CHANNEL_WATER_TEMPERATURE);
    }

    // Create the calibration curve config
    const adc_cali_curve_fitting_config_t waterCaliConfig = {
            .unit_id = ADC_UNIT_2,
            .chan = ADC_CHANNEL_WATER_TEMPERATURE,
            .atten = ADC_ATTEN_DB_12,
            .bitwidth = ADC_BITWIDTH_12,
    };

    // Create calibration curve fitting
    if (adc_cali_create_scheme_curve_fitting(&waterCaliConfig, &adc2WaterCaliHandle_) != ESP_OK) {
        // Mark the water channel as failed
        initAdc2WaterChannelFailed_ = true;

        // Logging
        loggerError("'adc_cali_create_scheme_curve_fitting' for the water temperature channel FAILED");
    }

    /* --- Configure the water temperature ADC2 channel --- */

    // Return result
    if (initAdc2OilChannelFailed_ || initAdc2FuelChannelFailed_ || initAdc2WaterChannelFailed_) return 2;// Initialization succeeded with errors
    return 1;                                                                                            // Initialization succeeded
}

void sensorManagerUpdateOilPressure(void) {
    // Was the init successfully?
    if (initAdc2Failed_ || initAdc2OilChannelFailed_) return;

    // Temporary containers
    int rawAdcValue = 0;
    int voltage = 0;

    // Try to read from the ADC
    if (adc_oneshot_read(adc2Handle_, ADC_CHANNEL_OIL_PRESSURE, &rawAdcValue) != ESP_OK) {
        // Log that it failed
        loggerWarn("Failed to read the oil pressure from the ADC!");
    }

    // Try to convert the ADC value to a voltage
    if (adc_cali_raw_to_voltage(adc2OilCaliHandle_, rawAdcValue, &voltage) != ESP_OK) {
        // Log that it failed
        loggerWarn("Failed to calculate the voltage from the ADC value!");
    }

    // Check the thresholds
    const bool oldOilPressureValue = oilPressure_;
    oilPressure_ = voltage > OIL_LOWER_VOLTAGE_THRESHOLD && voltage < OIL_UPPER_VOLTAGE_THRESHOLD;

    // Did it change?
    if (oldOilPressureValue != oilPressure_) {
        // Logging
        loggerInfo("Oil pressure changed! From: '%d' to '%d'", oldOilPressureValue, oilPressure_);
    }
}

bool sensorManagerHasOilPressure(void) {
    return oilPressure_;
}

void sensorManagerUpdateFuelLevel(void) {
    // Was the init successfully?
    if (initAdc2Failed_ || initAdc2FuelChannelFailed_) return;

    // Temporary containers
    int rawAdcValue = 0;
    int voltage = 0;

    // Try to read from the ADC
    if (adc_oneshot_read(adc2Handle_, ADC_CHANNEL_FUEL_LEVEL, &rawAdcValue) != ESP_OK) {
        // Log that it failed
        loggerWarn("Failed to read the fuel level from the ADC!");
    }

    // Try to convert the ADC value to a voltage
    if (adc_cali_raw_to_voltage(adc2FuelCaliHandle_, rawAdcValue, &voltage) != ESP_OK) {
        // Log that it failed
        loggerWarn("Failed to calculate the voltage from the ADC value!");
    }

    // Calculate resistance
    fuelLevelResistance_ = calculateVoltageDividerR2(OIL_FUEL_WATER_VOLTAGE_V, voltage, OIL_FUEL_R1);

    // Calculate the fuel level from the calculated resistance
    const int oldFuelLevelValue = fuelLevelInPercent_;
    fuelLevelInPercent_ = calculateFuelLevelFromResistance();
    printf("adc: %d - voltage: %d - resistance: %f - level: %d\n", rawAdcValue, voltage, fuelLevelResistance_, fuelLevelInPercent_);

    // Did it change?
    if (oldFuelLevelValue != fuelLevelInPercent_) {
        // Logging
        loggerInfo("Fuel level changed! From: '%d percent' to '%d percent'", oldFuelLevelValue, fuelLevelInPercent_);
    }
}

int sensorManagerGetFuelLevel(void) {
    return fuelLevelInPercent_;
}

void sensorManagerUpdateWaterTemperature(void) {
    // Was the init successfully?
    if (initAdc2Failed_ || initAdc2WaterChannelFailed_) return;

    // Temporary containers
    int rawAdcValue = 0;
    int voltage = 0;

    // Try to read from the ADC
    if (adc_oneshot_read(adc2Handle_, ADC_CHANNEL_WATER_TEMPERATURE, &rawAdcValue) != ESP_OK) {
        // Log that it failed
        loggerWarn("Failed to read the water temperature from the ADC!");
    }

    // Try to convert the ADC value to a voltage
    if (adc_cali_raw_to_voltage(adc2WaterCaliHandle_, rawAdcValue, &voltage) != ESP_OK) {
        // Log that it failed
        loggerWarn("Failed to calculate the voltage from the ADC value!");
    }

    // Calculate resistance
    waterTemperatureResistance_ = calculateVoltageDividerR2(OIL_FUEL_WATER_VOLTAGE_V, voltage, WATER_R1);

    // Calculate the water temperature from the calculated resistance
    const float oldWaterTemperatureValue = waterTemperature_;
    //waterTemperature_ = calculateWaterTemperatureFromResistance();

    // Did it change?
    if (oldWaterTemperatureValue != waterTemperature_) {
        // Logging
        loggerInfo("Water temperature changed! From: '%d °C' to '%d °C'", oldWaterTemperatureValue, waterTemperature_);
    }
}

float sensorManagerGetWaterTemperature(void) {
    return waterTemperature_;
}