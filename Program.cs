using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text.Json.Serialization;

// ─── Models ──────────────────────────────────────────────────────────────────

record Snippet
{
    public int Id { get; set; }
    public string Command { get; set; } = "";
    public List<string> Tags { get; set; } = new();
    public string Description { get; set; } = "";
    public DateTime CreatedAt { get; set; } = DateTime.UtcNow;
}

class SnipData
{
    public List<Snippet> Snippets { get; set; } = new();
    public int NextId { get; set; } = 1;
}

// ─── ANSI Color Helpers ───────────────────────────────────────────────────────

static class Ansi
{
    public static string Reset     => "\x1b[0m";
    public static string Bold      => "\x1b[1m";
    public static string Dim       => "\x1b[2m";
    public static string Red       => "\x1b[91m";
    public static string Green     => "\x1b[92m";
    public static string Yellow    => "\x1b[93m";
    public static string Blue      => "\x1b[94m";
    public static string Magenta   => "\x1b[95m";
    public static string Cyan      => "\x1b[96m";
    public static string White     => "\x1b[97m";
    public static string Gray      => "\x1b[90m";

    public static string Paint(string color, string text) => $"{color}{text}{Reset}";
    public static string B(string text) => $"{Bold}{text}{Reset}";
    public static string Truncate(string s, int maxLen)
    {
        if (string.IsNullOrEmpty(s)) return "";
        return s.Length > maxLen ? s[..(maxLen - 1)] + "…" : s;
    }
}

// ─── Storage ─────────────────────────────────────────────────────────────────

static class Store
{
    static readonly string DataDir  = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".snip");
    static readonly string DataFile = Path.Combine(DataDir, "snippets.json");

    static readonly JsonSerializerOptions JsonOpts = new()
    {
        WriteIndented = true,
        PropertyNamingPolicy = JsonNamingPolicy.CamelCase
    };

    public static SnipData Load()
    {
        if (!Directory.Exists(DataDir)) Directory.CreateDirectory(DataDir);
        if (!File.Exists(DataFile))
        {
            var empty = new SnipData();
            Save(empty);
            return empty;
        }
        var json = File.ReadAllText(DataFile);
        return JsonSerializer.Deserialize<SnipData>(json, JsonOpts) ?? new SnipData();
    }

    public static void Save(SnipData data)
    {
        if (!Directory.Exists(DataDir)) Directory.CreateDirectory(DataDir);
        File.WriteAllText(DataFile, JsonSerializer.Serialize(data, JsonOpts));
    }

    public static string DataFilePath => DataFile;
}

// ─── Display Helpers ─────────────────────────────────────────────────────────

static class Display
{
    const int IdW   = 4;
    const int TagW  = 18;
    const int CmdW  = 50;
    const int DescW = 32;

    public static void Table(IEnumerable<Snippet> snippets)
    {
        var list = snippets.ToList();
        if (list.Count == 0)
        {
            Console.WriteLine(Ansi.Paint(Ansi.Gray, "  No snippets found."));
            return;
        }

        string sep = Ansi.Paint(Ansi.Gray,
            "  " +
            new string('─', IdW + 2) + "┼" +
            new string('─', TagW + 2) + "┼" +
            new string('─', CmdW + 2) + "┼" +
            new string('─', DescW + 2));

        string header =
            "  " +
            Ansi.Paint(Ansi.Bold + Ansi.White, " ID".PadRight(IdW + 1))   + Ansi.Paint(Ansi.Gray, "│") +
            Ansi.Paint(Ansi.Bold + Ansi.White, " Tags".PadRight(TagW + 1)) + Ansi.Paint(Ansi.Gray, "│") +
            Ansi.Paint(Ansi.Bold + Ansi.White, " Command".PadRight(CmdW + 1)) + Ansi.Paint(Ansi.Gray, "│") +
            Ansi.Paint(Ansi.Bold + Ansi.White, " Description".PadRight(DescW + 1));

        Console.WriteLine();
        Console.WriteLine(header);
        Console.WriteLine(sep);

        foreach (var s in list)
        {
            string tags  = string.Join(" ", s.Tags.Select(t => $"[{t}]"));
            string idStr = Ansi.Paint(Ansi.Yellow, s.Id.ToString().PadLeft(IdW));
            string tagStr  = Ansi.Paint(Ansi.Cyan,  Ansi.Truncate(tags, TagW).PadRight(TagW));
            string cmdStr  = Ansi.Paint(Ansi.Green, Ansi.Truncate(s.Command, CmdW).PadRight(CmdW));
            string descStr = Ansi.Paint(Ansi.Gray,  Ansi.Truncate(s.Description, DescW).PadRight(DescW));
            Console.WriteLine($"  {idStr} {Ansi.Paint(Ansi.Gray, "│")} {tagStr} {Ansi.Paint(Ansi.Gray, "│")} {cmdStr} {Ansi.Paint(Ansi.Gray, "│")} {descStr}");
        }
        Console.WriteLine();
    }

