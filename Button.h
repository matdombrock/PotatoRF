#pragma once
#include "GPIO.h"

class Button{
public:
    Button(const char* newChipName="gpiochip1", int newLineNum=98) 
    : gpio(newChipName, newLineNum)
    {
        gpio.open();
    }
    ~Button(){
        gpio.release();
    }
    void setToggleMode(){
        //std::cout << "Btn set to toggle";
        toggleMode = true;
    }
    bool get(){
        return gpio.get();
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
