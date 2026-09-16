#include <windows.h>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <array>
#include <iomanip>
#include <sstream>

namespace fs = std::filesystem;

static bool readAt(const fs::path& path, std::uint64_t offset, void* buf, std::size_t size) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!f) return false;
    f.read(reinterpret_cast<char*>(buf), static_cast<std::streamsize>(size));
    return static_cast<std::size_t>(f.gcount()) == size;
}

static bool writeAt(const fs::path& path, std::uint64_t offset, const void* buf, std::size_t size) {
    std::fstream f(path, std::ios::binary | std::ios::in | std::ios::out);
    if (!f) return false;
    f.seekp(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!f) return false;
    f.write(reinterpret_cast<const char*>(buf), static_cast<std::streamsize>(size));
    f.flush();
    return static_cast<bool>(f);
}

static void pauseBeforeExit() {
    std::cout << "\nPress Enter to close..." << std::flush;
    std::string dummy;
    std::getline(std::cin, dummy);
}

static int fail(int code, const std::string& msg) {
    std::cerr << "\n[ERROR] " << msg << "\n";
    pauseBeforeExit();
    return code;
}

int main(int argc, char** argv) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    std::cout << "Linux Patcher by Ku7l\n"
                 "-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-\n";

    fs::path exe;
    if (argc >= 2) {
        exe = fs::path(argv[1]);
    } else {
        std::cout << "Enter path to Minecraft.Windows.exe: " << std::flush;
        std::string input;
        std::getline(std::cin, input);
        while (!input.empty() && (input.back() == '\n' || input.back() == '\r')) input.pop_back();
        if (input.size() >= 2 && input.front() == '"' && input.back() == '"') {
            input = input.substr(1, input.size() - 2);
        }
        exe = fs::path(input);
    }

    if (exe.empty()) return fail(1, "No path was entered");
    std::error_code ec;
    exe = fs::absolute(exe, ec);
    if (ec || !fs::exists(exe)) return fail(2, "File not found: " + exe.string());
    if (!fs::is_regular_file(exe)) return fail(3, "The path is not a regular file.");

    constexpr std::uint64_t kOffset = 0x238FEEC;
    constexpr std::uint32_t kOld = 7;
    constexpr std::uint32_t kNew = 20;
    constexpr std::array<std::uint8_t, 4> kTail{0x0F, 0x44, 0xC2, 0xC3};

    std::uint8_t opcode = 0;
    std::uint32_t imm = 0;
    std::array<std::uint8_t, 4> tail{};

    std::cout << "\nCheck executable...\n";
    if (!readAt(exe, kOffset, &opcode, 1) ||
        !readAt(exe, kOffset + 1, &imm, sizeof(imm)) ||
        !readAt(exe, kOffset + 5, tail.data(), tail.size())) {
        return fail(4, "error");
    }

    if (opcode != 0xBA || tail != kTail) {
        std::ostringstream oss;
        oss << "Patch signature mismatch at file offset 0x" << std::hex << kOffset
            << ". Refusing to patch this executable.";
        return fail(5, oss.str());
    }

    if (imm == kNew) {
        std::cout << "DeviceOS is already patched to 20\n";
        pauseBeforeExit();
        return 0;
    }

    if (imm != kOld) {
        std::ostringstream oss;
        oss << "Expected DeviceOS constant 7, but found " << std::dec << imm
            << ". Refusing to patch this executable.";
        return fail(6, oss.str());
    }

    const fs::path backup = exe.string() + ".before-deviceos20.bak";
    if (!fs::exists(backup)) {
        std::error_code copyEc;
        fs::copy_file(exe, backup, fs::copy_options::none, copyEc);
        if (copyEc) {
            return fail(7, "Could not create backup: " + backup.string() +
                           "\nClose minecraft and make sure you have write permission");
        }
        std::cout << "Backup created: " << backup << "\n";
    } else {
        std::cout << "Backup already exist: " << backup << "\n";
    }

    const std::uint32_t replacement = kNew;
    std::cout << "Patching file offset 0x" << std::hex << kOffset << std::dec
              << " (DeviceOS " << kOld << " -> " << kNew << ")...\n";

    if (!writeAt(exe, kOffset + 1, &replacement, sizeof(replacement))) {
        return fail(8, "Could not write the executable, try close minecraft and try again as administrator if needed");
    }

    std::uint32_t verify = 0;
    if (!readAt(exe, kOffset + 1, &verify, sizeof(verify)) || verify != kNew) {
        return fail(9, "The patch write could not be verified.");
    }

    std::cout << "\n[+] Minecraft.Windows.exe patched successfully.\n"
    pauseBeforeExit();
    return 0;
}
