#include "linking_loader.hpp"

#include <cstdint>
#include <exception>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::uint32_t parse_cli_u32(const std::string& text, const char* name) {
    std::size_t idx = 0;
    unsigned long value = 0;
    try {
        value = std::stoul(text, &idx, 0);
    } catch (const std::exception&) {
        throw std::runtime_error(std::string("Failed to parse argument ") + name + ": " + text);
    }

    if (idx != text.size()) {
        throw std::runtime_error(std::string("Invalid argument format for ") + name + ": " + text);
    }

    if (value > 0xFFFFFFFFUL) {
        throw std::runtime_error(std::string("Argument out of 32-bit range for ") + name + ": " + text);
    }

    return static_cast<std::uint32_t>(value);
}

void print_usage(const char* argv0) {
    std::cerr << "Usage: " << argv0
              << " <obj1> [obj2 ...] [--load-address 0x1000] [--dump-start 0x1000] [--dump-length 64] [--report output.txt]\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    std::vector<std::string> object_files;
    std::uint32_t load_address = 0x1000;
    std::uint32_t dump_start = 0x1000;
    std::uint32_t dump_length = 64;
    std::string report_path = "output.txt";

    try {
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];

            if (arg == "--load-address") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--load-address is missing a value");
                }
                load_address = parse_cli_u32(argv[++i], "--load-address");
            } else if (arg == "--dump-start") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--dump-start is missing a value");
                }
                dump_start = parse_cli_u32(argv[++i], "--dump-start");
            } else if (arg == "--dump-length") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--dump-length is missing a value");
                }
                dump_length = parse_cli_u32(argv[++i], "--dump-length");
            } else if (arg == "--report") {
                if (i + 1 >= argc) {
                    throw std::runtime_error("--report is missing a value");
                }
                report_path = argv[++i];
            } else if (!arg.empty() && arg[0] == '-') {
                throw std::runtime_error("Unknown option: " + arg);
            } else {
                object_files.push_back(arg);
            }
        }

        if (object_files.empty()) {
            print_usage(argv[0]);
            return 1;
        }

        LinkingLoader loader;
        const auto entry = loader.linking_load(object_files, load_address);

        std::ofstream report(report_path, std::ios::out | std::ios::trunc);
        if (!report) {
            throw std::runtime_error("Cannot write report file: " + report_path);
        }

        report << "=== Linking Summary ===\n";
        report << "Object count: " << object_files.size() << '\n';
        report << "Load base   : 0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(4)
               << load_address << '\n';
        report << "Entry point : 0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(4)
               << entry << "\n\n";

        loader.display_symbol_table(report);

        report << "Memory Window\n";
        report << "-------------\n";
        report << "Start : 0x" << std::uppercase << std::hex << std::setfill('0') << std::setw(4)
               << dump_start << '\n';
        report << "Length: " << std::dec << dump_length << " bytes\n\n";
        loader.display_memory(report, dump_start, dump_length);

        std::cout << "Linking completed. Entry=0x"
                  << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << entry
                  << ". Report written to: " << report_path << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Execution failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
