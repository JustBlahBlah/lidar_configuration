#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <lidar_helper.hpp>

enum Commands {
    ENABLE_OUTPUT,
    DISABLE_OUTPUT,
    OUTPUT_FORMAT,
    FRAME_RATE,
    SAVE_SETTINGS,
    BAUD_RATE,
    RESTORE_FACTORY_SETTINGS  // Added enum
};

enum class Frame_rate {
    UNDIFINED,
    HZ_1000
} fr = Frame_rate::UNDIFINED;

enum class Baud_rate {
    UNDIFINED,
    BAUD_256000
} br = Baud_rate::UNDIFINED;

enum class Output_format {
    UNDIFINED,
    CM
} of = Output_format::UNDIFINED;

static void printHelp() {
    std::cout << "Lidar Configuration Tool\n"
              << "Usage: sudo ./config [options]\n\n"
              << "Options:\n"
              << "  -c, --current <baud>       Specify current baud rate (Required)\n"
              << "  -o, --output <out_format>  Set output format (cm, m, mm)\n"
              << "  -e, --enable               Enable output\n"
              << "  -b, --baud <baud_rate>     Set lidar to specified baud rate\n"
              << "  -f, --frame <frame_rate>   Set lidar to specified frame rate\n"
              << "  -s, --save                 Save current settings\n"
              << "  -R, --reset                Restore factory settings (115200 baud, 100Hz)\n" // Help text
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

    // --- ARGUMENT PARSING ---
    for (int i = 1; i < argc; i++) {
        std::string a = argv[i];

        if ((a == "--current" || a == "-c") && i + 1 < argc) {
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

        } else if (a == "--reset" || a == "-R") { // New Flag
            std::cout << " -> Queueing Factory Reset" << std::endl;
            commands.push_back(Commands::RESTORE_FACTORY_SETTINGS);

        } else if (a == "--help" || a == "-h") {
            printHelp();
            return 0;
        }
    }

    // --- EXECUTION ---
    if (commands.empty()) {
        std::cerr << "No commands provided. Use -h for help.\n";
        return 0;
    }

    if (current_baud_rate == 0) {
        std::cerr << "[ERROR] You must specify current baud rate with -c <baud>" << std::endl;
        return 1;
    }

    int serial_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_SYNC);
    if (serial_fd < 0) {
        std::cerr << "Error opening port " << SERIAL_PORT << std::endl;
        return 1;
    }

    setup_serial(serial_fd, current_baud_rate);

    if (check_strict_connection(serial_fd)) {
        std::cout << "Connection confirmed at " << current_baud_rate << " baud." << std::endl;
    } else {
        std::cerr << " [ERROR] LiDAR not found at " << current_baud_rate << " baud." << std::endl;
        close(serial_fd);
        return 1;
    }

    for (const auto& cmd : commands) {
        switch (cmd) {
            case Commands::ENABLE_OUTPUT:
                send_command(serial_fd, "Enable Output", {0x5A, 0x05, 0x07, 0x01});
                break;

            case Commands::RESTORE_FACTORY_SETTINGS: // Logic for Reset
                std::cout << "\n[RESET] Restoring Factory Settings..." << std::endl;
                // Command: 5A 04 10 6E
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
                    case Baud_rate::BAUD_256000:
                        if (current_baud_rate != 256000) {
                            std::cout << "\n[UPGRADE] Switching to 256000 baud..." << std::endl;
                            send_command(serial_fd, "Set Baud 256000", {0x5A, 0x08, 0x06, 0x00, 0xE8, 0x03, 0x00});
                            send_command(serial_fd, "Save Settings", {0x5A, 0x04, 0x11, 0x6F});
                            close(serial_fd);
                            sleep(2); 

                            // Reconnect
                            serial_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY | O_SYNC);
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

    close(serial_fd);
    std::cout << "Configuration sequence finished." << std::endl;
    return 0;
}
