#pragma once

#include <cstddef>
#include <cstdint>
#include <iosfwd>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

class LinkingLoader {
private:
    // Properties
    struct TextRecord {
        std::uint32_t addr;
        std::string data_hex;
    };

    struct ModificationRecord {
        std::uint32_t addr;
        std::uint32_t length;
        char op;
        std::string symbol;
    };

    struct ObjectFileData {
        std::string name;
        std::uint32_t start = 0;
        std::uint32_t length = 0;
        std::vector<TextRecord> program;
        std::vector<ModificationRecord> modifications;
        bool has_entry = false;
        std::uint32_t entry_point = 0;
    };

    std::vector<std::uint8_t> memory_;
    std::unordered_map<std::string, std::uint32_t> symtab_;
    std::vector<std::string> symbol_order_;
    std::vector<ModificationRecord> reloc_info_;
    std::uint32_t prog_start_ = 0;
    std::uint32_t prog_length_ = 0;
    std::uint32_t entry_point_ = 0;

public:
    // Constructor
    explicit LinkingLoader(std::size_t mem_size = 0x10000);

private:
    // Helper functions
    std::tuple<std::string, std::uint32_t, std::uint32_t> parse_header(const std::string& line) const;
    TextRecord parse_text(const std::string& line) const;
    ModificationRecord parse_modification(const std::string& line) const;
    std::tuple<bool, std::uint32_t> parse_end(const std::string& line) const;

    ObjectFileData read_object_file(const std::string& filename) const;
    void load_program(const std::vector<TextRecord>& program,
                      std::uint32_t load_base,
                      std::uint32_t start_addr);
    void apply_relocations(std::uint32_t base_addr);

    static std::uint32_t parse_hex_u32(const std::string& text, const char* field_name);
    static std::uint8_t parse_hex_byte(const std::string& text, const char* field_name);
    static std::int64_t read_signed_be(const std::vector<std::uint8_t>& memory,
                                       std::uint32_t addr,
                                       std::uint32_t length);
    static void write_signed_be(std::vector<std::uint8_t>& memory,
                                std::uint32_t addr,
                                std::uint32_t length,
                                std::int64_t value);

public:
    // Methods
    void reset();
    std::uint32_t linking_load(const std::vector<std::string>& object_files,
                               std::uint32_t load_address = 0x1000);
    void display_memory(std::ostream& out, std::uint32_t start_addr, std::uint32_t length) const;
    void display_symbol_table(std::ostream& out) const;
};
