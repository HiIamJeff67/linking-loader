#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::string shell_quote(const std::string& s) {
    std::string out = "'";
    for (char c : s) {
        if (c == '\'') {
            out += "'\\''";
        } else {
            out.push_back(c);
        }
    }
    out += "'";
    return out;
}

void run_command_or_throw(const std::string& cmd) {
    const int code = std::system(cmd.c_str());
    if (code != 0) {
        throw std::runtime_error("Command failed: " + cmd);
    }
}

std::string rstrip(const std::string& s) {
    std::size_t end = s.size();
    while (end > 0 && std::isspace(static_cast<unsigned char>(s[end - 1])) != 0) {
        --end;
    }
    return s.substr(0, end);
}

std::vector<std::string> load_normalized_lines(const std::filesystem::path& path, bool is_cpp_report) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Cannot open file: " + path.string());
    }

    std::vector<std::string> out;
    std::string line;
    while (std::getline(in, line)) {
        std::string s = rstrip(line);

        if (is_cpp_report) {
            if (s == "=== ESTAB ===" || s == "=== MEMORY ===") {
                continue;
            }
        }

        if (s.empty()) {
            continue;
        }
        out.push_back(s);
    }

    return out;
}

void compare_lines_or_throw(const std::vector<std::string>& ref,
                            const std::vector<std::string>& got,
                            const std::string& ref_name,
                            const std::string& got_name) {
    if (ref.size() != got.size()) {
        std::ostringstream oss;
        oss << "Line count mismatch: " << ref_name << "=" << ref.size()
            << ", " << got_name << "=" << got.size();
        throw std::runtime_error(oss.str());
    }

    for (std::size_t i = 0; i < ref.size(); ++i) {
        if (ref[i] != got[i]) {
            std::ostringstream oss;
            oss << "Mismatch at line " << (i + 1) << "\n"
                << ref_name << ": " << ref[i] << "\n"
                << got_name << ": " << got[i];
            throw std::runtime_error(oss.str());
        }
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <cpp_loader_exe> <project_root>\n";
        return 2;
    }

    try {
        const std::filesystem::path cpp_loader = argv[1];
        const std::filesystem::path root = argv[2];

        const std::filesystem::path temp_dir = root / "test" / "_tmp";
        std::filesystem::create_directories(temp_dir);

        const std::filesystem::path ref_out = temp_dir / "ref_output.txt";
        const std::filesystem::path cpp_out = temp_dir / "cpp_output.txt";

        const std::filesystem::path py_script = root / "example" / "linkingloader.py";
        const std::filesystem::path ex_a = root / "example" / "PROGA.obj";
        const std::filesystem::path ex_b = root / "example" / "PROGB.obj";
        const std::filesystem::path ex_c = root / "example" / "PROGC.obj";

        const std::filesystem::path cpp_a = root / "PROGA.obj";
        const std::filesystem::path cpp_b = root / "PROGB.obj";
        const std::filesystem::path cpp_c = root / "PROGC.obj";

        const std::string py_cmd =
            "python3 " + shell_quote(py_script.string()) +
            " 4000 " + shell_quote(ex_a.string()) +
            " " + shell_quote(ex_b.string()) +
            " " + shell_quote(ex_c.string()) +
            " > " + shell_quote(ref_out.string());

        const std::string cpp_cmd =
            shell_quote(cpp_loader.string()) +
            " 4000 " + shell_quote(cpp_a.string()) +
            " " + shell_quote(cpp_b.string()) +
            " " + shell_quote(cpp_c.string()) +
            " --report " + shell_quote(cpp_out.string());

        run_command_or_throw(py_cmd);
        run_command_or_throw(cpp_cmd);

        const auto ref_lines = load_normalized_lines(ref_out, false);
        const auto cpp_lines = load_normalized_lines(cpp_out, true);

        compare_lines_or_throw(ref_lines, cpp_lines, "python", "cpp");

        std::cout << "PASS: Python and C++ loader outputs are equivalent.\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
