#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdlib>
#include <chrono>
#include <iomanip>
#include <sstream>
#include "json.hpp"

#ifdef _WIN32
#include <windows.h>
#include <shlobj.h>
#else
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>
#endif

using json = nlohmann::json;

// ─── ANSI Colors ─────────────────────────────────────────────────────────────
namespace Ansi {
    const std::string Reset = "\x1b[0m";
    const std::string Bold = "\x1b[1m";
    const std::string Dim = "\x1b[2m";
    const std::string Red = "\x1b[91m";
    const std::string Green = "\x1b[92m";
    const std::string Yellow = "\x1b[93m";
    const std::string Blue = "\x1b[94m";
    const std::string Magenta = "\x1b[95m";
    const std::string Cyan = "\x1b[96m";
    const std::string White = "\x1b[97m";
    const std::string Gray = "\x1b[90m";

    std::string Paint(const std::string& color, const std::string& text) {
        return color + text + Reset;
    }
    std::string B(const std::string& text) {
        return Bold + text + Reset;
    }
    std::string Truncate(const std::string& s, size_t maxLen) {
        if (s.empty()) return "";
        // Simple truncation. For multi-byte characters, this might cut them, 
        // but for a simple CLI we keep it basic.
        return s.length() > maxLen ? s.substr(0, maxLen - 1) + "…" : s;
    }
}

// ─── Storage ─────────────────────────────────────────────────────────────────

std::string GetDataDir() {
    std::string path;
#ifdef _WIN32
    char szPath[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROFILE, NULL, 0, szPath))) {
        path = szPath;
    } else {
        path = getenv("USERPROFILE") ? getenv("USERPROFILE") : ".";
    }
#else
    const char *homedir;
    if ((homedir = getenv("HOME")) == NULL) {
        homedir = getpwuid(getuid())->pw_dir;
    }
    path = homedir;
#endif
    return path + "/.snip";
}

std::string GetDataFile() {
    return GetDataDir() + "/snippets.json";
}

void EnsureDirectoryExists() {
#ifdef _WIN32
    CreateDirectoryA(GetDataDir().c_str(), NULL);
#else
    std::string dir = GetDataDir();
    std::string cmd = "mkdir -p " + dir;
    system(cmd.c_str());
#endif
}

json LoadData() {
    EnsureDirectoryExists();
    std::ifstream f(GetDataFile());
    if (!f.is_open()) {
        json empty;
        empty["nextId"] = 1;
        empty["snippets"] = json::array();
        return empty;
    }
    json j;
    try {
        f >> j;
    } catch (...) {
        j["nextId"] = 1;
        j["snippets"] = json::array();
    }
    return j;
}

void SaveData(const json& data) {
    EnsureDirectoryExists();
    std::ofstream f(GetDataFile());
    if (f.is_open()) {
        f << data.dump(2);
    }
}

// ─── Display Helpers ─────────────────────────────────────────────────────────

std::string PadRight(std::string s, size_t len) {
    if (s.length() < len) {
        s.append(len - s.length(), ' ');
    }
    return s;
}

std::string PadLeft(std::string s, size_t len) {
    if (s.length() < len) {
        s.insert(0, len - s.length(), ' ');
    }
    return s;
}

void PrintTable(const json& snippets) {
    if (snippets.empty()) {
        std::cout << Ansi::Paint(Ansi::Gray, "  No snippets found.") << std::endl;
        return;
    }

    const int IdW = 4;
    const int TagW = 18;
    const int CmdW = 50;
    const int DescW = 32;

    std::string sep = Ansi::Paint(Ansi::Gray,
        "  " +
        std::string(IdW + 2, '-') + "+" +
        std::string(TagW + 2, '-') + "+" +
        std::string(CmdW + 2, '-') + "+" +
        std::string(DescW + 2, '-'));

    std::string header =
        "  " +
        Ansi::Paint(Ansi::Bold + Ansi::White, PadRight(" ID", IdW + 1)) + Ansi::Paint(Ansi::Gray, "│") +
        Ansi::Paint(Ansi::Bold + Ansi::White, PadRight(" Tags", TagW + 1)) + Ansi::Paint(Ansi::Gray, "│") +
        Ansi::Paint(Ansi::Bold + Ansi::White, PadRight(" Command", CmdW + 1)) + Ansi::Paint(Ansi::Gray, "│") +
        Ansi::Paint(Ansi::Bold + Ansi::White, PadRight(" Description", DescW + 1));

    std::cout << "\n" << header << "\n" << sep << "\n";

    for (const auto& s : snippets) {
        std::string tagsStr = "";
        if (s.contains("tags") && s["tags"].is_array()) {
            for (const auto& t : s["tags"]) {
                tagsStr += "[" + t.get<std::string>() + "] ";
            }
        }
        
        std::string idStr = Ansi::Paint(Ansi::Yellow, PadLeft(std::to_string(s["id"].get<int>()), IdW));
        std::string tagCol = Ansi::Paint(Ansi::Cyan, PadRight(Ansi::Truncate(tagsStr, TagW), TagW));
        std::string cmdCol = Ansi::Paint(Ansi::Green, PadRight(Ansi::Truncate(s.value("command", ""), CmdW), CmdW));
        std::string descCol = Ansi::Paint(Ansi::Gray, PadRight(Ansi::Truncate(s.value("description", ""), DescW), DescW));

        std::cout << "  " << idStr << " " << Ansi::Paint(Ansi::Gray, "│") << " " 
                  << tagCol << " " << Ansi::Paint(Ansi::Gray, "│") << " " 
                  << cmdCol << " " << Ansi::Paint(Ansi::Gray, "│") << " " 
                  << descCol << "\n";
    }
    std::cout << std::endl;
}

