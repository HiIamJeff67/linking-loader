#include "linking_loader.hpp"

#include "util.hpp"

#include <cstddef>
#include <iomanip>
#include <stdexcept>

LinkingLoader::LinkingLoader(std::shared_ptr<const IPasser1> passer1,
                             std::shared_ptr<const IPasser2> passer2)
    : passer1_(std::move(passer1)), passer2_(std::move(passer2)) {
    if (!passer1_ || !passer2_) {
        throw std::runtime_error("LinkingLoader requires both Passer1 and Passer2 dependencies");
    }
}

std::uint32_t LinkingLoader::linking_load(std::uint32_t progaddr, const std::vector<std::string>& object_files) {
    if (object_files.empty()) {
        throw std::runtime_error("At least one object file is required");
    }

    const Pass1Result pass1 = passer1_->build_estab(progaddr, object_files);

    progaddr_ = pass1.progaddr;
    memory_space_bytes_ = pass1.memory_space_bytes;
    estab_ = pass1.estab;
    estab_order_ = pass1.estab_order;

    memory_hex_.assign(static_cast<std::size_t>(memory_space_bytes_) * 2U, '.');

    passer2_->load_and_relocate(progaddr_, object_files, estab_, memory_hex_);

    return progaddr_;
}

std::uint32_t LinkingLoader::progaddr() const {
    return progaddr_;
}

std::uint32_t LinkingLoader::memory_space_bytes() const {
    return memory_space_bytes_;
}

void LinkingLoader::write_estab(std::ostream& out) const {
    for (const auto& name : estab_order_) {
        const auto it = estab_.find(name);
        if (it == estab_.end()) continue;
        out << name << ":0x" << std::hex << std::nouppercase << it->second << std::dec << '\n';
    }
}

void LinkingLoader::write_memory_dump(std::ostream& out,
                                      std::uint32_t start_addr,
                                      std::uint32_t length_bytes) const {
    if (start_addr < progaddr_) {
        throw std::runtime_error("dump start is below PROGADDR");
    }

    const std::size_t start_nibble = static_cast<std::size_t>(start_addr - progaddr_) * 2U;
    const std::size_t total_nibbles = static_cast<std::size_t>(length_bytes) * 2U;

    for (std::size_t i = 0; i < total_nibbles; ++i) {
        if ((i % 32U) == 0U) {
            const std::uint32_t addr = start_addr + static_cast<std::uint32_t>(i / 2U);
            out << '\n' << util::encode_twos_complement_hex(addr, 4) << ' ';
        }

        const std::size_t idx = start_nibble + i;
        if (idx >= memory_hex_.size()) {
            throw std::out_of_range("memory dump index out of range");
        }
        out << memory_hex_[idx] << ' ';
    }
    out << '\n';
}
