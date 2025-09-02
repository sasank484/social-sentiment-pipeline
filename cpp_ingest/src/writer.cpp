#include "writer.hpp"
#include <fstream>
#include <stdexcept>

void append_ndjson(const nlohmann::json& obj, const std::string& path) {
    std::ofstream ofs(path, std::ios::app);
    if (!ofs.is_open()) {
        throw std::runtime_error("Cannot open NDJSON path: " + path);
    }
    ofs << obj.dump() << "\n";
    ofs.flush();
}
