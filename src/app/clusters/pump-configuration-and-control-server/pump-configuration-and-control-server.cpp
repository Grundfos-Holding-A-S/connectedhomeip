/**
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include <app/util/af.h>

#include <app/util/af-event.h>
#include <app/util/attribute-storage.h>

#include <app/common/gen/attribute-id.h>
#include <app/common/gen/attribute-type.h>
#include <app/common/gen/cluster-id.h>
#include <app/common/gen/enums.h>

#include "pump-configuration-and-control-server.h"

using namespace chip;

static PumpOperationMode _opMode;
static PumpControlMode _ctrlMode;

void emberAfPCC_SelectControlMode(EndpointId endpoint)
{
    EmberAfStatus status;
    uint8_t newValue;
    if (_opMode == PmpOpMode_Normal)
    {
        HalPmpCfgCtrlRemSensorType connectedSensor = halPmpCfgCtrlGetRemSensorType(endpoint);
        switch(connectedSensor)
        {
            case HAL_PMP_CFG_CTRL_REM_SENSOR_TYPE_NONE:
                // use ControlMode Speed
                _ctrlMode = PmpCtrlMode_Auto; // or maybe PmpCtrlMode_Auto ??
                break;
            case HAL_PMP_CFG_CTRL_REM_SENSOR_TYPE_TEMP:
                // use Temperature control
                _ctrlMode = PmpCtrlMode_ConstTemperature;
                break;
            case HAL_PMP_CFG_CTRL_REM_SENSOR_TYPE_PRESSURE:
                // use Pressure control
                _ctrlMode = PmpCtrlMode_ConstPressure; // or maybe PmpCtrlMode_PropPressure ??
                break;
            case HAL_PMP_CFG_CTRL_REM_SENSOR_TYPE_FLOW:
                // use Flow control
                _ctrlMode = PmpCtrlMode_ConstFlow;
                break;
        }
    }
    else
    {
        _ctrlMode = PmpCtrlMode_ConstSpeed;
    }
    newValue = (uint8_t)_ctrlMode;
    status = emberAfWriteAttribute(endpoint, ZCL_PUMP_CONFIG_CONTROL_CLUSTER_ID, ZCL_CONTROL_MODE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                                       (uint8_t *) &newValue, ZCL_ENUM8_ATTRIBUTE_TYPE);
    if (status != EMBER_ZCL_STATUS_SUCCESS)
    {
        emberAfPumpConfigControlClusterPrintln("ERR: writing pump control mode %x", status);
    }   
}

void emberAfPumpConfigurationAndControlClusterServerInitCallback(EndpointId endpoint)
{
    EmberAfStatus status;
    uint8_t currentValue;
    // read current operationMode value
    status = emberAfReadAttribute(endpoint, ZCL_PUMP_CONFIG_CONTROL_CLUSTER_ID, ZCL_OPERATION_MODE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                                  (uint8_t *) &currentValue, sizeof(currentValue),
                                  NULL); // data type

    if (status != EMBER_ZCL_STATUS_SUCCESS)
    {
        emberAfPumpConfigControlClusterPrintln("ERR: reading pump operation mode %x", status);
        return;
    }
    _opMode = (PumpOperationMode)currentValue;
    emberAfPCC_SelectControlMode(endpoint);
}

void emberAfPumpConfigurationAndControlClusterServerAttributeChangedCallback(EndpointId endpoint, AttributeId attributeId)
{
    EmberAfStatus status;
    uint8_t currentValue;
    if (attributeId == ZCL_OPERATION_MODE_ATTRIBUTE_ID)
    {
        status = emberAfReadAttribute(endpoint, ZCL_PUMP_CONFIG_CONTROL_CLUSTER_ID, ZCL_OPERATION_MODE_ATTRIBUTE_ID, CLUSTER_MASK_SERVER,
                                    (uint8_t *) &currentValue, sizeof(currentValue),
                                    NULL); // data type

        if (status != EMBER_ZCL_STATUS_SUCCESS)
        {
            emberAfPumpConfigControlClusterPrintln("ERR: reading pump operation mode %x", status);
            return;
        }
        _opMode = (PumpOperationMode)currentValue;
        emberAfPCC_SelectControlMode(endpoint);
    }
}

//******************************************************************************
// Notification callback from the HAL plugin
//******************************************************************************

HalPmpCfgCtrlRemSensorType __attribute__((weak)) halPmpCfgCtrlGetRemSensorType(chip::EndpointId endpoint)
{
    return HAL_PMP_CFG_CTRL_REM_SENSOR_TYPE_NONE;
}