std::string GetCurrentTimeStr() {
    auto t = std::time(nullptr);
    auto tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string ToLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::tolower(c); });
    return s;
}

// ─── Arg Parser ───────────────────────────────────────────────────────────────

struct Args {
    std::vector<std::string> positional;
    std::map<std::string, std::string> flags;
};

Args ParseArgs(int argc, char* argv[]) {
    Args parsed;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg.rfind("--", 0) == 0) {
            std::string key = arg.substr(2);
            if (i + 1 < argc && std::string(argv[i+1]).rfind("--", 0) != 0) {
                parsed.flags[key] = argv[i+1];
                i++;
            } else {
                parsed.flags[key] = "true";
            }
        } else {
            parsed.positional.push_back(arg);
        }
    }
    return parsed;
}

// ─── Commands ─────────────────────────────────────────────────────────────────

void PrintHelp() {
    std::cout << "\n" << Ansi::B(Ansi::Paint(Ansi::Magenta, "  ◆ snip") + Ansi::Paint(Ansi::White, " — CLI Command Snippet Manager")) << "\n\n";
    std::cout << Ansi::B(Ansi::Paint(Ansi::White, "  Usage:")) << "\n";

    std::vector<std::pair<std::string, std::string>> cmds = {
        {"snip add <cmd> [--tag <t>] [--desc <d>]", "Save a new snippet"},
        {"snip list [--tag <filter>]", "List all snippets"},
        {"snip search <query>", "Search by command, tag, or description"},
        {"snip show <id>", "Show full snippet details"},
        {"snip run <id>", "Execute a snippet"},
        {"snip copy <id>", "Copy snippet to clipboard"},
        {"snip edit <id> [--command] [--tag] [--desc]", "Edit a snippet"},
        {"snip delete <id>", "Delete a snippet"},
        {"snip tags", "List all tags"},
        {"snip help", "Show this help"}
    };

    for (const auto& cmd : cmds) {
        std::cout << "  " << Ansi::Paint(Ansi::Green, PadRight(cmd.first, 50)) << " " << Ansi::Paint(Ansi::Gray, cmd.second) << "\n";
    }

    std::cout << "\n" << Ansi::Paint(Ansi::Gray, "  Data stored at: " + GetDataFile()) << "\n\n";
}

