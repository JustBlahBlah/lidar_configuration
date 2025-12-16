#ifndef LIDAR_HELPER
#define LIDAR_HELPER
#include <defines.hpp>

// Кроссплатформенные функции управления портом
SerialPortHandle open_port(const char* port_name);
void close_port(SerialPortHandle fd);
int setup_serial(SerialPortHandle fd, int baud_rate);
void flush_serial(SerialPortHandle fd);
int read_serial(SerialPortHandle fd, uint8_t* buffer, size_t size);
int write_serial(SerialPortHandle fd, const std::vector<uint8_t>& data);

// Логика лидара
uint8_t calculate_checksum(const std::vector<uint8_t>& cmd);
bool check_strict_connection(SerialPortHandle fd);
void send_command(SerialPortHandle fd, std::string name, std::vector<uint8_t> cmd);

#endif // LIDAR_HELPER
