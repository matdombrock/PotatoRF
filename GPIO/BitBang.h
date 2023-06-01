#pragma once
#include "GPIO.h"

class BitBang{
public:
    BitBang(const char* newChipName="gpiochip1", int newLineNum=98) 
    : gpio(newChipName, newLineNum)
    {
        gpio.open();
    }
    ~BitBang(){
        gpio.release();
    }
    void setToggleMode(){
        //std::cout << "Btn set to toggle";
        toggleMode = true;
    }
    bool get(){
        return gpio.get();
    }
    void set(bool val){
        gpio.set(val);
    }
    bool getToggled(){
        return toggled;
    }
    bool changed(){
        stateC = gpio.getState();
        if(stateC == gpio.get()){
            // No change
            return false;
        }
        // State changed
        if(toggleMode){
            if(stateC == true){
                // The button is down
                toggled = !toggled;
                //std::cout << "TOGGLED" << std::endl;
                //std::cout << toggled << std::endl;
                return true;
            }
            else{
                return false;
            }
        }
        return true;
    }
private:
    bool stateC = false;
    bool toggleMode = false;
    bool toggled = false;
    GPIO gpio;
};
// on off on off
//