void CmdAdd(const Args& args) {
    if (args.positional.empty()) {
        std::cout << Ansi::Paint(Ansi::Red, "  Error: ") << "Provide a command to save.\n";
        std::cout << Ansi::Paint(Ansi::Gray, "  Usage: snip add \"your command\" [--tag tagname] [--desc \"description\"]") << "\n";
        return;
    }

    auto data = LoadData();
    int id = data["nextId"].get<int>();
    data["nextId"] = id + 1;

    json snippet;
    snippet["id"] = id;
    snippet["command"] = args.positional[0];
    
    std::vector<std::string> tags;
    if (args.flags.count("tag")) {
        std::string tagStr = args.flags.at("tag");
        size_t pos = 0;
        while ((pos = tagStr.find(",")) != std::string::npos) {
            tags.push_back(tagStr.substr(0, pos));
            tagStr.erase(0, pos + 1);
        }
        if (!tagStr.empty()) tags.push_back(tagStr);
    }
    snippet["tags"] = tags;
    
    snippet["description"] = args.flags.count("desc") ? args.flags.at("desc") : 
                             (args.flags.count("description") ? args.flags.at("description") : "");
    snippet["createdAt"] = GetCurrentTimeStr();

    data["snippets"].push_back(snippet);
    SaveData(data);

    std::cout << "\n" << Ansi::Paint(Ansi::Green, "  ✓ Snippet saved!") << Ansi::Paint(Ansi::Gray, " (id: " + std::to_string(id) + ")\n");
    std::cout << "  " << Ansi::Paint(Ansi::Gray, "CMD:") << " " << Ansi::Paint(Ansi::Green, snippet["command"].get<std::string>()) << "\n";
    if (!tags.empty()) {
        std::cout << "  " << Ansi::Paint(Ansi::Gray, "Tags:") << " ";
        for (const auto& t : tags) std::cout << Ansi::Paint(Ansi::Cyan, "[" + t + "] ");
        std::cout << "\n";
    }
    if (!snippet["description"].get<std::string>().empty()) {
        std::cout << "  " << Ansi::Paint(Ansi::Gray, "Desc:") << " " << snippet["description"].get<std::string>() << "\n";
    }
    std::cout << "\n";
}

void CmdList(const Args& args) {
    auto data = LoadData();
    json snippets = data["snippets"];

    if (args.flags.count("tag")) {
        std::string filterTag = ToLower(args.flags.at("tag"));
        json filtered = json::array();
        for (const auto& s : snippets) {
            bool matches = false;
            if (s.contains("tags") && s["tags"].is_array()) {
                for (const auto& t : s["tags"]) {
                    if (ToLower(t.get<std::string>()).find(filterTag) != std::string::npos) {
                        matches = true;
                        break;
                    }
                }
            }
            if (matches) filtered.push_back(s);
        }
        snippets = filtered;
    }

    std::cout << "\n" << Ansi::B("  " + Ansi::Paint(Ansi::Magenta, "◆") + " snip — " + std::to_string(snippets.size()) + " snippet(s)") << "\n";

    if (snippets.empty()) {
        std::cout << Ansi::Paint(Ansi::Yellow, "\n  No snippets yet!") << "\n";
        std::cout << Ansi::Paint(Ansi::Gray, "  Run: snip add \"your command\" --tag mytag --desc \"what it does\"\n\n");
        return;
    }

    PrintTable(snippets);
}

void CmdSearch(const Args& args) {
    if (args.positional.empty()) {
        std::cout << Ansi::Paint(Ansi::Red, "  Error: ") << "Provide a search query.\n";
        std::cout << Ansi::Paint(Ansi::Gray, "  Usage: snip search <query>") << "\n";
        return;
    }

    std::string query = ToLower(args.positional[0]);
    auto data = LoadData();
    json results = json::array();

    for (const auto& s : data["snippets"]) {
        std::string cmd = ToLower(s.value("command", ""));
        std::string desc = ToLower(s.value("description", ""));
        bool matches = cmd.find(query) != std::string::npos || desc.find(query) != std::string::npos;
        
        if (!matches && s.contains("tags") && s["tags"].is_array()) {
            for (const auto& t : s["tags"]) {
                if (ToLower(t.get<std::string>()).find(query) != std::string::npos) {
                    matches = true;
                    break;
                }
            }
        }
        
        if (matches) {
            results.push_back(s);
        }
    }

    std::cout << "\n" << Ansi::B("  " + Ansi::Paint(Ansi::Magenta, "◆") + " Results for \"" + Ansi::Paint(Ansi::Yellow, args.positional[0]) + "\":") << "\n";
    PrintTable(results);
}

void CmdShow(const Args& args) {
    if (args.positional.empty()) {
        std::cout << Ansi::Paint(Ansi::Red, "  Error: ") << "Provide a snippet ID.\n";
        return;
    }

    int id = 0;
    try { id = std::stoi(args.positional[0]); } catch(...) { std::cout << Ansi::Paint(Ansi::Red, "  Invalid ID.\n"); return; }

    auto data = LoadData();
    json* target = nullptr;
    for (auto& s : data["snippets"]) {
        if (s["id"] == id) { target = &s; break; }
    }

    if (!target) {
        std::cout << Ansi::Paint(Ansi::Red, "  Snippet #" + std::to_string(id) + " not found.\n");
        return;
    }

    std::cout << "\n  " << Ansi::Paint(Ansi::Yellow, Ansi::B("#" + std::to_string(id))) << "  ";
    if (target->contains("tags") && (*target)["tags"].is_array()) {
        for (const auto& t : (*target)["tags"]) {
            std::cout << Ansi::Paint(Ansi::Cyan, "[" + t.get<std::string>() + "] ");
        }
    }
    std::cout << "\n  " << Ansi::Paint(Ansi::Gray, "Description:") << " " << (target->value("description", "").empty() ? Ansi::Paint(Ansi::Gray, "(none)") : target->value("description", "")) << "\n";
    std::cout << "  " << Ansi::Paint(Ansi::Gray, "Created:") << " " << target->value("createdAt", "") << "\n\n";
    std::cout << "  " << Ansi::Paint(Ansi::Bold + Ansi::Green, "$ " + target->value("command", "")) << "\n\n";
}

