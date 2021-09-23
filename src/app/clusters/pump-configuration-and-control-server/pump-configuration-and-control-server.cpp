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

#include <app-common/zap-generated/attributes/Accessors.h>
#include <app-common/zap-generated/enums.h>
#include <app-common/zap-generated/ids/Attributes.h>
#include <app-common/zap-generated/ids/Clusters.h>
#include <app-common/zap-generated/ids/Commands.h>

using namespace chip;
using namespace chip::app::Clusters::PumpConfigurationAndControl::Attributes;

void emberAfPumpConfigurationAndControlClusterServerInitCallback(EndpointId endpoint)
{
    emberAfDebugPrintln("Initialize PCC Server Cluster [EP:%d]", endpoint);
    // TODO
}

// Method being called prior to setting the actual attribute
EmberAfStatus emberAfPumpConfigurationAndControlClusterServerPreAttributeChangedCallback(chip::EndpointId endpoint,
                                                                                         chip::AttributeId attributeId,
                                                                                         EmberAfAttributeType attributeType,
                                                                                         uint16_t size, uint8_t * value)
{
    EmberAfStatus status = EMBER_ZCL_STATUS_SUCCESS;

    emberAfDebugPrintln("PCC ServerPreAttributeChangedCB: [EP:%d, ID:0x%x, size:%d]", endpoint, attributeId, size);

    switch (attributeId)
    {
    case Ids::OperationMode: {
        uint8_t opMode = *value;
        if (opMode < EMBER_ZCL_PUMP_OPERATION_MODE_NORMAL || opMode > EMBER_ZCL_PUMP_OPERATION_MODE_LOCAL)
        {
            status = EMBER_ZCL_STATUS_INVALID_VALUE;
        }
        else
        {
            SetEffectiveOperationMode(endpoint, opMode);
            status = EMBER_ZCL_STATUS_SUCCESS;
        }
        break;
    }
    default:
        break;
    }

    return status;
}

void emberAfPumpConfigurationAndControlClusterServerAttributeChangedCallback(EndpointId endpoint, AttributeId attributeId)
{
    emberAfDebugPrintln("PCC Server Cluster Attribute changed [EP:%d, ID:0x%x]", endpoint, attributeId);
    // TODO
}
