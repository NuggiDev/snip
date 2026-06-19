# snip ✂️

> A personal CLI command snippet manager — save, search, tag, and run your most-used terminal commands. No more Googling the same command for the 5th time.

Built in **C++**, compiled natively for maximum speed.

---

## Installation

### Build from source (CMake)

You will need **CMake** and a C++ compiler (like MSVC, GCC, or Clang).

```powershell
git clone https://github.com/NuggiDev/snip.git
cd snip

# Create a build directory
mkdir build
cd build

# Generate build files and compile
cmake ..
cmake --build . --config Release
```

Then add the compiled `snip.exe` (inside `build/Release` or `build/` depending on your compiler) to a folder in your `PATH`:

```powershell
mkdir C:\Users\<you>\bin
copy Release\snip.exe C:\Users\<you>\bin\snip.exe

# Add to PATH permanently (run once)
[Environment]::SetEnvironmentVariable("PATH", $env:PATH + ";C:\Users\<you>\bin", "User")
```

---

## Usage

```
snip add <cmd> [--tag <t>] [--desc <d>]     Save a new snippet
snip list [--tag <filter>]                  List all snippets
snip search <query>                         Search by command, tag, or description
snip show <id>                              Show full snippet details
snip run <id>                               Execute a snippet
snip copy <id>                              Copy snippet to clipboard
snip edit <id> [--command] [--tag] [--desc] Edit a snippet
snip delete <id>                            Delete a snippet
snip tags                                   List all tags
snip help                                   Show help
```

---

## Examples

```powershell
# Save a command
snip add "netsh wlan show profile name=\"MyWiFi\" key=clear" --tag wifi --desc "show wifi password"

# List all snippets
snip list

# Search by tag or keyword
snip search netsh

# Copy to clipboard
snip copy 1

# Run directly
snip run 2
```

---

## Data Storage

All snippets are stored locally in plain JSON format:

```
# Windows
C:\Users\<username>\.snip\snippets.json

# Linux / macOS
~/.snip/snippets.json
```

Easy to back up, version control, or share.

---

## Requirements

- CMake (version 3.10+)
- A C++17 compatible compiler

---

## License

MIT
