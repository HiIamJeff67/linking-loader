#include "passers.hpp"

#include "util.hpp"

#include <cctype>
#include <cstddef>
#include <sstream>
#include <stdexcept>

namespace {

ControlSection parse_control_section(const std::string& filename) {
    const auto lines = util::read_text_lines(filename);
    if (lines.empty()) {
        throw std::runtime_error("Object file is empty: " + filename);
    }

    const std::string& header = lines[0];
    if (header.empty() || header[0] != 'H') {
        throw std::runtime_error("Missing H record in: " + filename);
    }

    ControlSection cs;
    cs.name = util::first_token(util::slice_or_empty(header, 1, 6));
    cs.start = util::parse_hex_u32(util::slice_or_empty(header, 7, 6), "H.start");
    cs.length = util::parse_hex_u32(util::slice_or_empty(header, 13, 6), "H.length");

    for (std::size_t i = 1; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        if (line.empty()) continue;

        if (line[0] == 'T') {
            TextRecord tr;
            tr.addr = util::parse_hex_u32(util::slice_or_empty(line, 1, 6), "T.addr");
            tr.length_bytes = util::parse_hex_u32(util::slice_or_empty(line, 7, 2), "T.length");
            tr.data = util::slice_or_empty(line, 9, tr.length_bytes * 2U);
            cs.text_records.push_back(tr);
            continue;
        }

        if (line[0] == 'M') {
            ModificationRecord mr;
            mr.addr = util::parse_hex_u32(util::slice_or_empty(line, 1, 6), "M.addr");
            mr.length_nibbles = util::parse_hex_u32(util::slice_or_empty(line, 7, 2), "M.length");
            mr.op = line.size() > 9 ? line[9] : '\0';
            mr.symbol = util::trim_ascii_whitespace(util::slice_or_empty(line, 10, line.size()));
            cs.mod_records.push_back(mr);
            continue;
        }
    }

    return cs;
}

void set_nibble(std::vector<char>& memory_hex, std::size_t index, char value) {
    if (index >= memory_hex.size()) {
        std::ostringstream oss;
        oss << "Memory write out of range at nibble index " << index;
        throw std::out_of_range(oss.str());
    }
    memory_hex[index] = value;
}

char get_nibble(const std::vector<char>& memory_hex, std::size_t index) {
    if (index >= memory_hex.size()) {
        std::ostringstream oss;
        oss << "Memory read out of range at nibble index " << index;
        throw std::out_of_range(oss.str());
    }
    return memory_hex[index];
}

void apply_text_record(const TextRecord& record,
                      std::uint32_t csaddr,
                      std::uint32_t progaddr,
                      std::vector<char>& memory_hex) {
    const std::uint32_t addr = record.addr + csaddr;
    std::size_t memory_index = static_cast<std::size_t>(addr - progaddr) * 2U;

    const std::size_t expected_nibbles = static_cast<std::size_t>(record.length_bytes) * 2U;
    for (std::size_t i = 0; i < expected_nibbles && i < record.data.size(); ++i) {
        set_nibble(memory_hex, memory_index++, static_cast<char>(std::toupper(static_cast<unsigned char>(record.data[i]))));
    }
}

void apply_mod_record(const ModificationRecord& record,
                      std::uint32_t csaddr,
                      std::uint32_t progaddr,
                      const std::unordered_map<std::string, std::uint32_t>& estab,
                      std::vector<char>& memory_hex) {
    const std::uint32_t addr = record.addr + csaddr;
    std::size_t memory_index = static_cast<std::size_t>(addr - progaddr) * 2U;

    if (record.length_nibbles == 5U) {
        memory_index += 1U;
    }

    std::string current;
    current.reserve(record.length_nibbles);
    for (std::size_t i = 0; i < record.length_nibbles; ++i) {
        current.push_back(get_nibble(memory_hex, memory_index + i));
    }

    const std::uint32_t bits = record.length_nibbles * 4U;
    std::int64_t value = util::decode_twos_complement_hex(current, bits);

    auto it = estab.find(record.symbol);
    if (it == estab.end()) {
        throw std::runtime_error("Symbol not found in ESTAB: " + record.symbol);
    }

    if (record.op == '+') {
        value += static_cast<std::int64_t>(it->second);
    } else if (record.op == '-') {
        value -= static_cast<std::int64_t>(it->second);
    } else {
        throw std::runtime_error(std::string("Invalid relocation operator: ") + record.op);
    }

    const std::string result = util::encode_twos_complement_hex(value, record.length_nibbles);
    for (std::size_t i = 0; i < result.size(); ++i) {
        set_nibble(memory_hex, memory_index + i, result[i]);
    }
}

}  // namespace

Pass1Result Passer1::build_estab(std::uint32_t progaddr,
                             const std::vector<std::string>& object_files) const {
    Pass1Result result;
    result.progaddr = progaddr;

    std::uint32_t csaddr = progaddr;

    for (const auto& file : object_files) {
        const auto lines = util::read_text_lines(file);
        if (lines.empty()) {
            throw std::runtime_error("Object file is empty: " + file);
        }

        const std::string& header = lines[0];
        const std::string csname = util::first_token(util::slice_or_empty(header, 1, 6));
        const std::uint32_t cslength = util::parse_hex_u32(util::slice_or_empty(header, 13, 6), "H.length");

        result.memory_space_bytes += cslength;

        const auto insert_cs = result.estab.emplace(csname, csaddr);
        if (!insert_cs.second) {
            throw std::runtime_error("Duplicate symbol in ESTAB: " + csname);
        }
        result.estab_order.push_back(csname);

        for (std::size_t l = 1; l < lines.size(); ++l) {
            const std::string& line = lines[l];
            if (line.empty() || line[0] != 'D') {
                continue;
            }

            if (line.size() <= 1) {
                continue;
            }

            const std::size_t n = (line.size() - 1) / 12;
            for (std::size_t j = 0; j < n; ++j) {
                const std::string name = util::first_token(util::slice_or_empty(line, 1 + (12 * j), 6));
                if (name.empty()) {
                    continue;
                }

                const std::uint32_t addr = util::parse_hex_u32(util::slice_or_empty(line, 7 + (12 * j), 6), "D.addr");
                const std::uint32_t symbol_addr = csaddr + addr;

                const auto insert_symbol = result.estab.emplace(name, symbol_addr);
                if (!insert_symbol.second) {
                    throw std::runtime_error("Duplicate symbol in ESTAB: " + name);
                }
                result.estab_order.push_back(name);
            }
        }

        csaddr += cslength;
    }

    return result;
}

void Passer2::load_and_relocate(std::uint32_t progaddr,
                      const std::vector<std::string>& object_files,
                      const std::unordered_map<std::string, std::uint32_t>& estab,
                      std::vector<char>& memory_hex) const {
    std::uint32_t csaddr = progaddr;

    for (const auto& file : object_files) {
        const ControlSection cs = parse_control_section(file);

        for (const auto& tr : cs.text_records) {
            apply_text_record(tr, csaddr, progaddr, memory_hex);
        }
        for (const auto& mr : cs.mod_records) {
            apply_mod_record(mr, csaddr, progaddr, estab, memory_hex);
        }

        csaddr += cs.length;
    }
}
