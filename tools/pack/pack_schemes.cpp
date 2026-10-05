// brothemes_pack: builds tests/data/iterm2-color-schemes.bpk from a checkout of
// https://github.com/mbadolato/iTerm2-Color-Schemes, and lists or extracts one.
//
//   brothemes_pack <checkout-dir> <out.bpk>     pack the formats the oracle reads
//   brothemes_pack --list <file.bpk>
//   brothemes_pack --extract <file.bpk> <dir>   recreate the files
//
// Packed: LICENSE and CREDITS.md (the collection's licence and attribution),
// schemes/*.itermcolors, windowsterminal/*.json, kitty/*.conf and ghostty/*,
// byte for byte. write_archive() decodes the result and compares it with the
// input before anything is written.
#include "bpk.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

std::string read_file(const fs::path& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

std::string utf8(const fs::path& p) {
    const auto u = p.generic_u8string();
    return std::string(u.begin(), u.end());
}

fs::path from_utf8(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

int pack(const fs::path& root, const fs::path& out) {
    struct Source {
        const char* dir;
        const char* ext;  // nullptr: every regular file
    };
    const Source sources[] = {
        {"schemes", ".itermcolors"},
        {"windowsterminal", ".json"},
        {"kitty", ".conf"},
        {"ghostty", nullptr},
    };
    std::vector<bpk::Entry> entries;
    for (const char* top : {"LICENSE", "CREDITS.md"}) {
        if (fs::is_regular_file(root / top)) entries.push_back({top, read_file(root / top)});
    }
    if (entries.empty() || entries[0].path != "LICENSE") {
        std::fprintf(stderr, "no LICENSE in %s: not an iTerm2-Color-Schemes checkout?\n", utf8(root).c_str());
        return 1;
    }
    for (const Source& s : sources) {
        if (!fs::is_directory(root / s.dir)) {
            std::fprintf(stderr, "missing %s/ in %s\n", s.dir, utf8(root).c_str());
            return 1;
        }
        std::vector<bpk::Entry> files;
        for (const auto& e : fs::directory_iterator(root / s.dir)) {
            if (!e.is_regular_file()) continue;
            if (s.ext && e.path().extension() != s.ext) continue;
            files.push_back({utf8(fs::relative(e.path(), root)), read_file(e.path())});
        }
        std::sort(files.begin(), files.end(), [](const bpk::Entry& a, const bpk::Entry& b) { return a.path < b.path; });
        for (auto& f : files) entries.push_back(std::move(f));
    }
    size_t raw = 0;
    for (const auto& e : entries) raw += e.data.size();
    const std::string archive = bpk::write_archive(entries);  // verified round trip
    std::ofstream f(out, std::ios::binary);
    f.write(archive.data(), std::streamsize(archive.size()));
    if (!f) {
        std::fprintf(stderr, "cannot write %s\n", utf8(out).c_str());
        return 1;
    }
    std::printf("%zu files, %zu bytes -> %zu bytes (%.1f%%)\n", entries.size(), raw, archive.size(),
                100.0 * double(archive.size()) / double(raw));
    return 0;
}

int list(const fs::path& file, const fs::path* extract_to) {
    std::string error;
    const auto entries = bpk::read_archive_file(utf8(file), &error);
    if (!entries) {
        std::fprintf(stderr, "%s: %s\n", utf8(file).c_str(), error.c_str());
        return 1;
    }
    for (const auto& e : *entries) {
        if (!extract_to) {
            std::printf("%10zu  %s\n", e.data.size(), e.path.c_str());
            continue;
        }
        const fs::path rel = from_utf8(e.path);
        if (rel.is_absolute() || e.path.find("..") != std::string::npos) {
            std::fprintf(stderr, "refusing unsafe path %s\n", e.path.c_str());
            return 1;
        }
        const fs::path dest = *extract_to / rel;
        fs::create_directories(dest.parent_path());
        std::ofstream f(dest, std::ios::binary);
        f.write(e.data.data(), std::streamsize(e.data.size()));
        if (!f) {
            std::fprintf(stderr, "cannot write %s\n", utf8(dest).c_str());
            return 1;
        }
    }
    std::printf("%zu files\n", entries->size());
    return 0;
}

} // namespace

int main(int argc, char** argv) {
    try {
        const std::vector<std::string> args(argv + 1, argv + argc);
        // Command-line paths are in the platform's narrow encoding: fs::path(char*) converts them.
        if (args.size() == 2 && args[0] == "--list") return list(fs::path(args[1]), nullptr);
        if (args.size() == 3 && args[0] == "--extract") {
            const fs::path dir(args[2]);
            return list(fs::path(args[1]), &dir);
        }
        if (args.size() == 2 && args[0].rfind("--", 0) != 0) return pack(fs::path(args[0]), fs::path(args[1]));
    } catch (const std::exception& e) {
        std::fprintf(stderr, "error: %s\n", e.what());
        return 1;
    }
    std::fprintf(stderr,
                 "usage: brothemes_pack <iTerm2-Color-Schemes checkout> <out.bpk>\n"
                 "       brothemes_pack --list <file.bpk>\n"
                 "       brothemes_pack --extract <file.bpk> <dir>\n");
    return 2;
}
