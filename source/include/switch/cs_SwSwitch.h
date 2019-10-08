/**
 * Author: Crownstone Team
 * Copyright: Crownstone (https://crownstone.rocks)
 * Date: Sep 26, 2019
 * License: LGPLv3+, Apache License 2.0, and/or MIT (triple-licensed)
 */

#pragma once

#include <drivers/cs_Serial.h>
#include <events/cs_EventListener.h>
#include <switch/cs_HwSwitch.h>
#include <switch/cs_ISwitch.h>

// forward declaration used for friend definition.
class SwitchAggregator;

/**
 * Wrapper object to the underlying HwSwitch that adds safety features and 
 * software configurable settings.
 * 
 * Provides a simple interface to operate the switch.
 */
class SwSwitch : public ISwitch, public EventListener {
    private:
    HwSwitch hwSwitch;
    switch_state_t currentState = {0};

    // config
    bool allowDimming = false;     // user/app setting
    bool allowSwitching = true;    // user/app setting "locked" if false

    // hardware validation

    // time it takes for a dimmer power check to finish
    const uint32_t dimmerCheckCountDown_initvalue = DIMMER_BOOT_CHECK_DELAY_MS / TICK_INTERVAL_MS;
    uint32_t dimmerCheckCountDown = 0;

    // after this time, assume dimmer is powered  no matter what
    uint32_t dimmerPowerUpCountDown = PWM_BOOT_DELAY_MS / TICK_INTERVAL_MS; 
  
    // flag to prevent multiple power checks, will be set by EVT_TICK handler after calling checkDimmerPower
    bool checkedDimmerPowerUsage = false; 

    // result of checkDimmerPower, true indicates that the dimmer is powered/ready for use.
    bool measuredDimmerPowerUsage = false; 


    /**
     * Initializes the count down timer
     */
    void startDimmerPowerCheck();
    /**
     * Returns true if the dimmerCheckCountDown has expired or
     * if a dimmer power check has successfully measured a correct state.
     */
    bool isDimmerCircuitPowered();

    /**
     * try to check if the dimmer is powered and save the value into
     * isDimmerCircuitPowered.
     * 
     * If dimming isn't allowed or currentState has a dimmer intensity of
     * zero, this method will have no effect.
     * 
     * Will fire a EVT_DIMMER_POWERED event when it detects power while 
     * previous know state was unpowered.
     */
    void checkDimmerPower();

    // error state semantics
    TYPIFY(STATE_ERRORS) getErrorState();
    bool hasDimmingFailed(); 
    bool isSwitchFried();
    bool errorStateOK();

    // functional semantics (derrived from error state)
    bool isSafeToTurnRelayOn();
    bool isSafeToTurnRelayOff();
    bool isSafeToDim();

    // persistance features
    void store(switch_state_t nextState);
    void storeRelayStateUpdate(bool is_on);
    void storeIntensityStateUpdate(uint8_t intensity);
    // void storeDimmerStateUpdate(bool is_on); // not implemented

    // exceptional methods (ignores error state but logs and persists)
    void forceRelayOn();
    void forceSwitchOff();
    void forceDimmerOff();
    
    void setState_unchecked(uint8_t dimmer_value, bool relay_state);
    void setDimmer_unchecked(uint8_t dimmer_value);
    void setRelay_unchecked(bool relay_state);

    // checks state and persists (doesn't call any virtual methods).
    void resetToCurrentState();

    public:
    /**
     * Restores state from persistent memory and tries to recover to that state.
     */
    SwSwitch(HwSwitch hw_switch);

    // SwSwitch

    // will setRelay(true) if dimmer was active
    void setAllowDimming(bool allowed); 
    
    // if false is passed, no state changes may occur through
    // the ISwitch interface.
    void setAllowSwitching(bool allowed); // TODO(Arend): this isn't implemented!

    /**
     * Checks the current state and tries to set it to the opposite.
     */
    void toggle();

    // ISwitch

    /**
     * Checks if the relay is still okay and will set it to the 
     * desired value if possible.
     * 
     * Updates switchState and calls saveSwitchState.
     */
    void setRelay(bool is_on);

    /**
     * Checks if the dimmer is still okay and will set it to the 
     * desired value if possible.
     * 
     * Updates switchState and calls saveSwitchState.
     */
    void setDimmer(bool is_on);

    /**
     * Checks if the dimmer is still okay and will set it to the 
     * desired value if possible.
     * 
     * Will prefer to use dimmer features, but if impossible try
     * to solve the request using the relay.
     * 
     * Updates switchState and calls saveSwitchState.
     */
    void setIntensity(uint8_t value);

    // EventListener

    /**
     * Listens to Allow Dimming, and Lock events and error states
     */
    virtual void handleEvent(event_t& evt) override;
};