    public static void Tag(string t) => Ansi.Paint(Ansi.Cyan, $"[{t}]");
}

// ─── Commands ─────────────────────────────────────────────────────────────────

static class Commands
{
    public static void Add(string[] args, Dictionary<string, string> flags)
    {
        if (args.Length == 0)
        {
            Console.WriteLine(Ansi.Paint(Ansi.Red, "  Error: ") + "Provide a command to save.");
            Console.WriteLine(Ansi.Paint(Ansi.Gray, "  Usage: snip add \"your command\" [--tag tagname] [--desc \"description\"]"));
            return;
        }

        var data = Store.Load();
        var id = data.NextId++;
        var tags = flags.ContainsKey("tag")
            ? flags["tag"].Split(',').Select(t => t.Trim()).ToList()
            : new List<string>();

        var snippet = new Snippet
        {
            Id = id,
            Command = args[0],
            Tags = tags,
            Description = flags.GetValueOrDefault("desc") ?? flags.GetValueOrDefault("description") ?? "",
            CreatedAt = DateTime.UtcNow
        };

        data.Snippets.Add(snippet);
        Store.Save(data);

        Console.WriteLine();
        Console.WriteLine(Ansi.Paint(Ansi.Green, "  ✓ Snippet saved!") + Ansi.Paint(Ansi.Gray, $" (id: {id})"));
        Console.WriteLine($"  {Ansi.Paint(Ansi.Gray, "CMD:")} {Ansi.Paint(Ansi.Green, snippet.Command)}");
        if (tags.Count > 0) Console.WriteLine($"  {Ansi.Paint(Ansi.Gray, "Tags:")} {string.Join(" ", tags.Select(t => Ansi.Paint(Ansi.Cyan, $"[{t}]")))}");
        if (!string.IsNullOrEmpty(snippet.Description)) Console.WriteLine($"  {Ansi.Paint(Ansi.Gray, "Desc:")} {snippet.Description}");
        Console.WriteLine();
    }

    public static void List(string[] args, Dictionary<string, string> flags)
    {
        var data = Store.Load();
        IEnumerable<Snippet> snippets = data.Snippets;

        if (flags.TryGetValue("tag", out var filterTag))
            snippets = snippets.Where(s => s.Tags.Any(t => t.Contains(filterTag, StringComparison.OrdinalIgnoreCase)));

        Console.WriteLine();
        Console.WriteLine(Ansi.B($"  {Ansi.Paint(Ansi.Magenta, "◆")} snip — {data.Snippets.Count} snippet(s)"));

        if (!data.Snippets.Any())
        {
            Console.WriteLine(Ansi.Paint(Ansi.Yellow, "\n  No snippets yet!"));
            Console.WriteLine(Ansi.Paint(Ansi.Gray, "  Run: snip add \"your command\" --tag mytag --desc \"what it does\"\n"));
            return;
        }

        Display.Table(snippets);
    }

