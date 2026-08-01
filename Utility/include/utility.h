#include <string>

extern std::string log_level_global;

static std::string trim(const std::string& s);
std::string getAttributeValue(const std::string& filename,
                              const std::string& attribute);
