#include "linking_loader.hpp"

#include <algorithm>
#include <cctype>
#include <exception>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>


namespace {

std::string slice_field(const std::string& line, std::size_t start, std::size_t len) {
    if (start >= line.size()) {
        return "";
    }
    return line.substr(start, len);
}

}  // namespace

LinkingLoader::LinkingLoader(std::size_t mem_size) : memory_(mem_size, 0) {}

void LinkingLoader::reset() {
    symtab_.clear();
    symbol_order_.clear();
    reloc_info_.clear();
    std::fill(memory_.begin(), memory_.end(), 0);
    prog_start_ = 0;
    prog_length_ = 0;
    entry_point_ = 0;
}

std::uint32_t LinkingLoader::parse_hex_u32(const std::string& text, const char* field_name) {
    if (text.empty()) {
        throw std::runtime_error(std::string("Empty hexadecimal field: ") + field_name);
    }
    for (char c : text) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            std::ostringstream oss;
            oss << "Non-hex character in " << field_name << ": '" << text << "'";
            throw std::runtime_error(oss.str());
        }
    }

    unsigned long value = 0;
    try {
        value = std::stoul(text, nullptr, 16);
    } catch (const std::exception&) {
        std::ostringstream oss;
        oss << "Failed to parse hexadecimal field " << field_name << ": '" << text << "'";
        throw std::runtime_error(oss.str());
    }

    if (value > std::numeric_limits<std::uint32_t>::max()) {
        std::ostringstream oss;
        oss << "Field exceeds 32-bit range " << field_name << ": '" << text << "'";
        throw std::runtime_error(oss.str());
    }

    return static_cast<std::uint32_t>(value);
}

std::uint8_t LinkingLoader::parse_hex_byte(const std::string& text, const char* field_name) {
    const std::uint32_t value = parse_hex_u32(text, field_name);
    if (value > 0xFFU) {
        std::ostringstream oss;
        oss << "Field exceeds byte range " << field_name << ": '" << text << "'";
        throw std::runtime_error(oss.str());
    }
    return static_cast<std::uint8_t>(value);
}

std::tuple<std::string, std::uint32_t, std::uint32_t>
LinkingLoader::parse_header(const std::string& line) const {
    std::string name = slice_field(line, 1, 6);
    while (!name.empty() && std::isspace(static_cast<unsigned char>(name.back()))) {
        name.pop_back();
    }

    const std::uint32_t start = parse_hex_u32(slice_field(line, 7, 6), "H.start");
    const std::uint32_t length = parse_hex_u32(slice_field(line, 13, 6), "H.length");
    return {name, start, length};
}

LinkingLoader::TextRecord LinkingLoader::parse_text(const std::string& line) const {
    const std::uint32_t addr = parse_hex_u32(slice_field(line, 1, 6), "T.addr");
    const std::uint8_t size = parse_hex_byte(slice_field(line, 7, 2), "T.size");

    std::string data = slice_field(line, 9, static_cast<std::size_t>(size) * 2);

    for (char c : data) {
        if (!std::isxdigit(static_cast<unsigned char>(c))) {
            std::ostringstream oss;
            oss << "Non-hex character in T.data: '" << data << "'";
            throw std::runtime_error(oss.str());
        }
    }

    return {addr, data};
}

LinkingLoader::ModificationRecord LinkingLoader::parse_modification(const std::string& line) const {
    const std::uint32_t addr = parse_hex_u32(slice_field(line, 1, 6), "M.addr");
    const std::uint32_t length = parse_hex_u32(slice_field(line, 7, 2), "M.length");
    const char op = line.size() > 9 ? line[9] : '\0';
    const std::string symbol = slice_field(line, 10, line.size());

    if (symbol.empty()) {
        throw std::runtime_error("M.symbol cannot be empty");
    }

    return {addr, length, op, symbol};
}

std::tuple<bool, std::uint32_t> LinkingLoader::parse_end(const std::string& line) const {
    if (line.size() > 1) {
        return {true, parse_hex_u32(slice_field(line, 1, 6), "E.entry")};
    }
    return {false, 0};
}

LinkingLoader::ObjectFileData LinkingLoader::read_object_file(const std::string& filename) const {
    std::ifstream in(filename);
    if (!in) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    ObjectFileData obj;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        if (line.empty()) {
            continue;
        }

        switch (line[0]) {
            case 'H': {
                auto [name, start, length] = parse_header(line);
                obj.name = name;
                obj.start = start;
                obj.length = length;
                break;
            }
            case 'T':
                obj.program.push_back(parse_text(line));
                break;
            case 'M':
                obj.modifications.push_back(parse_modification(line));
                break;
            case 'E': {
                auto [has_entry, entry] = parse_end(line);
                if (has_entry) {
                    obj.has_entry = true;
                    obj.entry_point = entry;
                }
                break;
            }
            default:
                break;
        }
    }

    return obj;
}