    public static void Search(string[] args, Dictionary<string, string> flags)
    {
        if (args.Length == 0)
        {
            Console.WriteLine(Ansi.Paint(Ansi.Red, "  Error: ") + "Provide a search query.");
            Console.WriteLine(Ansi.Paint(Ansi.Gray, "  Usage: snip search <query>"));
            return;
        }

        var q = args[0].ToLower();
        var data = Store.Load();
        var results = data.Snippets.Where(s =>
            s.Command.Contains(q, StringComparison.OrdinalIgnoreCase) ||
            s.Description.Contains(q, StringComparison.OrdinalIgnoreCase) ||
            s.Tags.Any(t => t.Contains(q, StringComparison.OrdinalIgnoreCase))
        );

        Console.WriteLine();
        Console.WriteLine(Ansi.B($"  {Ansi.Paint(Ansi.Magenta, "◆")} Results for \"{Ansi.Paint(Ansi.Yellow, args[0])}\":"));
        Display.Table(results);
    }

    public static void Show(string[] args, Dictionary<string, string> flags)
    {
        if (args.Length == 0 || !int.TryParse(args[0], out int id))
        {
            Console.WriteLine(Ansi.Paint(Ansi.Red, "  Error: ") + "Provide a snippet ID.");
            return;
        }

        var data = Store.Load();
        var s = data.Snippets.FirstOrDefault(x => x.Id == id);
        if (s is null) { Console.WriteLine(Ansi.Paint(Ansi.Red, $"  Snippet #{id} not found.")); return; }

        Console.WriteLine();
        Console.WriteLine($"  {Ansi.Paint(Ansi.Yellow, Ansi.B($"#{s.Id}"))}  {string.Join(" ", s.Tags.Select(t => Ansi.Paint(Ansi.Cyan, $"[{t}]")))}");
        Console.WriteLine($"  {Ansi.Paint(Ansi.Gray, "Description:")} {(string.IsNullOrEmpty(s.Description) ? Ansi.Paint(Ansi.Gray, "(none)") : s.Description)}");
        Console.WriteLine($"  {Ansi.Paint(Ansi.Gray, "Created:")} {s.CreatedAt.ToLocalTime():yyyy-MM-dd HH:mm}");
        Console.WriteLine();
        Console.WriteLine($"  {Ansi.Paint(Ansi.Bold + Ansi.Green, "$ " + s.Command)}");
        Console.WriteLine();
    }

    public static void Run(string[] args, Dictionary<string, string> flags)
    {
        if (args.Length == 0 || !int.TryParse(args[0], out int id))
        {
            Console.WriteLine(Ansi.Paint(Ansi.Red, "  Error: ") + "Provide a snippet ID.");
            return;
        }

        var data = Store.Load();
        var s = data.Snippets.FirstOrDefault(x => x.Id == id);
        if (s is null) { Console.WriteLine(Ansi.Paint(Ansi.Red, $"  Snippet #{id} not found.")); return; }

        Console.WriteLine();
        Console.WriteLine($"  {Ansi.Paint(Ansi.Gray, "Running:")} {Ansi.Paint(Ansi.Green, s.Command)}");
        Console.WriteLine(Ansi.Paint(Ansi.Gray, "  " + new string('─', 60)));
        Console.WriteLine();

        bool isWindows = RuntimeInformation.IsOSPlatform(OSPlatform.Windows);
        var psi = new ProcessStartInfo
        {
            FileName = isWindows ? "cmd.exe" : "/bin/sh",
            Arguments = isWindows ? $"/c {s.Command}" : $"-c \"{s.Command}\"",
            UseShellExecute = false,
        };

        using var proc = Process.Start(psi);
        proc?.WaitForExit();
        Console.WriteLine();
    }

    public static void Copy(string[] args, Dictionary<string, string> flags)
    {
        if (args.Length == 0 || !int.TryParse(args[0], out int id))
        {
            Console.WriteLine(Ansi.Paint(Ansi.Red, "  Error: ") + "Provide a snippet ID.");
            return;
        }

        var data = Store.Load();
        var s = data.Snippets.FirstOrDefault(x => x.Id == id);
        if (s is null) { Console.WriteLine(Ansi.Paint(Ansi.Red, $"  Snippet #{id} not found.")); return; }

        try
        {
            bool isWindows = RuntimeInformation.IsOSPlatform(OSPlatform.Windows);
            bool isMac     = RuntimeInformation.IsOSPlatform(OSPlatform.OSX);

            string shell, arg;
            if (isWindows) { shell = "cmd.exe"; arg = $"/c echo {s.Command.Replace("\"", "\\\"")} | clip"; }
            else if (isMac) { shell = "/bin/sh"; arg = $"-c \"echo '{s.Command}' | pbcopy\""; }
            else { shell = "/bin/sh"; arg = $"-c \"echo '{s.Command}' | xclip -selection clipboard\""; }

            Process.Start(new ProcessStartInfo(shell, arg) { UseShellExecute = false })?.WaitForExit();

            Console.WriteLine();
            Console.WriteLine(Ansi.Paint(Ansi.Green, "  ✓ Copied to clipboard!"));
            Console.WriteLine($"  {Ansi.Paint(Ansi.Gray, "$")} {Ansi.Paint(Ansi.Green, s.Command)}");
            Console.WriteLine();
        }
        catch
        {
            Console.WriteLine();
            Console.WriteLine(Ansi.Paint(Ansi.Yellow, "  Could not copy. Here is your command:"));
            Console.WriteLine($"  {Ansi.Paint(Ansi.Green, s.Command)}");
            Console.WriteLine();
        }
    }

