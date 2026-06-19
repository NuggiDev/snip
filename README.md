# snip ✂️

> A personal CLI command snippet manager — save, search, tag, and run your most-used terminal commands. No more Googling the same command for the 5th time.

Built in C# (.NET 8), runs as a single self-contained `.exe` on Windows.

---

## Installation

### Build from source

```powershell
git clone https://github.com/NuggiDev/snip.git
cd snip
dotnet publish -c Release -r win-x64 --self-contained true -p:PublishSingleFile=true -o publish
```

Then add `publish\snip.exe` to a folder in your `PATH`:

```powershell
mkdir C:\Users\<you>\bin
copy publish\snip.exe C:\Users\<you>\bin\snip.exe

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

# Save a WSL2 port forward command
snip add "netsh interface portproxy add v4tov4 listenport=3000 listenaddress=0.0.0.0 connectport=3000 connectaddress=$(wsl hostname -I)" --tag wsl --desc "WSL2 port forward"

# List all snippets
snip list

# Search by tag or keyword
snip search wsl
snip search netsh

# Copy to clipboard
snip copy 1

# Run directly
snip run 2

# Edit a snippet
snip edit 1 --desc "Updated description"

# Delete
snip delete 3
```

---

## Data Storage

All snippets are stored locally at:

```
~/.snip/snippets.json
```

Plain JSON — easy to back up, version control, or share.

---

## Requirements

- .NET 8 SDK (to build)
- Windows 10/11 (x64)

---

## License

MIT
