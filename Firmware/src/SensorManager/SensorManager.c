/* --- Includes --- */
#include "SensorManager/SensorManager.h"

/* --- Private Defines & Macros --- */

/* --- Private Variables, Typedefs etc. --- */
// ADC stuff
static adc_oneshot_unit_handle_t adc2Handle_;
static bool initAdc2Failed_ = false;
static adc_oneshot_unit_handle_t adc1Handle_;
static bool initAdc1Failed_ = false;

// Oil pressure stuff
static bool oilPressure_ = false;
static adc_cali_handle_t adc2OilCaliHandle_;
static bool initAdc2OilChannelFailed_ = false;
static void (*oilPressureCallback_)(void *) = NULL;

// Fuel level stuff
static int fuelLevelInPercent_ = 0;
static int fuelLevelInLitre_ = 0;
static float fuelLevelResistance_ = 0.0f;
static adc_cali_handle_t adc2FuelCaliHandle_;
static bool initAdc2FuelChannelFailed_ = false;
static void (*fuelLevelPercentCallback_)(void *) = NULL;
static void (*fuelLevelLitreCallback_)(void *) = NULL;

// Water temperature stuff
static float waterTemperature_ = 0.0f;
static float waterTemperatureResistance_ = 0.0f;
static adc_cali_handle_t adc2WaterCaliHandle_;
static bool initAdc2WaterChannelFailed_ = false;
static void (*waterTemperatureCallback_)(void *) = NULL;

// Speed stuff
static int speedInHz_ = -1;
static int speed_ = -1;
static int64_t lastTimeOfFallingEdgeSpeed_ = 0;
static int64_t timeOfFallingEdgeSpeed_ = 0;
static bool speedIsrActive_ = false;
static bool initSpeedIsrFailed_ = false;
static void (*speedCallback_)(void *) = NULL;

// RPM stuff
static int rpmInHz_ = -1;
static int rpm_ = -1;
static int64_t lastTimeOfFallingEdgeRPM_ = 0;
static int64_t timeOfFallingEdgeRPM_ = 0;
static bool rpmIsrActive_ = false;
static bool initRpmIsrFailed_ = false;
static void (*rpmCallback_)(void *) = NULL;

// Internal temperature sensor stuff
static int intTempRawAdcValue_ = 0;
static int intTempVoltageMV_ = 0;
static double internalTemperature_ = 0.0;
static adc_cali_handle_t adc2IntTempCaliHandle_;
static bool initAdc1IntTempChannelFailed_ = false;
static void (*internalTemperatureCallback_)(void *) = NULL;

// Temporary stuff so I don't forget anything to implement
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

    /* --- Configure the oil pressure ADC2 channel --- */

    // Configure oil pressure GPIO
    gpio_set_direction(GPIO_OIL_PRESSURE, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_OIL_PRESSURE, GPIO_PULLDOWN_ONLY);

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

    /* --- Configure the internal temperature sensor --- */

    // Setup gpio
    gpio_set_direction(GPIO_NUM_7, GPIO_MODE_INPUT);
    gpio_set_pull_mode(GPIO_NUM_7, GPIO_PULLDOWN_ONLY);

    // Initialize the ADC2
    const adc_oneshot_unit_init_cfg_t adc1InitConfig = {
            .unit_id = ADC_UNIT_1,
            .ulp_mode = ADC_ULP_MODE_DISABLE};
    if (adc_oneshot_new_unit(&adc1InitConfig, &adc1Handle_) != ESP_OK) {
        // Init was NOT successful!
        initAdc1Failed_ = true;

        // Logging
        loggerWarn("Failed to initialize ADC1!");
    }

    // Create the channel config
    const adc_oneshot_chan_cfg_t adc2IntTempConfig = {
            .bitwidth = ADC_BITWIDTH_12,
            .atten = ADC_ATTEN_DB_6,
    };
    initAdc1IntTempChannelFailed_ = adc_oneshot_config_channel(adc1Handle_, ADC_CHANNEL_INT_TEMPERATURE, &adc2IntTempConfig) != ESP_OK;

    // Logging
    if (initAdc1IntTempChannelFailed_) {
        loggerError("Failed to initialize ADC2 channel: %d", ADC_CHANNEL_INT_TEMPERATURE);
    }

    // Create the calibration curve config
    const adc_cali_curve_fitting_config_t intTempCaliConfig = {
            .unit_id = ADC_UNIT_1,
            .chan = ADC_CHANNEL_INT_TEMPERATURE,
            .atten = ADC_ATTEN_DB_6,
            .bitwidth = ADC_BITWIDTH_12,
    };

    // Create calibration curve fitting
    if (adc_cali_create_scheme_curve_fitting(&intTempCaliConfig, &adc2IntTempCaliHandle_) != ESP_OK) {
        // Mark the oil channel as failed
        initAdc1IntTempChannelFailed_ = true;

        // Logging
        loggerError("'adc_cali_create_scheme_curve_fitting' for the internal temperature sensor channel FAILED");
    }

    /* --- Configure the internal temperature sensor --- */

    // Return result
    if (initAdc2Failed_ || initAdc2OilChannelFailed_ || initAdc2FuelChannelFailed_ || initAdc2WaterChannelFailed_ || initSpeedIsrFailed_ || initRpmIsrFailed_ || initAdc1IntTempChannelFailed_)
        return 2;// Initialization succeeded with errors
    return 1;    // Initialization succeeded
}

