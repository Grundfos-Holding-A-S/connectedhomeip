/*
 *
 *    Copyright (c) 2020 Project CHIP Authors
 *    Copyright (c) 2019 Google LLC.
 *    All rights reserved.
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

#include "PumpManager.h"

#include "AppConfig.h"
#include "AppTask.h"
#include "FreeRTOS.h"

#define ACTUATOR_MOVEMENT_PERIOS_MS 500

PumpManager PumpManager::sLock;

int PumpManager::Init()
{
    int ret = 0;

    mTimerHandle = xTimerCreate("BLT_TIMER", pdMS_TO_TICKS(ACTUATOR_MOVEMENT_PERIOS_MS), pdFALSE, this, TimerEventHandler);
    if (NULL == mTimerHandle)
    {
        PLAT_LOG("failed to create bolt lock timer");
        while (1)
            ;
    }

    mState              = kState_LockingCompleted;
    mAutoLockTimerArmed = false;
    mAutoRelock         = false;
    mAutoLockDuration   = 0;

    return ret;
}

void PumpManager::SetCallbacks(Callback_fn_initiated aActionInitiated_CB, Callback_fn_completed aActionCompleted_CB)
{
    mActionInitiated_CB = aActionInitiated_CB;
    mActionCompleted_CB = aActionCompleted_CB;
}

bool PumpManager::IsActionInProgress()
{
    return (mState == kState_LockingInitiated || mState == kState_UnlockingInitiated);
}

bool PumpManager::IsUnlocked()
{
    return (mState == kState_UnlockingCompleted);
}

void PumpManager::EnableAutoRelock(bool aOn)
{
    mAutoRelock = aOn;
}

void PumpManager::SetAutoLockDuration(uint32_t aDurationInSecs)
{
    mAutoLockDuration = aDurationInSecs;
}

bool PumpManager::InitiateAction(int32_t aActor, Action_t aAction)
{
    bool action_initiated = false;
    State_t new_state;

    // Initiate Lock/Unlock Action only when the previous one is complete.
    if (mState == kState_LockingCompleted && aAction == UNLOCK_ACTION)
    {
        action_initiated = true;

        new_state = kState_UnlockingInitiated;
    }
    else if (mState == kState_UnlockingCompleted && aAction == LOCK_ACTION)
    {
        action_initiated = true;

        new_state = kState_LockingInitiated;
    }

    if (action_initiated)
    {
        if (mAutoLockTimerArmed && new_state == kState_LockingInitiated)
        {
            // If auto lock timer has been armed and someone initiates locking,
            // cancel the timer and continue as normal.
            mAutoLockTimerArmed = false;

            CancelTimer();
        }

        StartTimer(ACTUATOR_MOVEMENT_PERIOS_MS);

        // Since the timer started successfully, update the state and trigger callback
        mState = new_state;

        if (mActionInitiated_CB)
        {
            mActionInitiated_CB(aAction, aActor);
        }
    }

    return action_initiated;
}

void PumpManager::StartTimer(uint32_t aTimeoutMs)
{
    xTimerChangePeriod(mTimerHandle, pdMS_TO_TICKS(aTimeoutMs), 100);
    xTimerStart(mTimerHandle, 100);
}

void PumpManager::CancelTimer(void)
{
    xTimerStop(mTimerHandle, 100);
}

void PumpManager::TimerEventHandler(TimerHandle_t aTimer)
{
    PumpManager * lock = static_cast<PumpManager *>(pvTimerGetTimerID(aTimer));

    // The timer event handler will be called in the context of the timer task
    // once sLockTimer expires. Post an event to apptask queue with the actual handler
    // so that the event can be handled in the context of the apptask.
    AppEvent event;
    event.Type                  = AppEvent::kEventType_AppEvent;
    event.BoltLockEvent.Context = static_cast<PumpManager *>(lock);
    if (lock->mAutoLockTimerArmed)
    {
        event.Handler = AutoReLockTimerEventHandler;
    }
    else
    {
        event.Handler = ActuatorMovementTimerEventHandler;
    }
    GetAppTask().PostEvent(&event);
}

void PumpManager::AutoReLockTimerEventHandler(AppEvent * aEvent)
{
    PumpManager * lock = static_cast<PumpManager *>(aEvent->BoltLockEvent.Context);
    int32_t actor      = 0;

    // Make sure auto lock timer is still armed.
    if (!lock->mAutoLockTimerArmed)
    {
        return;
    }

    lock->mAutoLockTimerArmed = false;

    PLAT_LOG("Auto Re-Lock has been triggered!");

    lock->InitiateAction(actor, LOCK_ACTION);
}

void PumpManager::ActuatorMovementTimerEventHandler(AppEvent * aEvent)
{
    Action_t actionCompleted = INVALID_ACTION;

    PumpManager * lock = static_cast<PumpManager *>(aEvent->BoltLockEvent.Context);

    if (lock->mState == kState_LockingInitiated)
    {
        lock->mState    = kState_LockingCompleted;
        actionCompleted = LOCK_ACTION;
    }
    else if (lock->mState == kState_UnlockingInitiated)
    {
        lock->mState    = kState_UnlockingCompleted;
        actionCompleted = UNLOCK_ACTION;
    }

    if (actionCompleted != INVALID_ACTION)
    {
        if (lock->mActionCompleted_CB)
        {
            lock->mActionCompleted_CB(actionCompleted);
        }

        if (lock->mAutoRelock && actionCompleted == UNLOCK_ACTION)
        {
            // Start the timer for auto relock
            lock->StartTimer(lock->mAutoLockDuration * 1000);

            lock->mAutoLockTimerArmed = true;

            PLAT_LOG("Auto Re-lock enabled. Will be triggered in %u seconds", lock->mAutoLockDuration);
        }
    }
}

int16_t PumpManager::GetMaxPressure()
{
    // 1.6.1. MaxPressure Attribute
    // Range -3276.7 kPa to 3276.7 kPa (steps of 0.1 kPa)
    // -3276.8 is invalid value - perhaps 'null'

    // Return 2000.0 kPa as Max Pressure
    return 20000;
}

uint16_t PumpManager::GetMaxSpeed()
{
    // 1.6.2. MaxSpeed Attribute
    // Range 0 RPM to 65534 RPM (steps of 1 RPM)
    // 65535 is invalid value - perhaps 'null'

    // Return 1000 RPM as MaxSpeed
    return 1000;
}

uint16_t PumpManager::GetMaxFlow()
{
    // 1.6.3. MaxFlow Attribute
    // Range 0 m3/h to 6553.4 m3/h (steps of 0.1 m3/h)
    // 6553.5 m3/h is invalid value - perhaps 'null'

    // Return 200.0 m3/h as MaxFlow
    return 2000;
}

int16_t PumpManager::GetMinConstPressure()
{
    // 1.6.4. MinConstPressure Attribute
    // Range -3276.7 kPa to 3276.7 kPa (steps of 0.1 kPa)
    // -3276.8 is invalid value - perhaps 'null'

    // Return -100.0 kPa as MinConstPressure
    return -1000;
}

int16_t PumpManager::GetMaxConstPressure()
{
    // 1.6.5. MaxConstPressure Attribute
    // Range -3276.7 kPa to 3276.7 kPa (steps of 0.1 kPa)
    // -3276.8 is invalid value - perhaps 'null'

    // Return 100.0 kPa as MaxConstPressure
    return 1000;
}

int16_t PumpManager::GetMinCompPressure()
{
    // 1.6.6. MinCompPressure Attribute
    // Range -3276.7 kPa to 3276.7 kPa (steps of 0.1 kPa)
    // -3276.8 is invalid value - perhaps 'null'

    // Return -20.0 kPa as MinCompPressure
    return -200;
}

int16_t PumpManager::GetMaxCompPressure()
{
    // 1.6.7. MaxCompPressure Attribute
    // Range -3276.7 kPa to 3276.7 kPa (steps of 0.1 kPa)
    // -3276.8 is invalid value - perhaps 'null'

    // Return 20.0 kPa as MaxCompPressure
    return 200;
}

uint16_t PumpManager::GetMinConstSpeed()
{
    // 1.6.8. MinConstSpeed Attribute
    // Range 0 to 65534 RPM (steps of 1 RPM)
    // 65535 RPM is invalid valud - perhaps 'null'

    // Return 200 RPM as MinConstSpeed
    return 200;
}

uint16_t PumpManager::GetMaxConstSpeed()
{
    // 1.6.9. MaxConstSpeed Attribute
    // Range 0 to 65534 RPM (steps of 1 RPM)
    // 65535 RPM is invalid valud - perhaps 'null'

    // Return 2000 RPM as MaxConstSpeed
    return 2000;
}

uint16_t PumpManager::GetMinConstFlow()
{
    // 1.6.10. MinConstFlow Attribute
    // Range 0 m3/h to 6553.4 m3/h (steps of 0.1 m3/h)
    // 6553.5 m3/h is invalid value - perhaps 'null'

    // Return 12.5 m3/h as MinConstFlow
    return 125;
}

uint16_t PumpManager::GetMaxConstFlow()
{
    // 1.6.11. MaxConstFlow Attribute
    // Range 0 m3/h to 6553.4 m3/h (steps of 0.1 m3/h)
    // 6553.5 m3/h is invalid value - perhaps 'null'

    // Return 655.7 m3/h as MaxConstFlow
    return 6557;
}

int16_t PumpManager::GetMinConstTemp()
{
    // 1.6.12. MinConstTemp Attribute
    // Range -273.15 C to 327.67 C (steps of 0.01 C)
    // All other values are invalid values - perhaps 'null'

    // Return 30.00 C as MinConstTemp
    return 3000;
}

int16_t PumpManager::GetMaxConstTemp()
{
    // 1.6.13. MaxConstTemp Attribute
    // Range -273.15 C to 327.67 C (steps of 0.01 C)
    // All other values are invalid values - perhaps 'null'

    // Return 56.00 C as MaxConstTemp
    return 5600;
}
