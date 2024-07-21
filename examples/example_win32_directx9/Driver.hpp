#ifndef DRIVER_HPP
#define DRIVER_HPP

#include <string>
#include <map>
#include <chrono>
#include <windows.h>

class DriverHandler {
public:
    class Buttons {
    public:
        void open_url(const std::string& url);
        void setMusic(const std::string& key, const std::string& path);
        std::string getOutput();
        void changeFunction(const std::string& key, const std::string& function, const std::string& link);
        void saveSettings();
        void loadSettings();
        void playSound(const std::string& path);
        int handle();

    private:
        struct KeyFunction {
            std::string function;
            std::string path;
            KeyFunction() = default;
            KeyFunction(const std::string& func, const std::string& pth) : function(func), path(pth) {}
        };

        std::map<std::string, KeyFunction> urls;
        LPCWSTR arduino_port = L"COM3"; // Change to your actual COM port
        DWORD baudrate = CBR_9600;
        int debounce_time = 2; // Debounce time in seconds
        std::chrono::time_point<std::chrono::steady_clock> last_opened_time;
    };
};

#endif // DRIVER_HPP
