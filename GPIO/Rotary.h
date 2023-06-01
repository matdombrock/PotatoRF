#include <chrono>
#include <stdio.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unistd.h>
#include "GPIO.h"

class Rotary{
public:
    Rotary(
        const char* newChipNameDT, int newLineNumDT,
        const char* newChipNameCLK, int newLineNumCLK
    )
    :
    gpioDT(newChipNameDT, newLineNumDT),
    gpioCLK(newChipNameCLK, newLineNumCLK)
    {
        gpioDT.open();
        gpioCLK.open();
        // Start loop
        std::cout << "Starting Read Thread" << std::endl;
        m_readThread = std::thread(&Rotary::readThreadFunc, this);
    }
    ~Rotary(){
        gpioDT.release();
        gpioCLK.release();
        m_readThread.join();
    }
    std::string read(){
        std::string out = outQueue;
        // Check if we actually have a queue
        // and should clear
        if(out.size() > 0){
            clear();
        }
        return out;
    }
    void clear(){
        // Lock the mutex before accessing the message queue
        std::unique_lock<std::mutex> lock(m_messageQueueMutex);
        // Add the message to the queue
        // saying we want to clear it
        std::string message = "clear";
        m_messageQueue.push(message);
         // Notify the thread that there's a new message
        m_messageQueueCV.notify_one();
    }
    bool m_readThreadRunning = true;
    std::thread m_readThread;
    std::queue<std::string> m_messageQueue;
    std::mutex m_messageQueueMutex;
    std::condition_variable m_messageQueueCV;
private:
    void readThreadFunc(){
        wasHit = false;
        lastDir = 'n';
        std::cout << "Enter Read Thread Loop" << std::endl;
        while(m_readThreadRunning){
            usleep(1000);// CPU Optimization
            std::unique_lock<std::mutex> lock(m_messageQueueMutex);
            if (!m_messageQueue.empty()) {
                m_messageQueueCV.wait(lock, [this] { return !m_messageQueue.empty(); });
                // Get the next message from the queue
                std::string message = m_messageQueue.front();
                m_messageQueue.pop();
                // Process the message
                if(message == "clear"){
                    // Clear out queue
                    outQueue = "";
                }
            }
            // Release the lock and continue the loop
            lock.unlock();
            // Read CLK & DT
            bool clk = gpioCLK.get();
            bool dt = gpioDT.get();
            if(clk && dt){
                wasHit = true;
                continue;
            }
            if(wasHit && dt){
                processChange('l');
                continue;
            }
            if(wasHit && clk){
                processChange('r');
                continue;
            }
        }
    }

    // Process an incoming read message
    void processChange(char thisDir){
        int speed = checkSpeed(ts);
        if(checkDebounce(speed, thisDir) == false){
            std::string dirString = thisDir == 'r' ? "Right" : "Left";
            std::cout << dirString << ": " << speed << std::endl;
            sendOut(speed, thisDir);
        }
        ts = timestamp();
        lastDir = thisDir;
        wasHit = false;
    }

    // Send out one char for each speed lvl
    void sendOut(int speed, char thisDir){
        for(int s = 0; s < speed + 1; s++){
            std::string dirString(1, thisDir);
            outQueue += dirString;
        }
    }

    // Returns true if this movement was a bounce
    // If the direction has changed and the
    // speed was over `debounceThreshold` this was
    // likely a bounce
    bool checkDebounce(int speed, const char thisDir){
        if(speed > debounceThreshold && lastDir != thisDir){
            std::string dirString = thisDir == 'r' ? "Right" : "Left";
            std::cout << "Debounce "<< dirString << ": " << speed << std::endl;

            return true;
        }
        return false;
    }
    // Returns a speed val 0->10
    int checkSpeed(int64_t ts){
        auto delta = timestamp() - ts;
        delta = delta > 500 ? 500 : delta;
        delta = delta < 0 ? 0 : delta;
        // get a speed val 0->10
        auto speed = (500 - delta)/50;
        return speed;
    }
    // Returns the timestamp in ms
    int64_t timestamp(){
        auto now = std::chrono::system_clock::now();
        auto now_ms = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
        auto ts = now_ms.time_since_epoch().count();
        //std::cout << ts << std::endl;
        return ts;
    }
    int debounceThreshold = 5;// ms
    //
    std::string outQueue = "";
    char lastDir = 'n'; 
    bool wasHit = false;
    int64_t ts;
    GPIO gpioDT;
    GPIO gpioCLK;
};
