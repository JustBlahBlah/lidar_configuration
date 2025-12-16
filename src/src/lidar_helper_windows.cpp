// Этот файл компилируется ТОЛЬКО на Windows
#ifdef _WIN32 

#include <lidar_helper.hpp>

SerialPortHandle open_port(const char* port_name) {
    // Windows требует префикс \\.\ для портов выше COM9
    std::string path = std::string("\\\\.\\") + port_name;
    return CreateFileA(path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
}

void close_port(SerialPortHandle fd) {
    CloseHandle(fd);
}

int setup_serial(SerialPortHandle fd, int baud_rate) {
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(fd, &dcbSerialParams)) return -1;
    
    dcbSerialParams.BaudRate = baud_rate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    
    if (!SetCommState(fd, &dcbSerialParams)) return -1;

    // Важные тайминги, иначе ReadFile зависнет навсегда
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50; 
    timeouts.ReadTotalTimeoutConstant = 50;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    
    if (!SetCommTimeouts(fd, &timeouts)) return -1;
    return 0;
}

void flush_serial(SerialPortHandle fd) {
    PurgeComm(fd, PURGE_RXCLEAR | PURGE_TXCLEAR);
}

int read_serial(SerialPortHandle fd, uint8_t* buffer, size_t size) {
    DWORD bytesRead;
    if (!ReadFile(fd, buffer, (DWORD)size, &bytesRead, NULL)) return -1;
    return (int)bytesRead;
}

int write_serial(SerialPortHandle fd, const std::vector<uint8_t>& data) {
    DWORD bytesWritten;
    if (!WriteFile(fd, data.data(), (DWORD)data.size(), &bytesWritten, NULL)) return -1;
    return (int)bytesWritten;
}

// === ЛОГИКА (Копия из Linux версии, т.к. логика одинаковая) ===

uint8_t calculate_checksum(const std::vector<uint8_t>& cmd) {
    int sum = 0;
    for (uint8_t byte : cmd) sum += byte;
    return (uint8_t)(sum & 0xFF);
}

bool check_strict_connection(SerialPortHandle fd) {
    std::vector<uint8_t> buffer(128);
    int valid_headers = 0;
    for(int i=0; i<5; i++) { 
        int n = read_serial(fd, buffer.data(), buffer.size());
        if (n > 0) {
            for(int j = 0; j < n - 1; j++) {
                if(buffer[j] == 0x59 && buffer[j+1] == 0x59) valid_headers++;
            }
        }
        if (valid_headers >= 2) return true; 
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    return false;
}

void send_command(SerialPortHandle fd, std::string name, std::vector<uint8_t> cmd) {
    uint8_t cmd_id = cmd[2];
    cmd.push_back(calculate_checksum(cmd));

    flush_serial(fd);
    std::cout << " -> Sending " << name << "... " << std::flush;
    write_serial(fd, cmd);

    auto start = std::chrono::steady_clock::now();
    std::vector<uint8_t> accumulator;
    uint8_t buf[64];
    bool found = false;

    while (true) {
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 500) break;

        int n = read_serial(fd, buf, sizeof(buf));
        if (n > 0) {
            accumulator.insert(accumulator.end(), buf, buf + n);
            if (accumulator.size() >= 4) {
                for (size_t i = 0; i < accumulator.size() - 3; ++i) {
                    if (accumulator[i] == 0x5A && accumulator[i+2] == cmd_id) {
                         std::cout << "RESPONSE RECEIVED!" << std::endl;
                         found = true;
                         goto done;
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    done:
    if (!found) std::cout << "NO RESPONSE" << std::endl;
}

#endif // WIN32
