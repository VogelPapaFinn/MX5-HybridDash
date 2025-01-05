/* --- Includes --- */
#include "SensorManager/SensorManager.h"

/* --- Private Defines & Macros --- */

/* --- Private Variables, Typedefs etc. --- */
// ADC stuff
static adc_oneshot_unit_handle_t adc2Handle_;
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

// Speed stuff
static int speedInHz_ = -1;
static int speed_ = -1;
static int64_t lastTimeOfFallingEdgeSpeed_ = 0;
static int64_t timeOfFallingEdgeSpeed_ = 0;
static bool speedIsrActive_ = false;
static bool initSpeedIsrFailed_ = false;

// RPM stuff
static int rpmInHz_ = -1;
static int rpm_ = -1;
static int64_t lastTimeOfFallingEdgeRPM_ = 0;
static int64_t timeOfFallingEdgeRPM_ = 0;
static bool rpmIsrActive_ = false;
static bool initRpmIsrFailed_ = false;

// Temporary stuff so I don't forget anything to implement
static int tempSensor1_ = -1;
static int tempSensor2_ = -1;

//! \brief Calculates the resistance of a voltage divider
//! \param preR1VoltageV The original voltage the divider works with [in Volts] e.g. 3.3V
//! \param voltageMV The measured voltage between R1 and R2 [in Millivolts]
//! \param r1 The resistance of R1
//! \retval The calculated resistance of R2 as float
float calculateVoltageDividerR2(const float preR1VoltageV, const int voltageMV, const int r1) {
    const float vIn = preR1VoltageV;
    const float vOut = (float) voltageMV / 1000.0f;
    const float r = (float) r1;

    // R2 = R1 * (voltageMV / (preR1VoltageV - voltageMV))
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

//! \brief Calculates the speed in kmh from the measured frequency.
//! \retval The speed in kmh
//! TODO: Implement actual conversion
float calculateSpeedFromFrequency() {
    return speedInHz_;
}

//! \brief Calculates the rpm from the measured frequency.
//! \retval The rpm's
int calculateRpmFromFrequency() {
    double multiplier = 0.0;

    // Get the multiplier
    if (rpmInHz_ <= 0)
        return -1;
    if (rpmInHz_ <= 8)
        multiplier = 50.0;
    else if (rpmInHz_ <= 11)
        multiplier = 45.45;
    else if (rpmInHz_ <= 17)
        multiplier = 41.18;
    else if (rpmInHz_ <= 25)
        multiplier = 40.0;
    else if (rpmInHz_ <= 56)
        multiplier = 34.48;
    else if (rpmInHz_ <= 92)
        multiplier = 32.61;
    else if (rpmInHz_ <= 123)
        multiplier = 32.52;
    else if (rpmInHz_ <= 157)
        multiplier = 31.85;
    else if (rpmInHz_ <= 188)
        multiplier = 31.91;
    else if (rpmInHz_ <= 220)
        multiplier = 31.82;
    else if (rpmInHz_ <= 262)
        multiplier = 30.54;

    // Calculate the rpm and return it
    return (int) ((double) rpmInHz_ * multiplier);
}

//! \brief ISR for the speed, triggered everytime there is a falling edge
static void IRAM_ATTR speedInterruptHandler() {
    lastTimeOfFallingEdgeSpeed_ = timeOfFallingEdgeSpeed_;
    timeOfFallingEdgeSpeed_ = esp_timer_get_time();
}

//! \brief ISR for the rpm, triggered everytime there is a falling edge
static void IRAM_ATTR rpmInterruptHandler() {
    lastTimeOfFallingEdgeRPM_ = timeOfFallingEdgeRPM_;
    timeOfFallingEdgeRPM_ = esp_timer_get_time();
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

    /* --- Configure the speed interrupt --- */

    // Setup gpio
    gpio_set_direction(GPIO_SPEED, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_SPEED, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(GPIO_SPEED, GPIO_INTR_NEGEDGE);

    // Install ISR service
    if (gpio_install_isr_service(ESP_INTR_FLAG_IRAM) != ESP_OK) {
        // Failed, so we cant install our speed/rpm ISR's
        initSpeedIsrFailed_ = true;
        initRpmIsrFailed_ = true;

        // Logging
        loggerError("Couldn't install the ISR service. Speed and RPM are unavailable!");
    }

    // Activate the ISR for measuring the frequency for the speed
    if (!initSpeedIsrFailed_ && sensorManagerEnableSpeedISR()) {
        // Everything worked
        speedIsrActive_ = true;
        initSpeedIsrFailed_ = false;
    } else {
        // It failed
        speedIsrActive_ = false;
        initSpeedIsrFailed_ = true;

        // Logging
        loggerError("Failed to enable the speed ISR!");
    }

    /* --- Configure the speed interrupt --- */

    /* --- Configure the rpm interrupt --- */

    // Setup gpio
    gpio_set_direction(GPIO_RPM, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_RPM, GPIO_PULLDOWN_ONLY);
    gpio_set_intr_type(GPIO_RPM, GPIO_INTR_NEGEDGE);

    // Activate the ISR for measuring the frequency for the rpm
    if (!initRpmIsrFailed_ && sensorManagerEnableRpmISR()) {
        // Everything worked
        rpmIsrActive_ = true;
        initRpmIsrFailed_ = false;
    } else {
        // It failed
        rpmIsrActive_ = false;
        initRpmIsrFailed_ = true;

        // Logging
        loggerError("Failed to enable the rpm ISR!");
    }

    /* --- Configure the rpm interrupt --- */

    // Return result
    if (initAdc2OilChannelFailed_ || initAdc2FuelChannelFailed_ || initAdc2WaterChannelFailed_ || initSpeedIsrFailed_ || initRpmIsrFailed_)
        return 2;// Initialization succeeded with errors
    return 1;    // Initialization succeeded
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
    waterTemperature_ = calculateWaterTemperatureFromResistance();

    // Did it change?
    if (oldWaterTemperatureValue != waterTemperature_) {
        // Logging
        loggerInfo("Water temperature changed! From: '%d °C' to '%d °C'", oldWaterTemperatureValue, waterTemperature_);
    }
}

float sensorManagerGetWaterTemperature(void) {
    return waterTemperature_;
}

bool sensorManagerEnableSpeedISR() {
    return (gpio_isr_handler_add(GPIO_SPEED, speedInterruptHandler, NULL) == ESP_OK);
}

void sensorManagerDisableSpeedISR() {
    gpio_isr_handler_remove(GPIO_SPEED);
}

void sensorManagerUpdateSpeed(void) {
    // Calculate how much time between the two falling edges was
    const int64_t time = timeOfFallingEdgeSpeed_ - lastTimeOfFallingEdgeSpeed_;

    // Convert the time to seconds
    const float fT = (float) time / 1000.0f;

    // Then save the speed frequency (rounded)
    speedInHz_ = (int) round(1000.0 / fT);

    // Is the speed value valid?
    if (speedInHz_ >= 500) speedInHz_ = -1;

    // Convert the frequency to actual speed
    speed_ = (int) calculateSpeedFromFrequency();
}

int sensorManagerGetSpeed(void) {
    return speed_;
}

bool sensorManagerEnableRpmISR() {
    return gpio_isr_handler_add(GPIO_RPM, rpmInterruptHandler, NULL) == ESP_OK;
}

void sensorManagerDisableRpmISR() {
    gpio_isr_handler_remove(GPIO_RPM);
}

void sensorManagerUpdateRPM(void) {
    // Calculate how much time between the two falling edges was
    const int64_t time = timeOfFallingEdgeRPM_ - lastTimeOfFallingEdgeRPM_;

    // Convert the time to seconds
    const float fT = (float) time / 1000.0f;

    // Then save the rpm frequency (rounded)
    rpmInHz_ = (int) round(1000.0 / fT);

    // Is the rpm value valid?
    if (rpmInHz_ >= 300) rpmInHz_ = -1;

    // Convert the frequency to actual rpm
    rpm_ = calculateRpmFromFrequency();
}

int sensorManagerGetRPM(void) {
    return rpm_;
}