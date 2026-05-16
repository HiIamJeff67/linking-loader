#ifndef LINKING_LOADER_HPP
#define LINKING_LOADER_HPP

#include "passers.hpp"

#include <cstdint>
#include <iosfwd>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class LinkingLoader {
private:
    std::shared_ptr<const IPasser1> passer1_;
    std::shared_ptr<const IPasser2> passer2_;

    std::uint32_t progaddr_ = 0;
    std::uint32_t memory_space_bytes_ = 0;
    std::unordered_map<std::string, std::uint32_t> estab_;
    std::vector<std::string> estab_order_;
    std::vector<char> memory_hex_;

public:
    explicit LinkingLoader(std::shared_ptr<const IPasser1> passer1 = std::make_shared<Passer1>(),
                           std::shared_ptr<const IPasser2> passer2 = std::make_shared<Passer2>());

    std::uint32_t linking_load(std::uint32_t progaddr, const std::vector<std::string>& object_files);

    std::uint32_t progaddr() const;
    std::uint32_t memory_space_bytes() const;

    void write_estab(std::ostream& out) const;
    void write_memory_dump(std::ostream& out, std::uint32_t start_addr, std::uint32_t length_bytes) const;
};

#endif  // LINKING_LOADER_HPP
