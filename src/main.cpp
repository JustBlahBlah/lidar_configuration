#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

enum Commands {
	ENABLE_OUTPUT, //
	DISABLE_OUTPUT,
	OUTPUT_FORMAT, //
	FRAME_RATE,
	SAVE_SETTINGS, 
	BAUD_RATE, //
	RESTORE_FACTORY_SETTINGS
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
              << "  -c, --current <baud>        baud states for the cuurent baud rate of the lidar\n"
              << "  -o, --output <out_format>      out_format can be specified as: cm, m, mm\n"
              << "  -e, --enable    	enble output\n"
              << "  -b, --baud <baud_rate>	set lidar to specified baud rate\n"
	      << "  -f, --frame <frame_rate>	set lidar to specified frame rate\n"
	      << "  -s, --save		save current set of commands\n"
	      << "  -h, --help               Show this help message\n" 
              << std::endl;
}

int main(int argc, char** argv){
	std::cout << "Basic" << std::endl;
	std::vector<Commands> commands;
	int current_baud_rate = 0;
	for(int i = 1; i < argc; i++) {
		std::string a = argv[i];
		std::cout << a << std::endl;
		if((a == "--current" || a == "-c") && i + 1 < argc) {
			current_baud_rate = std::stoi(argv[++i]);
			std::cout << "cbr " << current_baud_rate << std::endl;
		} else if(a == "--enable" || a == "-e") {
			commands.push_back(Commands::ENABLE_OUTPUT);
		} else if((a == "--output" || a == "-o") && i + 1 < argc) {
			std::string argument = argv[++i];
			std::transform(argument.begin(), argument.end(), argument.begin(), 
               			[](unsigned char c){ return std::tolower(c); });
			
		 
			if(argument == "cm") of = Output_format::CM;
			else of = Output_format::UNDIFINED; 

			commands.push_back(Commands::OUTPUT_FORMAT);
		} else if((a == "--baud" || a == "-b") && i + 1 < argc) {
			int baud = std::stoi(argv[++i]);
			switch(baud) {
				case 256000:
					{
						std::cout << "baud 256";
						br = Baud_rate::BAUD_256000;
					} break;
				default : 
					{
						std::cout << "baud def";
						br = Baud_rate::UNDIFINED;
					} break;
			}

			commands.push_back(Commands::BAUD_RATE);
		} else if((a == "--frame" || a == "-f") && i + 1 < argc) {
			int frame = std::stoi(argv[++i]);
			switch(frame) {
				case 1000:
					{
						std::cout << "frame 1000";
						fr = Frame_rate::HZ_1000;
					} break;
				default : 
					{
						std::cout << "frame def";
						fr = Frame_rate::UNDIFINED;
					} break;
			}	

			commands.push_back(Commands::FRAME_RATE);
		} else if(a == "--save" || a == "-s"){
			commands.push_back(Commands::SAVE_SETTINGS);
		} else if((a == "--help" || a == "-h")) {
			printHelp();
			return 0;
		}
	}


	return 0;
}