    public static void Delete(string[] args, Dictionary<string, string> flags)
    {
        if (args.Length == 0 || !int.TryParse(args[0], out int id))
        {
            Console.WriteLine(Ansi.Paint(Ansi.Red, "  Error: ") + "Provide a snippet ID.");
            return;
        }

        var data = Store.Load();
        var s = data.Snippets.FirstOrDefault(x => x.Id == id);
        if (s is null) { Console.WriteLine(Ansi.Paint(Ansi.Red, $"  Snippet #{id} not found.")); return; }

        data.Snippets.Remove(s);
        Store.Save(data);

        Console.WriteLine();
        Console.WriteLine(Ansi.Paint(Ansi.Green, $"  ✓ Deleted snippet #{id}") + Ansi.Paint(Ansi.Gray, $": {s.Command}"));
        Console.WriteLine();
    }

    public static void Edit(string[] args, Dictionary<string, string> flags)
    {
        if (args.Length == 0 || !int.TryParse(args[0], out int id))
        {
            Console.WriteLine(Ansi.Paint(Ansi.Red, "  Error: ") + "Provide a snippet ID.");
            return;
        }

        var data = Store.Load();
        var s = data.Snippets.FirstOrDefault(x => x.Id == id);
        if (s is null) { Console.WriteLine(Ansi.Paint(Ansi.Red, $"  Snippet #{id} not found.")); return; }

        if (flags.TryGetValue("command", out var cmd)) s = s with { Command = cmd };
        if (flags.TryGetValue("tag", out var tag)) s = s with { Tags = tag.Split(',').Select(t => t.Trim()).ToList() };
        if (flags.TryGetValue("desc", out var desc) || flags.TryGetValue("description", out desc))
            s = s with { Description = desc ?? "" };

        var idx = data.Snippets.FindIndex(x => x.Id == id);
        data.Snippets[idx] = s;
        Store.Save(data);

        Console.WriteLine();
        Console.WriteLine(Ansi.Paint(Ansi.Green, $"  ✓ Snippet #{id} updated!"));
        Console.WriteLine($"  {Ansi.Paint(Ansi.Gray, "CMD:")} {Ansi.Paint(Ansi.Green, s.Command)}");
        Console.WriteLine();
    }

    public static void Tags(string[] args, Dictionary<string, string> flags)
    {
        var data = Store.Load();
        var tagGroups = data.Snippets
            .SelectMany(s => s.Tags)
            .GroupBy(t => t)
            .OrderBy(g => g.Key);

        Console.WriteLine();
        if (!tagGroups.Any())
        {
            Console.WriteLine(Ansi.Paint(Ansi.Gray, "  No tags yet."));
        }
        else
        {
            Console.WriteLine(Ansi.B($"  {Ansi.Paint(Ansi.Magenta, "◆")} All tags:"));
            foreach (var g in tagGroups)
                Console.WriteLine($"  {Ansi.Paint(Ansi.Cyan, $"[{g.Key}]")} {Ansi.Paint(Ansi.Gray, $"{g.Count()} snippet(s)")}");
        }
        Console.WriteLine();
    }

