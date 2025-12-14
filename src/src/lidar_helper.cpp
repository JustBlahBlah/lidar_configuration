#include <lidar_helper.hpp>

uint8_t calculate_checksum(const std::vector<uint8_t>& cmd) {
    int sum = 0;
    for (uint8_t byte : cmd) sum += byte;
    return (uint8_t)(sum & 0xFF);
}

int setup_serial(int fd, int baud_rate) {
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

bool check_strict_connection(int fd) {
    std::vector<uint8_t> buffer(128);
    int valid_headers = 0;
    for(int i=0; i<5; i++) { 
        int n = read(fd, buffer.data(), buffer.size());
        if (n > 0) {
            for(int j = 0; j < n - 1; j++) {
                if(buffer[j] == 0x59 && buffer[j+1] == 0x59) valid_headers++;
            }
        }
        if (valid_headers >= 2) return true; 
        usleep(100000); 
    }
    return false;
}


void send_command(int fd, std::string name, std::vector<uint8_t> cmd) {

    uint8_t cmd_id = cmd[2]; // The Command ID is always the 3rd byte

    cmd.push_back(calculate_checksum(cmd));


    tcflush(fd, TCIFLUSH); // Clear buffer before sending

    std::cout << " -> Sending " << name << "... " << std::flush;

    write(fd, cmd.data(), cmd.size());


    // Listen for response (Max 500ms)

    auto start = std::chrono::steady_clock::now();

    std::vector<uint8_t> accumulator;

    uint8_t buf[64];

    bool found = false;


    while (true) {

        auto now = std::chrono::steady_clock::now();

        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 500) break;


        int n = read(fd, buf, sizeof(buf));

        if (n > 0) {

            accumulator.insert(accumulator.end(), buf, buf + n);

            

            // Scan accumulator for response packet (Header 5A + Matching ID)

            if (accumulator.size() >= 4) {

                for (size_t i = 0; i < accumulator.size() - 3; ++i) {

                    if (accumulator[i] == 0x5A && accumulator[i+2] == cmd_id) {

                        int len = accumulator[i+1];

                        if (i + len <= accumulator.size()) {

                            // Verify checksum of response

                            int calc_sum = 0;

                            for (int j = 0; j < len - 1; j++) calc_sum += accumulator[i+j];

                            

                            if ((uint8_t)(calc_sum & 0xFF) == accumulator[i + len - 1]) {

                                std::cout << "RESPONSE RECEIVED: ";

                                for(int k=0; k<len; k++) printf("%02X ", accumulator[i+k]);

                                std::cout << std::endl;

                                found = true;

                                goto done; // Break out of nested loops

                            }

                        }

                    }

                }

            }

        }

        usleep(1000);

    }


    done:

    if (!found) {

        std::cout << "NO RESPONSE (Timed out or buried in noise)" << std::endl;

    }

    

    usleep(100000); // Small safety pause

} 