void CmdRun(const Args& args) {
    if (args.positional.empty()) {
        std::cout << Ansi::Paint(Ansi::Red, "  Error: ") << "Provide a snippet ID.\n";
        return;
    }

    int id = 0;
    try { id = std::stoi(args.positional[0]); } catch(...) { std::cout << Ansi::Paint(Ansi::Red, "  Invalid ID.\n"); return; }

    auto data = LoadData();
    json* target = nullptr;
    for (auto& s : data["snippets"]) {
        if (s["id"] == id) { target = &s; break; }
    }

    if (!target) {
        std::cout << Ansi::Paint(Ansi::Red, "  Snippet #" + std::to_string(id) + " not found.\n");
        return;
    }

    std::string cmd = target->value("command", "");
    std::cout << "\n  " << Ansi::Paint(Ansi::Gray, "Running:") << " " << Ansi::Paint(Ansi::Green, cmd) << "\n";
    std::cout << Ansi::Paint(Ansi::Gray, "  " + std::string(60, '-')) << "\n\n";

    int ret = system(cmd.c_str());
    (void)ret;
    std::cout << "\n";
}

void CmdCopy(const Args& args) {
    if (args.positional.empty()) {
        std::cout << Ansi::Paint(Ansi::Red, "  Error: ") << "Provide a snippet ID.\n";
        return;
    }

    int id = 0;
    try { id = std::stoi(args.positional[0]); } catch(...) { std::cout << Ansi::Paint(Ansi::Red, "  Invalid ID.\n"); return; }

    auto data = LoadData();
    json* target = nullptr;
    for (auto& s : data["snippets"]) {
        if (s["id"] == id) { target = &s; break; }
    }

    if (!target) {
        std::cout << Ansi::Paint(Ansi::Red, "  Snippet #" + std::to_string(id) + " not found.\n");
        return;
    }

    std::string cmd = target->value("command", "");
    std::string execCmd;
    
#ifdef _WIN32
    // Basic escape for double quotes on windows cmd
    std::string escaped = cmd;
    size_t pos = 0;
    while((pos = escaped.find("\"", pos)) != std::string::npos) {
        escaped.replace(pos, 1, "\\\"");
        pos += 2;
    }
    execCmd = "cmd.exe /c echo " + escaped + " | clip";
#else
    execCmd = "echo '" + cmd + "' | xclip -selection clipboard 2>/dev/null || echo '" + cmd + "' | pbcopy 2>/dev/null";
#endif

    int ret = system(execCmd.c_str());
    if (ret == 0) {
        std::cout << "\n  " << Ansi::Paint(Ansi::Green, "✓ Copied to clipboard!") << "\n";
        std::cout << "  " << Ansi::Paint(Ansi::Gray, "$") << " " << Ansi::Paint(Ansi::Green, cmd) << "\n\n";
    } else {
        std::cout << "\n  " << Ansi::Paint(Ansi::Yellow, "Could not copy automatically. Here is your command:") << "\n";
        std::cout << "  " << Ansi::Paint(Ansi::Green, cmd) << "\n\n";
    }
}

void CmdDelete(const Args& args) {
    if (args.positional.empty()) {
        std::cout << Ansi::Paint(Ansi::Red, "  Error: ") << "Provide a snippet ID.\n";
        return;
    }

    int id = 0;
    try { id = std::stoi(args.positional[0]); } catch(...) { std::cout << Ansi::Paint(Ansi::Red, "  Invalid ID.\n"); return; }

    auto data = LoadData();
    bool found = false;
    for (auto it = data["snippets"].begin(); it != data["snippets"].end(); ++it) {
        if ((*it)["id"] == id) {
            std::string cmd = (*it).value("command", "");
            data["snippets"].erase(it);
            SaveData(data);
            found = true;
            std::cout << "\n  " << Ansi::Paint(Ansi::Green, "✓ Deleted snippet #" + std::to_string(id)) << Ansi::Paint(Ansi::Gray, ": " + cmd) << "\n\n";
            break;
        }
    }

    if (!found) {
        std::cout << Ansi::Paint(Ansi::Red, "  Snippet #" + std::to_string(id) + " not found.\n");
    }
}

