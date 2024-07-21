#include "Driver.hpp"
#include <shellapi.h> // For ShellExecuteA
#include <mmsystem.h> // For PlaySoundA
#include <fstream>
#include <iostream>
#include <thread> // For std::this_thread::sleep_for

#pragma comment(lib, "Winmm.lib") // Link with Winmm.lib for PlaySoundA

void DriverHandler::Buttons::open_url(const std::string& url) {
    std::cout << "Attempting to open URL: " << url << std::endl;
    HINSTANCE result = ShellExecuteA(nullptr, "open", url.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    if ((int)result <= 32) {
        std::cerr << "Error: Unable to open URL. ShellExecuteA failed with code " << (int)result << std::endl;
    }
    else {
        std::cout << "Successfully opened URL: " << url << std::endl;
    }
}

void DriverHandler::Buttons::playSound(const std::string& path) {
    std::cout << "Attempting to play sound: " << path << std::endl;
    if (!PlaySoundA(path.c_str(), nullptr, SND_FILENAME | SND_ASYNC)) {
        std::cerr << "Error: Unable to play sound." << std::endl;
    }
    else {
        std::cout << "Successfully playing sound: " << path << std::endl;
    }
}

int DriverHandler::Buttons::handle() {
    HANDLE hSerial = CreateFileW(arduino_port, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            std::cerr << "Error: Serial port does not exist." << std::endl;
        }
        else {
            std::cerr << "Error: Unable to open serial port." << std::endl;
        }
        return 1;
    }

    DCB dcbSerialParams = { 0 };
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error: Getting device state." << std::endl;
        CloseHandle(hSerial);
        return 1;
    }

    dcbSerialParams.BaudRate = baudrate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    if (!SetCommState(hSerial, &dcbSerialParams)) {
        std::cerr << "Error: Setting device parameters." << std::endl;
        CloseHandle(hSerial);
        return 1;
    }

    COMMTIMEOUTS timeouts = { 0 };
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    if (!SetCommTimeouts(hSerial, &timeouts)) {
        std::cerr << "Error: Setting timeouts." << std::endl;
        CloseHandle(hSerial);
        return 1;
    }

    last_opened_time = std::chrono::steady_clock::now() - std::chrono::seconds(debounce_time);

    char buffer[256];
    DWORD bytesRead;
    while (true) {
        if (ReadFile(hSerial, buffer, sizeof(buffer) - 1, &bytesRead, NULL)) {
            if (bytesRead > 0) {
                buffer[bytesRead] = '\0';
                std::string line(buffer);
                line.erase(line.find_last_not_of(" \n\r\t") + 1);
                auto current_time = std::chrono::steady_clock::now();
                std::chrono::duration<double> elapsed_seconds = current_time - last_opened_time;

                if (urls.find(line) != urls.end() && elapsed_seconds.count() > debounce_time) {
                    std::cout << line << ". Opening web page..." << std::endl;
                    open_url(urls[line].path);
                    last_opened_time = current_time;
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    CloseHandle(hSerial);
    return 0;
}

void DriverHandler::Buttons::setMusic(const std::string& key, const std::string& path) {
    urls[key] = KeyFunction("music", path);
}

std::string DriverHandler::Buttons::getOutput() {
    return "Output";
}

void DriverHandler::Buttons::changeFunction(const std::string& key, const std::string& function, const std::string& link) {
    urls[key] = KeyFunction(function, link);
    saveSettings();
}

void DriverHandler::Buttons::saveSettings() {
    std::ofstream settings_file("settings.ini");
    if (settings_file.is_open()) {
        for (const auto& entry : urls) {
            settings_file << entry.first << " " << entry.second.function << " " << entry.second.path << std::endl;
        }
        settings_file.close();
    }
}

void DriverHandler::Buttons::loadSettings() {
    std::ifstream settings_file("settings.ini");
    if (settings_file.is_open()) {
        std::string key, function, path;
        while (settings_file >> key >> function >> path) {
            urls[key] = KeyFunction(function, path);
        }
        settings_file.close();
    }
}
