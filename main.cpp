#include <iostream>

int main() {
    bool ledOn = false;
    char command{};
    std::cout << "Enter command (t = toggle led, q = quit): ";
    
     while(std::cin >> command) {
        if(command == 't') {
            ledOn = !ledOn;
            std::cout << "LED is now " << (ledOn ? "ON" : "OFF") << std::endl;
        } else if(command == 'q') {
            break;
        } else {
            std::cout << "Unknown command" << std::endl;
        }
        std::cout << "Enter command (t = toggle led, q = quit): ";
    }
    return 0;
}