void sensorManagerRegisterCallback(const SENSOR sensorType, void callback(void *)) {
    switch (sensorType) {
        case (SENSOR_OIL_PRESSURE):
            // Save the cb function
            oilPressureCallback_ = callback;
            break;
        case (SENSOR_FUEL_LEVEL_PERCENT):
            // Save the cb function
            fuelLevelPercentCallback_ = callback;
            break;
        case (SENSOR_FUEL_LEVEL_LITRE):
            // Save the cb function
            fuelLevelLitreCallback_ = callback;
            break;
        case (SENSOR_WATER_TEMPERATURE):
            // Save the cb function
            waterTemperatureCallback_ = callback;
            break;
        case (SENSOR_INTERNAL_TEMPERATURE):
            // Save the cb function
            internalTemperatureCallback_ = callback;
            break;
        case (SENSOR_SPEED):
            // Save the cb function
            speedCallback_ = callback;
            break;
        case (SENSOR_RPM):
            // Save the cb function
            rpmCallback_ = callback;
            break;
    }
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
        // Callback
        if (oilPressureCallback_ != NULL) {
            oilPressureCallback_((void *) &oilPressure_);
        }

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
        // Callback 1
        if (fuelLevelPercentCallback_ != NULL) {
            fuelLevelPercentCallback_((void *) fuelLevelInPercent_);
        }

        // Callback 2
        if (fuelLevelLitreCallback_ != NULL) {
            fuelLevelLitreCallback_((void *) fuelLevelInLitre_);// TODO: Calculate Litres
        }

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
        // Callback
        if (waterTemperatureCallback_ != NULL) {
            waterTemperatureCallback_((void *) &waterTemperature_);
        }

        // Logging
        loggerInfo("Water temperature changed! From: '%d °C' to '%d °C'", oldWaterTemperatureValue, waterTemperature_);
    }
}

float sensorManagerGetWaterTemperature(void) {
    return waterTemperature_;
}

bool sensorManagerEnableSpeedISR() {
    // Was the init successfully?
    if (initSpeedIsrFailed_) return false;

    return (gpio_isr_handler_add(GPIO_SPEED, speedInterruptHandler, NULL) == ESP_OK);
}

void sensorManagerDisableSpeedISR() {
    // Was the init successfully?
    if (initSpeedIsrFailed_) return;

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
    if (speedInHz_ >= 500) speedInHz_ = 0;

    // Convert the frequency to actual speed
    const int oldSpeed = speed_;
    speed_ = (int) calculateSpeedFromFrequency();

    // Is it >1
    if (speed_ < 0) speed_ = 0;

    if (oldSpeed != speed_) {
        // Callback
        if (speedCallback_ != NULL) {
            //speedCallback_((void *) speed_); Problem hier
        }

        // Logging
        loggerInfo("Speed changed! From: '%d kmh' to '%d kmh'", oldSpeed, speed_);
    }
}

int sensorManagerGetSpeed(void) {
    return speed_;
}

bool sensorManagerEnableRpmISR() {
    // Was the init successfully?
    if (initRpmIsrFailed_) return false;

    return gpio_isr_handler_add(GPIO_RPM, rpmInterruptHandler, NULL) == ESP_OK;
}

void sensorManagerDisableRpmISR() {
    // Was the init successfully?
    if (initRpmIsrFailed_) return;

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
    const int oldRpm = rpm_;
    rpm_ = calculateRpmFromFrequency();

    // Is it >1
    if (rpm_ < 0) rpm_ = 0;

    if (oldRpm != rpm_) {
        // Callback
        if (rpmCallback_ != NULL) {
            rpmCallback_((void *) rpm_);
        }

        // Logging
        loggerInfo("RPM changed! From: '%d RPM' to '%d RPM'", oldRpm, rpm_);
    }
}

int sensorManagerGetRPM(void) {
    return rpm_;
}

void sensorManagerUpdateInternalTemperature(void) {
    // Was the init successfully?
    if (initAdc2Failed_ || initAdc1IntTempChannelFailed_) return;

    // Try to get a reading from the ADC
    if (adc_oneshot_read(adc1Handle_, ADC_CHANNEL_INT_TEMPERATURE, &intTempRawAdcValue_) != ESP_OK) {
        // Logging
        loggerError("Failed to read the internal temperature from the ADC!");
    }

    // Convert the adc raw value to a voltage (mV)
    adc_cali_raw_to_voltage(adc2IntTempCaliHandle_, intTempRawAdcValue_, &intTempVoltageMV_);

    // Then calculate the temperature from the voltage
    internalTemperature_ = ((double) intTempVoltageMV_ - 540.0) / 10.0;
}

double sensorManagerGetInternalTemperature(void) {
    return internalTemperature_;
}