    public static void Help()
    {
        Console.WriteLine();
        Console.WriteLine(Ansi.B(Ansi.Paint(Ansi.Magenta, "  ◆ snip") + Ansi.Paint(Ansi.White, " — CLI Command Snippet Manager")));
        Console.WriteLine();
        Console.WriteLine(Ansi.B(Ansi.Paint(Ansi.White, "  Usage:")));

        var cmds = new (string Cmd, string Desc)[]
        {
            ("snip add <cmd> [--tag <t>] [--desc <d>]",       "Save a new snippet"),
            ("snip list [--tag <filter>]",                     "List all snippets"),
            ("snip search <query>",                            "Search by command, tag, or description"),
            ("snip show <id>",                                 "Show full snippet details"),
            ("snip run <id>",                                  "Execute a snippet"),
            ("snip copy <id>",                                 "Copy snippet to clipboard"),
            ("snip edit <id> [--command] [--tag] [--desc]",   "Edit a snippet"),
            ("snip delete <id>",                               "Delete a snippet"),
            ("snip tags",                                      "List all tags"),
            ("snip help",                                      "Show this help"),
        };

        foreach (var (cmd, desc) in cmds)
            Console.WriteLine($"  {Ansi.Paint(Ansi.Green, cmd.PadRight(50))} {Ansi.Paint(Ansi.Gray, desc)}");

        Console.WriteLine();
        Console.WriteLine(Ansi.Paint(Ansi.Gray, $"  Data stored at: {Store.DataFilePath}"));
        Console.WriteLine();
    }
}

// ─── Arg Parser ───────────────────────────────────────────────────────────────

static class ArgParser
{
    public static (string[] positional, Dictionary<string, string> flags) Parse(string[] argv)
    {
        var positional = new List<string>();
        var flags = new Dictionary<string, string>(StringComparer.OrdinalIgnoreCase);

        int i = 0;
        while (i < argv.Length)
        {
            if (argv[i].StartsWith("--"))
            {
                var key = argv[i][2..];
                if (i + 1 < argv.Length && !argv[i + 1].StartsWith("--"))
                {
                    flags[key] = argv[i + 1];
                    i += 2;
                }
                else
                {
                    flags[key] = "true";
                    i++;
                }
            }
            else
            {
                positional.Add(argv[i]);
                i++;
            }
        }

        return (positional.ToArray(), flags);
    }
}

[System.Runtime.InteropServices.DllImport("kernel32.dll")] static extern IntPtr GetStdHandle(int nStdHandle);
[System.Runtime.InteropServices.DllImport("kernel32.dll")] static extern bool GetConsoleMode(IntPtr h, out uint mode);
[System.Runtime.InteropServices.DllImport("kernel32.dll")] static extern bool SetConsoleMode(IntPtr h, uint mode);

// ─── Entry Point ──────────────────────────────────────────────────────────────

Console.OutputEncoding = System.Text.Encoding.UTF8;

// Enable ANSI on Windows
if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
{
    var handle = GetStdHandle(-11);
    GetConsoleMode(handle, out uint mode);
    SetConsoleMode(handle, mode | 0x0004);
}

var (positional, flags) = ArgParser.Parse(args);
var command = positional.Length > 0 ? positional[0].ToLower() : null;
var rest = positional.Skip(1).ToArray();

switch (command)
{
    case "add":                 Commands.Add(rest, flags);    break;
    case "list": case "ls":     Commands.List(rest, flags);   break;
    case "search": case "find": Commands.Search(rest, flags); break;
    case "show":                Commands.Show(rest, flags);   break;
    case "run": case "exec":    Commands.Run(rest, flags);    break;
    case "copy": case "cp":     Commands.Copy(rest, flags);   break;
    case "delete": case "rm":   Commands.Delete(rest, flags); break;
    case "edit":                Commands.Edit(rest, flags);   break;
    case "tags":                Commands.Tags(rest, flags);   break;
    case "help": case "--help": case "-h":
    case null:                  Commands.Help();              break;
    default:
        Console.WriteLine(Ansi.Paint(Ansi.Red, $"  Unknown command: {command}"));
        Commands.Help();
        break;
}
