#include "Environment.hpp"

#include <spdlog/details/os.h>
#include <spdlog/spdlog.h>

#include <mutex>
#include <unordered_map>

// project
#include <utility/Logging.hpp>

namespace dai {
namespace utility {

std::string getEnv(const std::string& var) {
    return getEnv(var, Logging::getInstance().logger);
}

std::string getEnv(const std::string& var, spdlog::logger& logger) {
    // Initialize only when getEnv called, instead of globally
    static std::mutex mtx;
    static std::unordered_map<std::string, std::string> map;
    std::unique_lock<std::mutex> lock(mtx);

    if(map.count(var) > 0) {
        return map.at(var);
    }
    auto value = spdlog::details::os::getenv(var.c_str());
    map[var] = value;

    // Log if env variable is set
    if(!value.empty()) {
        logger.debug("Environment '{}' set to '{}'", var, value);
    }

    return value;
}

std::vector<std::string> splitList(const std::string& list, const std::string& delimiter) {
    std::vector<std::string> result;
    if(list.empty()) {
        return result;  // Return an empty vector if the input string is empty
    }
    size_t pos = 0;
    size_t end = 0;
    while((end = list.find(delimiter, pos)) != std::string::npos) {
        result.push_back(list.substr(pos, end - pos));
        pos = end + delimiter.size();
    }
    result.push_back(list.substr(pos));
    return result;
}

}  // namespace utility
}  // namespace dai
