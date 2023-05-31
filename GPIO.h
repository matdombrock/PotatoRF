/*
    The base class for all GPIO devices
    Handles:
    - Open
    - Read
    - Write
    - PWM
    - Release
*/
#pragma once
#include <gpiod.h>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unistd.h>
#include "Util.h"

class GPIO{
public:
    GPIO(const char* newChipName="gpiochip1", int newLineNum=98){
        chipName = newChipName;
        lineNum = newLineNum;
    }
    ~GPIO(){
        m_pwmThread.join();
    }
    void open(){
        log("Opening GPIO");
        // Open GPIO chip
        chip = gpiod_chip_open_by_name(chipName);
        // Open GPIO lines
        lineLED = gpiod_chip_get_line(chip, lineNum);
        // Open LED lines for output
        gpiod_line_request_output(lineLED, "example1", 0);
    }
    void release(){
        log("Releasing GPIO");
        // Release lines and chip
        pwmStop();
        gpiod_line_release(lineLED);
        gpiod_chip_close(chip);
    }
    virtual void set(bool val){
        pwmStop();
        gpiod_line_set_value(lineLED, val);
        state = val;
    }
    void on(){
        set(1);
    }
    void off(){
        set(0);
    }
    virtual bool get(){
        bool val = gpiod_line_get_value(lineLED);
        state = val;
        return val;
    }
    int getState(){
        return state;
    }
    // duty should be a float
    void pwm(int duty){
        if(pwmEnabled == false){
            log("Starting PWM Thread");
            pwmEnabled = true;
            if(m_pwmThreadRunning == false){
                m_pwmThreadRunning = true;
                m_pwmThread = std::thread(&GPIO::pwmThreadFunc, this);
            }
        }
        // Lock the mutex before accessing the message queue
        std::unique_lock<std::mutex> lock(m_messageQueueMutex);
        // Add the message to the queue
        std::string message = std::to_string(duty);
        m_messageQueue.push(message);
        // Notify the PWM thread that there's a new message
        m_messageQueueCV.notify_one();
    }
    void pwmStop(){
        pwmEnabled = false;
    }
    std::string getChipName(){
        return chipName;
    }
    int getLineNum(){
        return lineNum;
    }
    // Sets the cycle time in useconds
    // Default is 10000
    void setCycleTime(int useconds=10000){
        cycleTime = useconds;
    }
    // Log a message containing all of the GPIO info
    void log(std::string msg){
        std::cout << "---------------------------" << std::endl;
        std::cout << msg << std::endl;
        std::cout << "Chip Name: " << chipName << std::endl;
        std::cout << "Line Number: " << lineNum << std::endl;
        std::cout << "---------------------------" << std::endl;
    } 
    const char *chipName;
    int lineNum = 0; 
    int state = 0;// Cache the pin state
    struct gpiod_chip *chip;
    struct gpiod_line *lineLED;
    int cycleTime = 100 * 10;// 100 * 100
    bool pwmEnabled = false;
    bool m_pwmThreadRunning = false;
    std::thread m_pwmThread;
    std::queue<std::string> m_messageQueue;
    std::mutex m_messageQueueMutex;
    std::condition_variable m_messageQueueCV;
private:
    void pwmThreadFunc(){
        int duty = 0;
        while(m_pwmThreadRunning){
            if(pwmEnabled == false){
                continue;
            }
            std::unique_lock<std::mutex> lock(m_messageQueueMutex);
            if (!m_messageQueue.empty()) {
                m_messageQueueCV.wait(lock, [this] { return !m_messageQueue.empty(); });
                // Get the next message from the queue
                std::string message = m_messageQueue.front();
                m_messageQueue.pop();
                // Process the message
                std::cout << "Received PWM message: " << message << std::endl;
                // Set the duty cycle accoring to the message
                duty = std::stoi(message);
                // Ensure 0->100
                duty = Util::bound(duty, 0, 100);
            }
            // Release the lock and continue the loop
            lock.unlock();
            
            // Scale to cycleTime
            int onTime = duty * (cycleTime / 100);
            // Ensure onTime doesnt exceed
            // cycleTime
            if(onTime > cycleTime){
                onTime = cycleTime;
            }

            int offTime = cycleTime - onTime;
            if(offTime < 0){
                offTime = 0;
            }
            
            gpiod_line_set_value(lineLED, 1);

            // Sleep for duty time
            usleep(onTime);

            gpiod_line_set_value(lineLED, 0);
            
            // Sleep for rest time
            usleep(offTime);
            continue;
        }
    }
};
