#ifndef LINKING_LOADER_PASSERS_HPP
#define LINKING_LOADER_PASSERS_HPP

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct TextRecord {
    std::uint32_t addr = 0;
    std::uint32_t length_bytes = 0;
    std::string data;
};

struct ModificationRecord {
    std::uint32_t addr = 0;
    std::uint32_t length_nibbles = 0;
    char op = '+';
    std::string symbol;
};

struct ControlSection {
    std::string name;
    std::uint32_t start = 0;
    std::uint32_t length = 0;
    std::vector<TextRecord> text_records;
    std::vector<ModificationRecord> mod_records;
};

struct Pass1Result {
    std::uint32_t progaddr = 0;
    std::uint32_t memory_space_bytes = 0;
    std::unordered_map<std::string, std::uint32_t> estab;
    std::vector<std::string> estab_order;
};

class IPasser1 {
public:
    virtual ~IPasser1() = default;
    virtual Pass1Result build_estab(std::uint32_t progaddr,
                                    const std::vector<std::string>& object_files) const = 0;
};

class IPasser2 {
public:
    virtual ~IPasser2() = default;
    virtual void load_and_relocate(std::uint32_t progaddr,
                                   const std::vector<std::string>& object_files,
                                   const std::unordered_map<std::string, std::uint32_t>& estab,
                                   std::vector<char>& memory_hex) const = 0;
};

class Passer1 final : public IPasser1 {
public:
    Pass1Result build_estab(std::uint32_t progaddr,
                            const std::vector<std::string>& object_files) const override;
};

class Passer2 final : public IPasser2 {
public:
    void load_and_relocate(std::uint32_t progaddr,
                           const std::vector<std::string>& object_files,
                           const std::unordered_map<std::string, std::uint32_t>& estab,
                           std::vector<char>& memory_hex) const override;
};

#endif  // LINKING_LOADER_PASSERS_HPP
