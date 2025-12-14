#ifndef LIDAR_HELPER
#define LIDAR_HELPER
#include <defines.hpp>

uint8_t calculate_checksum(const std::vector<uint8_t>& cmd);
int setup_serial(int fd, int baud_rate);
bool check_strict_connection(int fd);
void send_command(int fd, std::string name, std::vector<uint8_t> cmd);

#endif // LIDAR_HELPER
