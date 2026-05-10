#include "common.h"
#include <map>
#include <functional>

void print_help() {
    std::cout << "Usage: mini_ffmpeg <command> <args>\n"
              << "Commands:\n"
              << "  probe <input>                                 - Probe media info\n"
              << "  remux <input> <output>                         - Remux to another container\n"
              << "  transcode <input> <output> [--scale WxH]       - Transcode video with optional scaling\n"
              << "  resample <input> <output> [--ar RATE] [--ac CH] - Resample audio\n";
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_help();
        return 1;
    }

    std::string command = argv[1];
    Config config;
    config.input_file = argv[2];

    // 解析通用和特定参数
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--scale" && i + 1 < argc) {
            config.scale = argv[++i];
        } else if (arg == "--ar" && i + 1 < argc) {
            config.sample_rate = std::stoi(argv[++i]);
        } else if (arg == "--ac" && i + 1 < argc) {
            config.channels = std::stoi(argv[++i]);
        } else if (config.output_file.empty()) {
            config.output_file = arg;
        }
    }

    std::map<std::string, std::function<int(const Config&)>> commands = {
        {"probe", cmd_probe},
        {"remux", cmd_remux},
        {"transcode", cmd_transcode},
        {"resample", cmd_resample}
    };

    if (commands.find(command) == commands.end()) {
        std::cerr << "Unknown command: " << command << "\n";
        print_help();
        return 1;
    }

    int ret = commands[command](config);
    if (ret < 0) {
        std::cerr << "Command '" << command << "' failed with exit code " << ret << "\n";
        return 1;
    }

    std::cout << "Command '" << command << "' finished successfully.\n";
    return 0;
}
