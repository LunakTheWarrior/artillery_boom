#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <regex>
#include <string>
#include <vector>

namespace {

std::string readFile(const std::filesystem::path& path) {
    std::ifstream input(path);
    EXPECT_TRUE(input.good()) << "Could not open " << path;
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

std::vector<unsigned> table(const std::string& source, const std::string& name) {
    const std::regex declaration("\\b" + name + "\\s*\\{([^}]*)\\}");
    std::smatch match;
    EXPECT_TRUE(std::regex_search(source, match, declaration)) << "Missing table " << name;
    if (match.empty()) return {};

    const std::regex number("0x[0-9a-fA-F]+|\\b[0-9]+\\b");
    std::vector<unsigned> values;
    for (std::sregex_iterator it(match[1].first, match[1].second, number), end; it != end; ++it)
        values.push_back(static_cast<unsigned>(std::stoul(it->str(), nullptr, 0)));
    return values;
}

std::vector<std::string> registers(const std::string& source) {
    const std::regex declaration(
        "Register<(?:std::)?(uint[0-9]+_t)>\\s+([A-Za-z_][A-Za-z_0-9]*)\\{(0x[0-9A-Fa-f]+)\\}");
    std::vector<std::string> result;
    for (std::sregex_iterator it(source.begin(), source.end(), declaration), end; it != end; ++it)
        result.push_back((*it)[1].str() + ":" + (*it)[2].str() + ":" + (*it)[3].str());
    return result;
}

std::filesystem::path workspaceRoot() {
    const char* sourceDirectory = std::getenv("TEST_SRCDIR");
    const char* workspace = std::getenv("TEST_WORKSPACE");
    EXPECT_NE(sourceDirectory, nullptr);
    EXPECT_NE(workspace, nullptr);
    return std::filesystem::path(sourceDirectory ? sourceDirectory : ".") /
           (workspace ? workspace : ".");
}

TEST(DriverParityTest, AvrAndPicoTablesStayInSync) {
    const auto root = workspaceRoot();
    const auto picoSource = readFile(root / "src/ModulinoDistance.cpp");
    const auto avrSource = readFile(root / "book_openener/firmware/book_openener/ModulinoDistance.cpp");
    for (const auto* name : {"defaultConfiguration", "rangeStatuses"})
        EXPECT_EQ(table(picoSource, name), table(avrSource, name)) << name << " diverged";

    const auto picoRegisters = readFile(root / "src/VL53L4CDRegisters.h");
    const auto avrRegisters = readFile(root / "book_openener/firmware/book_openener/VL53L4CDRegisters.h");
    EXPECT_EQ(registers(picoRegisters), registers(avrRegisters));
}

} // namespace
