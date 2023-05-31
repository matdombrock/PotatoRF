#pragma once
#include <iostream>

class Util{
public:
    // Output some text to the console
    template<typename T = std::string>
    static void echo(T input){
        std::cout << input << std::endl;
    }

    // Output some special (alert) text to the console
    static void alert(std::string str){
        echo("!!! "+str);
    }

    // Read a line of CLI input
    template<typename T = std::string>
    static T readLine(std::string prompt = "> "){
        T input;
        std::cout << prompt;
        std::cin >> input;
        return input;
    }

    // Returns the greater of 2 values
    template<typename T = int>
    static T greater(T a, T b){
        return a > b ? a : b;
    }

    // Returns the lesser of 2 values
    template<typename T = int>
    static T lesser(T a, T b){
        return a > b ? b : a;
    }

    // Returns a value bound between min and max
    template<typename T = int>
    static T bound(T val, T min, T max){
        val = Util::lesser(val, max);
        val = Util::greater(val, min);
        return val;
    }

    // Prints 1024 empty lines to the console
    static void clearScreen(){
        for(int i = 0; i < 1024; i++){
            echo("");
        }
    }

};
