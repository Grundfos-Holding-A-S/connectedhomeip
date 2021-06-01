
#pragma once

#include <app/util/basic-types.h>


typedef enum {
    PmpOpMode_Normal,
    PmpOpMode_Min,
    PmpOpMode_Max,    
    PmpOpMode_Local
} PumpOperationMode;

typedef enum {
    PmpCtrlMode_ConstSpeed,
    PmpCtrlMode_ConstPressure,
    PmpCtrlMode_PropPressure,    
    PmpCtrlMode_ConstFlow,
    PmpCtrlMode_ConstTemperature = 5,
    PmpCtrlMode_Auto = 7
} PumpControlMode;

// enum used to track the type of occupancy sensor being implemented
typedef enum
{
    HAL_PMP_CFG_CTRL_REM_SENSOR_TYPE_NONE     = 0x00,
    HAL_PMP_CFG_CTRL_REM_SENSOR_TYPE_TEMP     = 0x01,
    HAL_PMP_CFG_CTRL_REM_SENSOR_TYPE_PRESSURE = 0x02,
    HAL_PMP_CFG_CTRL_REM_SENSOR_TYPE_FLOW     = 0x03,
} HalPmpCfgCtrlRemSensorType;

/** @brief Get the hardware mechanism used to detect remote sensor
 *
 * This function should be used to determine what kind of remote sensor
 * is driving the pump functionality.
 *
 * @return HAL_PMP_CFG_CTRL_REM_SENSOR_TYPE_NONE, HAL_PMP_CFG_CTRL_REM_SENSOR_TYPE_TEMP,
 * HAL_PMP_CFG_CTRL_REM_SENSOR_TYPE_FLOW or HAL_PMP_CFG_CTRL_REM_SENSOR_TYPE_PRESSURE, which 
 * are defined to match the values used by the ZCL defined SensorType attribute
 */
HalPmpCfgCtrlRemSensorType halPmpCfgCtrlGetRemSensorType(chip::EndpointId endpoint);
