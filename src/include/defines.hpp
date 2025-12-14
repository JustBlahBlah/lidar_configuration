#ifndef DEFINES
#define DEFINES

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <cstdint>
#include <iomanip>
#include <chrono>

#ifndef BOTHER
  #define BOTHER 0010000
#endif

struct termios2 {
    tcflag_t c_iflag; tcflag_t c_oflag; tcflag_t c_cflag; tcflag_t c_lflag;
    cc_t c_line; cc_t c_cc[19]; speed_t c_ispeed; speed_t c_ospeed;
};

#define TCGETS2 _IOR('T', 0x2A, struct termios2)
#define TCSETS2 _IOW('T', 0x2B, struct termios2)

const char* SERIAL_PORT = "/dev/ttyUSB0";

#endif //DEFINES
