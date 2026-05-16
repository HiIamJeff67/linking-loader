#include "linking_loader.hpp"

#include <cstdint>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::uint32_t parse_hex_progaddr(const std::string& text) {
    std::size_t idx = 0;
    unsigned long value = 0;
    try {
        value = std::stoul(text, &idx, 16);
    } catch (const std::exception&) {
        throw std::runtime_error("Failed to parse PROGADDR (hex): " + text);
    }

    if (idx != text.size()) {
        throw std::runtime_error("Invalid PROGADDR format (hex): " + text);
    }
    if (value > 0xFFFFFFFFUL) {
        throw std::runtime_error("PROGADDR out of 32-bit range: " + text);
    }

    return static_cast<std::uint32_t>(value);
}

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
              << " <PROGADDR_HEX> <obj1> [obj2 ...] [--dump-start 0x4000] [--dump-length 256] [--report output.txt]\n";
}

void ensure_parent_dir(const std::string& path) {
    const std::filesystem::path p(path);
    const auto parent = p.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

void write_report_file(const LinkingLoader& loader,
                       std::uint32_t dump_start,
                       std::uint32_t dump_length,
                       const std::string& report_path) {
    ensure_parent_dir(report_path);

    std::ofstream report(report_path, std::ios::out | std::ios::trunc);
    if (!report) {
        throw std::runtime_error("Cannot write report file: " + report_path);
    }

    report << "=== ESTAB ===\n";
    loader.write_estab(report);

    report << "\n=== MEMORY ===\n";
    loader.write_memory_dump(report, dump_start, dump_length);
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    std::uint32_t progaddr = 0;
    std::uint32_t dump_start = 0;
    std::uint32_t dump_length = 0;
    std::string report_path = "output.txt";
    bool report_overridden = false;

    std::vector<std::string> object_files;

    try {
        progaddr = parse_hex_progaddr(argv[1]);
        dump_start = progaddr;

        for (int i = 2; i < argc; ++i) {
            const std::string arg = argv[i];

            if (arg == "--dump-start") {
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
                report_overridden = true;
            } else if (!arg.empty() && arg[0] == '-') {
                throw std::runtime_error("Unknown option: " + arg);
            } else {
                object_files.push_back(arg);
            }
        }

        if (object_files.empty()) {
            throw std::runtime_error("At least one object file is required");
        }

        LinkingLoader loader;
        const std::uint32_t entry = loader.linking_load(progaddr, object_files);

        std::uint32_t effective_dump_length = dump_length;
        if (effective_dump_length == 0) {
            effective_dump_length = loader.memory_space_bytes();
        }

        if (report_overridden) {
            write_report_file(loader, dump_start, effective_dump_length, report_path);
            std::cout << "Linking completed. PROGADDR=0x"
                      << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << progaddr
                      << ", ENTRY=0x" << std::setw(4) << entry
                      << ". Report written to: " << report_path << '\n';
            return 0;
        }

        write_report_file(loader, dump_start, effective_dump_length, report_path);
        std::cout << "Linking completed. PROGADDR=0x"
                  << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << progaddr
                  << ", ENTRY=0x" << std::setw(4) << entry
                  << ". Report written to: " << report_path << '\n';
    } catch (const std::exception& e) {
        std::cerr << "Execution failed: " << e.what() << '\n';
        return 1;
    }

    return 0;
}