void LinkingLoader::load_program(const std::vector<TextRecord>& program,
                                 std::uint32_t load_base,
                                 std::uint32_t start_addr) {
    for (const auto& record : program) {
        const std::uint64_t actual_addr64 =
            static_cast<std::uint64_t>(load_base) + record.addr - start_addr;
        if (actual_addr64 > std::numeric_limits<std::uint32_t>::max()) {
            throw std::runtime_error("Computed load address exceeds 32-bit range");
        }

        const auto actual_addr = static_cast<std::uint32_t>(actual_addr64);
        for (std::size_t i = 0; i < record.data_hex.size(); i += 2) {
            const std::uint8_t byte = parse_hex_byte(record.data_hex.substr(i, 2), "T.data.byte");
            memory_.at(actual_addr + static_cast<std::uint32_t>(i / 2)) = byte;
        }
    }
}

std::int64_t LinkingLoader::read_signed_be(const std::vector<std::uint8_t>& memory,
                                           std::uint32_t addr,
                                           std::uint32_t length) {
    if (length == 0 || length > 8) {
        throw std::runtime_error("Relocation length supports only 1..8 bytes");
    }

    std::uint64_t raw = 0;
    for (std::uint32_t i = 0; i < length; ++i) {
        raw = (raw << 8U) | memory.at(addr + i);
    }

    const std::uint32_t bit_width = length * 8;
    const std::uint64_t sign_bit = 1ULL << (bit_width - 1);
    if ((raw & sign_bit) != 0U) {
        if (bit_width == 64) {
            return static_cast<std::int64_t>(raw);
        }
        const std::uint64_t mask = (~0ULL) << bit_width;
        raw |= mask;
    }

    return static_cast<std::int64_t>(raw);
}

void LinkingLoader::write_signed_be(std::vector<std::uint8_t>& memory,
                                    std::uint32_t addr,
                                    std::uint32_t length,
                                    std::int64_t value) {
    if (length == 0 || length > 8) {
        throw std::runtime_error("Relocation length supports only 1..8 bytes");
    }

    std::uint64_t raw = static_cast<std::uint64_t>(value);
    for (std::uint32_t i = 0; i < length; ++i) {
        const std::uint32_t shift = (length - 1U - i) * 8U;
        memory.at(addr + i) = static_cast<std::uint8_t>((raw >> shift) & 0xFFU);
    }
}

void LinkingLoader::apply_relocations(std::uint32_t base_addr) {
    (void)base_addr;
    for (const auto& rec : reloc_info_) {
        const auto it = symtab_.find(rec.symbol);
        if (it == symtab_.end()) {
            throw std::runtime_error("Symbol not found: " + rec.symbol);
        }

        std::int64_t value = read_signed_be(memory_, rec.addr, rec.length);

        if (rec.op == '+') {
            value += static_cast<std::int64_t>(it->second);
        } else if (rec.op == '-') {
            value -= static_cast<std::int64_t>(it->second);
        } else {
            throw std::runtime_error(std::string("Invalid relocation operator: ") + rec.op);
        }

        write_signed_be(memory_, rec.addr, rec.length, value);
    }
}

std::uint32_t LinkingLoader::linking_load(const std::vector<std::string>& object_files,
                                          std::uint32_t load_address) {
    reset();
    std::uint32_t current_addr = load_address;

    for (const auto& file : object_files) {
        const auto obj = read_object_file(file);

        symtab_[obj.name] = current_addr;
        symbol_order_.push_back(obj.name);

        for (const auto& mod : obj.modifications) {
            ModificationRecord adjusted = mod;
            adjusted.addr = current_addr + mod.addr - obj.start;
            reloc_info_.push_back(adjusted);
        }

        load_program(obj.program, current_addr, obj.start);

        if (obj.has_entry && entry_point_ == 0) {
            entry_point_ = current_addr + obj.entry_point - obj.start;
        }

        current_addr += obj.length;
    }

    apply_relocations(load_address);

    if (entry_point_ == 0) {
        entry_point_ = load_address;
    }

    return entry_point_;
}

void LinkingLoader::display_memory(std::ostream& out, std::uint32_t start_addr, std::uint32_t length) const {
    for (std::uint32_t i = start_addr; i < start_addr + length; i += 16) {
        out << std::uppercase << std::hex << std::setfill('0') << std::setw(4) << i << " | ";
        for (std::uint32_t j = 0; j < 16; ++j) {
            if (i + j < start_addr + length) {
                out << std::uppercase << std::hex << std::setfill('0') << std::setw(2)
                    << static_cast<unsigned>(memory_.at(i + j)) << " ";
            } else {
                out << "   ";
            }
        }
        out << '\n';
    }
}

void LinkingLoader::display_symbol_table(std::ostream& out) const {
    out << "Symbol Index\n";
    out << "------------\n";

    for (const auto& symbol : symbol_order_) {
        const auto it = symtab_.find(symbol);
        if (it != symtab_.end()) {
            out << symbol << " -> 0x" << std::uppercase << std::hex << std::setfill('0')
                << std::setw(4) << it->second << '\n';
        }
    }
    out << '\n';
}
