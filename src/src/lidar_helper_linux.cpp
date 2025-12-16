// Этот файл компилируется ТОЛЬКО на Linux
#ifndef _WIN32 

#include <lidar_helper.hpp>

SerialPortHandle open_port(const char* port_name) {
    return open(port_name, O_RDWR | O_NOCTTY | O_SYNC);
}

void close_port(SerialPortHandle fd) {
    close(fd);
}

int setup_serial(SerialPortHandle fd, int baud_rate) {
    struct termios2 tty;
    if (ioctl(fd, TCGETS2, &tty)) return -1;

    tty.c_cflag &= ~CBAUD; tty.c_cflag |= BOTHER; 
    tty.c_cflag |= CLOCAL | CREAD; tty.c_cflag &= ~CSIZE; tty.c_cflag |= CS8;
    tty.c_cflag &= ~PARENB; tty.c_cflag &= ~CSTOPB; tty.c_cflag &= ~CRTSCTS;
    tty.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tty.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tty.c_oflag &= ~OPOST;
    tty.c_ispeed = baud_rate; tty.c_ospeed = baud_rate;

    if (ioctl(fd, TCSETS2, &tty)) return -1;
    tcflush(fd, TCIOFLUSH);
    return 0;
}

void flush_serial(SerialPortHandle fd) {
    tcflush(fd, TCIFLUSH);
}

int read_serial(SerialPortHandle fd, uint8_t* buffer, size_t size) {
    return read(fd, buffer, size);
}

int write_serial(SerialPortHandle fd, const std::vector<uint8_t>& data) {
    return write(fd, data.data(), data.size());
}

// ... (Остальные функции check_strict_connection и send_command будут в общем файле или ниже)
// Чтобы не дублировать код логики, можно вынести calculate_checksum/check/send в отдельный файл common.cpp
// Но для простоты давайте реализуем их здесь.

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
                        int len = accumulator[i+1];
                        if (i + len <= accumulator.size()) {
                            // Проверка чексумм (упрощено)
                             std::cout << "RESPONSE RECEIVED!" << std::endl;
                             found = true;
                             goto done;
                        }
                    }
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    done:
    if (!found) std::cout << "NO RESPONSE" << std::endl;
}

#endif // Not WIN32