void CmdEdit(const Args& args) {
    if (args.positional.empty()) {
        std::cout << Ansi::Paint(Ansi::Red, "  Error: ") << "Provide a snippet ID.\n";
        return;
    }

    int id = 0;
    try { id = std::stoi(args.positional[0]); } catch(...) { std::cout << Ansi::Paint(Ansi::Red, "  Invalid ID.\n"); return; }

    auto data = LoadData();
    json* target = nullptr;
    for (auto& s : data["snippets"]) {
        if (s["id"] == id) { target = &s; break; }
    }

    if (!target) {
        std::cout << Ansi::Paint(Ansi::Red, "  Snippet #" + std::to_string(id) + " not found.\n");
        return;
    }

    if (args.flags.count("command")) (*target)["command"] = args.flags.at("command");
    if (args.flags.count("desc")) (*target)["description"] = args.flags.at("desc");
    if (args.flags.count("description")) (*target)["description"] = args.flags.at("description");
    
    if (args.flags.count("tag")) {
        std::vector<std::string> tags;
        std::string tagStr = args.flags.at("tag");
        size_t pos = 0;
        while ((pos = tagStr.find(",")) != std::string::npos) {
            tags.push_back(tagStr.substr(0, pos));
            tagStr.erase(0, pos + 1);
        }
        if (!tagStr.empty()) tags.push_back(tagStr);
        (*target)["tags"] = tags;
    }

    SaveData(data);
    std::cout << "\n  " << Ansi::Paint(Ansi::Green, "✓ Snippet #" + std::to_string(id) + " updated!") << "\n";
    std::cout << "  " << Ansi::Paint(Ansi::Gray, "CMD:") << " " << Ansi::Paint(Ansi::Green, target->value("command", "")) << "\n\n";
}

void CmdTags(const Args& args) {
    auto data = LoadData();
    std::map<std::string, int> tagCounts;

    for (const auto& s : data["snippets"]) {
        if (s.contains("tags") && s["tags"].is_array()) {
            for (const auto& t : s["tags"]) {
                tagCounts[t.get<std::string>()]++;
            }
        }
    }

    std::cout << "\n";
    if (tagCounts.empty()) {
        std::cout << Ansi::Paint(Ansi::Gray, "  No tags yet.\n");
    } else {
        std::cout << Ansi::B("  " + Ansi::Paint(Ansi::Magenta, "◆") + " All tags:") << "\n";
        for (const auto& pair : tagCounts) {
            std::cout << "  " << Ansi::Paint(Ansi::Cyan, "[" + pair.first + "]") << " " << Ansi::Paint(Ansi::Gray, std::to_string(pair.second) + " snippet(s)") << "\n";
        }
    }
    std::cout << "\n";
}

// ─── Entry Point ──────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
#ifdef _WIN32
    // Enable ANSI colors on Windows
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
    SetConsoleMode(hOut, dwMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
#endif

    Args parsedArgs = ParseArgs(argc, argv);
    std::string command = parsedArgs.positional.empty() ? "" : ToLower(parsedArgs.positional[0]);
    
    // Remove the command name from positional args
    if (!parsedArgs.positional.empty()) {
        parsedArgs.positional.erase(parsedArgs.positional.begin());
    }

    if (command == "add") CmdAdd(parsedArgs);
    else if (command == "list" || command == "ls") CmdList(parsedArgs);
    else if (command == "search" || command == "find") CmdSearch(parsedArgs);
    else if (command == "show") CmdShow(parsedArgs);
    else if (command == "run" || command == "exec") CmdRun(parsedArgs);
    else if (command == "copy" || command == "cp") CmdCopy(parsedArgs);
    else if (command == "delete" || command == "rm") CmdDelete(parsedArgs);
    else if (command == "edit") CmdEdit(parsedArgs);
    else if (command == "tags") CmdTags(parsedArgs);
    else if (command == "help" || command == "--help" || command == "-h" || command == "") PrintHelp();
    else {
        std::cout << Ansi::Paint(Ansi::Red, "  Unknown command: " + command) << "\n";
        PrintHelp();
    }

    return 0;
}
