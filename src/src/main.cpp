#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <thread> // Для sleep_for
#include <lidar_helper.hpp>

enum Commands {
    ENABLE_OUTPUT,
    DISABLE_OUTPUT,
    OUTPUT_FORMAT,
    FRAME_RATE,
    SAVE_SETTINGS,
    BAUD_RATE,
    RESTORE_FACTORY_SETTINGS
};

enum class Frame_rate {
    UNDIFINED,
    HZ_10,
    HZ_1000
} fr = Frame_rate::UNDIFINED;

enum class Baud_rate {
    UNDIFINED,
    BAUD_115200,
    BAUD_256000
} br = Baud_rate::UNDIFINED;

enum class Output_format {
    UNDIFINED,
    CM
} of = Output_format::UNDIFINED;

static void printHelp() {
    std::cout << "Lidar Configuration Tool\n"
              << "Usage: ./config [options]\n\n"
              << "Options:\n"
              << "  -p, --port <name>          Specify serial port (e.g. COM3 or /dev/ttyUSB0)\n"
              << "  -c, --current <baud>       Specify current baud rate (Required)\n"
              << "  -o, --output <out_format>  Set output format (cm)\n"
              << "  -e, --enable               Enable output\n"
              << "  -b, --baud <baud_rate>     Set lidar to specified baud rate\n"
              << "  -f, --frame <frame_rate>   Set lidar to specified frame rate\n"
              << "  -s, --save                 Save current settings\n"
              << "  -r, --reset                Restore factory settings\n"
              << "  -h, --help                 Show this help message\n"
              << std::endl;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        printHelp();
        return 0;
    }

    std::cout << "Starting Lidar Configuration Tool..." << std::endl;
    std::vector<Commands> commands;
    int current_baud_rate = 0;
    
    // Используем порт по умолчанию из defines.hpp (COM3 для Win, /dev/ttyUSB0 для Linux)
    std::string port_name = DEFAULT_PORT; 

    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];

        if ((a == "--port" || a == "-p") && i + 1 < argc) {
            port_name = argv[++i];
            std::cout << " -> Port set to: " << port_name << std::endl;

        } else if ((a == "--current" || a == "-c") && i + 1 < argc) {
            current_baud_rate = std::stoi(argv[++i]);
            std::cout << " -> Current Baud Rate: " << current_baud_rate << std::endl;

        } else if (a == "--enable" || a == "-e") {
            commands.push_back(Commands::ENABLE_OUTPUT);

        } else if ((a == "--output" || a == "-o") && i + 1 < argc) {
            std::string argument = argv[++i];
            std::transform(argument.begin(), argument.end(), argument.begin(),
                           [](unsigned char c) { return std::tolower(c); });

            if (argument == "cm") of = Output_format::CM;
            else of = Output_format::UNDIFINED;

            commands.push_back(Commands::OUTPUT_FORMAT);

        } else if ((a == "--baud" || a == "-b") && i + 1 < argc) {
            int baud = std::stoi(argv[++i]);
            switch (baud) {
                case 115200:
                    std::cout << " -> Target Baud: 115200" << std::endl;
                    br = Baud_rate::BAUD_115200;
                    break;
                case 256000:
                    std::cout << " -> Target Baud: 256000" << std::endl;
                    br = Baud_rate::BAUD_256000;
                    break;
                default:
                    std::cout << " -> Baud Rate Undefined/Unsupported" << std::endl;
                    br = Baud_rate::UNDIFINED;
                    break;
            }
            commands.push_back(Commands::BAUD_RATE);

        } else if ((a == "--frame" || a == "-f") && i + 1 < argc) {
            int frame = std::stoi(argv[++i]);
            switch (frame) {
                case 10:
                    std::cout << " -> Target Frame Rate: 10Hz" << std::endl;
                    fr = Frame_rate::HZ_10;
                    break;
                case 1000:
                    std::cout << " -> Target Frame Rate: 1000Hz" << std::endl;
                    fr = Frame_rate::HZ_1000;
                    break;
                default:
                    std::cout << " -> Frame Rate Undefined/Unsupported" << std::endl;
                    fr = Frame_rate::UNDIFINED;
                    break;
            }
            commands.push_back(Commands::FRAME_RATE);

        } else if (a == "--save" || a == "-s") {
            commands.push_back(Commands::SAVE_SETTINGS);

        } else if (a == "--reset" || a == "-r") {
            std::cout << " -> Queueing Factory Reset" << std::endl;
            commands.push_back(Commands::RESTORE_FACTORY_SETTINGS);

        } else if (a == "--help" || a == "-h") {
            printHelp();
            return 0;
        }
    }

    if (commands.empty()) {
        std::cerr << "No commands provided. Use -h for help.\n";
        return 0;
    }

    if (current_baud_rate == 0) {
        std::cerr << "[ERROR] You must specify current baud rate with -c <baud>" << std::endl;
        return 1;
    }

    // --- ОТКРЫТИЕ ПОРТА ---
    // Используем универсальную функцию open_port
    SerialPortHandle serial_fd = open_port(port_name.c_str());

    // Проверяем ошибку через макрос из defines.hpp
    if (serial_fd == INVALID_PORT_VALUE) {
        std::cerr << "Error opening port " << port_name << std::endl;
        return 1;
    }

    std::cout << "Setup serial..." << std::endl;
    // Настраиваем скорость (termios для Linux, DCB для Windows внутри функции)
    if (setup_serial(serial_fd, current_baud_rate) != 0) {
        std::cerr << "Error setting up serial port params." << std::endl;
        close_port(serial_fd);
        return 1;
    }
    
    std::cout << "Checking connection..." << std::endl;
    if (check_strict_connection(serial_fd)) {
        std::cout << "Connection confirmed at " << current_baud_rate << " baud." << std::endl;
    } else {
        std::cerr << " [ERROR] LiDAR not found at " << current_baud_rate << " baud." << std::endl;
        close_port(serial_fd);
        return 1;
    }

    std::cout << "Executing commands..." << std::endl;
    for (const auto& cmd : commands) {
        switch (cmd) {
            case Commands::ENABLE_OUTPUT:
                send_command(serial_fd, "Enable Output", {0x5A, 0x05, 0x07, 0x01});
                break;

            case Commands::RESTORE_FACTORY_SETTINGS:
                std::cout << "\n[RESET] Restoring Factory Settings..." << std::endl;
                send_command(serial_fd, "Factory Reset", {0x5A, 0x04, 0x10, 0x6E});
                std::cout << "LiDAR will reset to 115200 baud / 100Hz." << std::endl;
                break;

            case Commands::OUTPUT_FORMAT:
                switch (of) {
                    case Output_format::CM:
                        send_command(serial_fd, "Set Format (CM)", {0x5A, 0x05, 0x05, 0x01});
                        break;
                    default:
                        break;
                }
                break;

            case Commands::BAUD_RATE:
                switch (br) {
                    case Baud_rate::BAUD_115200:
                        if (current_baud_rate != 115200) {
                            std::cout << "\n[DOWNGRADE] Switching to 115200 baud..." << std::endl;
                            send_command(serial_fd, "Set Baud 115200", {0x5A, 0x08, 0x06, 0x00, 0xC2, 0x01, 0x00});
                            send_command(serial_fd, "Save Settings", {0x5A, 0x04, 0x11, 0x6F});
                            
                            // Закрываем порт
                            close_port(serial_fd);
                            std::cout << "Waiting for reboot..." << std::endl;
                            
                            // Кроссплатформенная пауза
                            std::this_thread::sleep_for(std::chrono::seconds(2));

                            // Переподключаемся
                            serial_fd = open_port(port_name.c_str());
                            if (serial_fd == INVALID_PORT_VALUE) {
                                std::cerr << "Failed to reopen port." << std::endl;
                                return 1;
                            }
                            setup_serial(serial_fd, 115200);
                            
                            if (!check_strict_connection(serial_fd)) {
                                std::cerr << " [ERROR] Failed to reconnect at 115200." << std::endl;
                                return 1;
                            }
                            current_baud_rate = 115200; 
                            std::cout << " [SUCCESS] Downgrade complete." << std::endl;
                        } else {
                            std::cout << "Already at 115200 baud." << std::endl;
                        }
                        break;
                        
                    case Baud_rate::BAUD_256000:
                        if (current_baud_rate != 256000) {
                            std::cout << "\n[UPGRADE] Switching to 256000 baud..." << std::endl;
                            send_command(serial_fd, "Set Baud 256000", {0x5A, 0x08, 0x06, 0x00, 0xE8, 0x03, 0x00});
                            send_command(serial_fd, "Save Settings", {0x5A, 0x04, 0x11, 0x6F});
                            
                            close_port(serial_fd);
                            std::cout << "Waiting for reboot..." << std::endl;
                            std::this_thread::sleep_for(std::chrono::seconds(2)); 

                            // Переподключаемся
                            serial_fd = open_port(port_name.c_str());
                            if (serial_fd == INVALID_PORT_VALUE) {
                                std::cerr << "Failed to reopen port." << std::endl;
                                return 1;
                            }
                            setup_serial(serial_fd, 256000);
                            
                            if (!check_strict_connection(serial_fd)) {
                                std::cerr << " [ERROR] Failed to reconnect at 256000." << std::endl;
                                return 1;
                            }
                            current_baud_rate = 256000; 
                            std::cout << " [SUCCESS] Upgrade complete." << std::endl;
                        } else {
                            std::cout << "Already at 256000 baud." << std::endl;
                        }
                        break;
                    default:
                        break;
                }
                break;

            case Commands::FRAME_RATE:
                switch (fr) {
                    case Frame_rate::HZ_10:
                         send_command(serial_fd, "Set 10Hz", {0x5A, 0x06, 0x03, 0x0A, 0x00});
                         break;
                    case Frame_rate::HZ_1000:
                         send_command(serial_fd, "Set 1000Hz", {0x5A, 0x06, 0x03, 0xE8, 0x03});
                         break;
                    default:
                         break;
                }
                break;

            case Commands::SAVE_SETTINGS:
                send_command(serial_fd, "Save Settings", {0x5A, 0x04, 0x11, 0x6F});
                break;
                
            default:
                break;
        }
    }

    close_port(serial_fd);
    std::cout << "Configuration sequence finished." << std::endl;
    return 0;
}
