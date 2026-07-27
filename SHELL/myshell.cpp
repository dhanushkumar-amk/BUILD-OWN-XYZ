/*
 * myshell.cpp - A simple, cross-platform C++ Shell Implementation
 * 
 * Features supported:
 * 1. Built-in Commands: cd, pwd, echo, exit
 * 2. External Command Execution: PATH resolution for executables
 * 3. File Redirection: Input (<) and Output (>) redirection
 * 4. Command Piping: Connecting stdout of one command to stdin of another (cmd1 | cmd2)
 * 5. Background Execution: Running commands asynchronously with trailing '&'
 * 6. Cross-Platform: Native Win32 API support on Windows and POSIX (fork/exec) on Linux/macOS
 */

#include <cerrno>       // Standard error number definitions (e.g. errno)
#include <cstdlib>      // Standard general utilities (std::getenv, etc.)
#include <cstring>      // String manipulation utilities (std::strerror)
#include <filesystem>   // Cross-platform filesystem functions (C++17)
#include <iostream>     // Standard I/O streams (std::cout, std::cin, std::cerr)
#include <sstream>      // String stream utilities (std::istringstream)
#include <string>       // std::string implementation
#include <vector>       // Dynamic array structure (std::vector)

// Conditional include based on the Operating System platform
#ifdef _WIN32
// Windows-specific headers for process and pipe management
#include <windows.h>
#include <direct.h>
#include <io.h>
#include <process.h>
#else
// POSIX-specific headers for Linux/Unix/macOS systems
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

namespace {

// Prompt displayed to the user before receiving input
constexpr const char* kPrompt = "myshell> ";

#ifdef _WIN32
// Path environment variable separator on Windows (e.g. C:\path1;C:\path2)
constexpr char kPathSeparator = ';';
// Process identifier handle type on Windows
using ProcessHandle = HANDLE;
#else
// Path environment variable separator on POSIX systems (e.g. /usr/bin:/bin)
constexpr char kPathSeparator = ':';
// Process identifier type on POSIX systems
using ProcessHandle = pid_t;
#endif

/*
 * Structure representing a single command and its input/output redirections.
 * Example: "grep foo < input.txt > output.txt"
 *   args        = ["grep", "foo"]
 *   input_file  = "input.txt"
 *   output_file = "output.txt"
 */
struct Command {
    std::vector<std::string> args;   // Command name and argument list
    std::string input_file;          // File name for input redirection (<)
    std::string output_file;         // File name for output redirection (>)
};

/*
 * Structure representing a pipeline of one or more commands chained with '|'.
 * Example: "ls | grep test &"
 *   commands   = [Command("ls"), Command("grep test")]
 *   background = true
 */
struct Pipeline {
    std::vector<Command> commands;  // List of commands connected in the pipeline
    bool background = false;        // True if command ends with '&' (run in background)
};

/*
 * Tokenizes an input string into whitespace-delimited words/tokens.
 * Input:  "ls -l | grep test"
 * Output: ["ls", "-l", "|", "grep", "test"]
 */
std::vector<std::string> tokenize(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream stream(line);
    std::string token;

    // Read word by word from the stream
    while (stream >> token) {
        tokens.push_back(token);
    }

    return tokens;
}

/*
 * Parses a vector of tokens into a Pipeline structure.
 * Checks for syntax errors like missing arguments around operators ('|', '>', '<', '&').
 */
bool parse_pipeline(const std::vector<std::string>& tokens, Pipeline& pipeline, std::string& error) {
    pipeline = Pipeline{};
    Command current;

    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];

        // Handle Pipe Operator '|'
        if (token == "|") {
            if (current.args.empty()) {
                error = "syntax error: empty command before '|'";
                return false;
            }
            pipeline.commands.push_back(current);
            current = Command{};  // Reset for the next command in pipeline
            continue;
        }

        // Handle Output Redirection Operator '>'
        if (token == ">") {
            if (current.args.empty()) {
                error = "syntax error: missing command before '>'";
                return false;
            }
            if (i + 1 >= tokens.size()) {
                error = "syntax error: missing filename after '>'";
                return false;
            }
            if (!current.output_file.empty()) {
                error = "syntax error: multiple output redirections";
                return false;
            }
            current.output_file = tokens[++i];  // Set output file name and skip next token
            continue;
        }

        // Handle Input Redirection Operator '<'
        if (token == "<") {
            if (current.args.empty()) {
                error = "syntax error: missing command before '<'";
                return false;
            }
            if (i + 1 >= tokens.size()) {
                error = "syntax error: missing filename after '<'";
                return false;
            }
            if (!current.input_file.empty()) {
                error = "syntax error: multiple input redirections";
                return false;
            }
            current.input_file = tokens[++i];  // Set input file name and skip next token
            continue;
        }

        // Handle Background Execution Operator '&'
        if (token == "&") {
            if (current.args.empty() && pipeline.commands.empty()) {
                error = "syntax error: missing command before '&'";
                return false;
            }
            if (i + 1 != tokens.size()) {
                error = "syntax error: '&' must be the last token";
                return false;
            }
            pipeline.background = true;  // Mark pipeline as background process
            continue;
        }

        // Regular argument or command name
        current.args.push_back(token);
    }

    // Push the final command into the pipeline list
    if (!current.args.empty()) {
        pipeline.commands.push_back(current);
    }

    return true;
}

/*
 * Resolves full path for a given command name.
 * 1. Checks if command is a relative/absolute path.
 * 2. Searches directories listed in the system PATH environment variable.
 */
std::string resolve_executable(const std::string& command) {
    namespace fs = std::filesystem;

    // Helper lambda to check if a path points to an existing file
    auto is_exec = [](const fs::path& p) {
        std::error_code ec;
        return fs::is_regular_file(p, ec);
    };

    fs::path cmd_path(command);

    // Case 1: Command contains path separator (e.g. ./script.sh or C:/bin/app)
    if (cmd_path.has_parent_path()) {
        if (is_exec(cmd_path)) return cmd_path.string();
#ifdef _WIN32
        if (is_exec(cmd_path.string() + ".exe")) return cmd_path.string() + ".exe";
        if (is_exec(cmd_path.string() + ".cmd")) return cmd_path.string() + ".cmd";
        if (is_exec(cmd_path.string() + ".bat")) return cmd_path.string() + ".bat";
#endif
        return {};
    }

    // Case 2: Check current directory on Windows
#ifdef _WIN32
    if (is_exec(cmd_path)) return cmd_path.string();
    if (is_exec(cmd_path.string() + ".exe")) return cmd_path.string() + ".exe";
    if (is_exec(cmd_path.string() + ".cmd")) return cmd_path.string() + ".cmd";
    if (is_exec(cmd_path.string() + ".bat")) return cmd_path.string() + ".bat";
#endif

    // Case 3: Search inside PATH directories
    const char* path_env = std::getenv("PATH");
    if (path_env == nullptr) {
        return {};
    }

    std::string path_str(path_env);
    std::stringstream ss(path_str);
    std::string dir;

    // Split PATH environment variable by delimiter (':' or ';')
    while (std::getline(ss, dir, kPathSeparator)) {
        if (dir.empty()) continue;
        fs::path p = fs::path(dir) / command;
        if (is_exec(p)) return p.string();
#ifdef _WIN32
        if (is_exec(p.string() + ".exe")) return p.string() + ".exe";
        if (is_exec(p.string() + ".cmd")) return p.string() + ".cmd";
        if (is_exec(p.string() + ".bat")) return p.string() + ".bat";
#endif
    }

    return {};  // Command not found
}

/*
 * Checks and executes shell built-in commands: exit, pwd, cd, echo.
 * Built-ins run inside the shell process itself without creating sub-processes.
 */
bool handle_builtin(const Command& command, int& last_status, bool& should_exit) {
    if (command.args.empty()) {
        return false;
    }

    const std::string& name = command.args[0];

    // Built-in: 'exit' - Exit the shell
    if (name == "exit") {
        should_exit = true;
        last_status = 0;
        return true;
    }

    // Built-in: 'pwd' - Print Working Directory
    if (name == "pwd") {
        std::error_code ec;
        auto p = std::filesystem::current_path(ec);
        if (ec) {
            std::cerr << "myshell: pwd: " << ec.message() << '\n';
            last_status = 1;
        } else {
            std::cout << p.string() << '\n';
            last_status = 0;
        }
        return true;
    }

    // Built-in: 'cd' - Change Directory
    if (name == "cd") {
        std::string target;
        if (command.args.size() < 2) {
            // Default to home directory if no argument is provided
            const char* home = std::getenv("HOME");
#ifdef _WIN32
            if (!home) home = std::getenv("USERPROFILE");
#endif
            if (home == nullptr) {
                std::cerr << "myshell: cd: HOME/USERPROFILE not set\n";
                last_status = 1;
                return true;
            }
            target = home;
        } else {
            target = command.args[1];
        }

        std::error_code ec;
        std::filesystem::current_path(target, ec);
        if (ec) {
            std::cerr << "myshell: cd: " << target << ": " << ec.message() << '\n';
            last_status = 1;
        } else {
            last_status = 0;
        }
        return true;
    }

    // Built-in: 'echo' - Print arguments to standard output
    if (name == "echo") {
        for (size_t i = 1; i < command.args.size(); ++i) {
            if (i > 1) std::cout << ' ';
            std::cout << command.args[i];
        }
        std::cout << '\n';
        last_status = 0;
        return true;
    }

    return false;  // Not a built-in command
}

#ifdef _WIN32
/*
 * Reaps finished background child processes on Windows to clean up OS resources.
 */
void reap_background_children(std::vector<ProcessHandle>& background_pids, bool blocking) {
    for (size_t i = 0; i < background_pids.size();) {
        DWORD timeout = blocking ? INFINITE : 0;
        DWORD wait_res = WaitForSingleObject(background_pids[i], timeout);
        if (wait_res == WAIT_OBJECT_0) {
            CloseHandle(background_pids[i]);
            background_pids.erase(background_pids.begin() + i);
        } else {
            ++i;
        }
    }
}
#else
/*
 * Reaps finished background child processes on POSIX systems using waitpid().
 */
void reap_background_children(std::vector<ProcessHandle>& background_pids, bool blocking) {
    for (size_t i = 0; i < background_pids.size();) {
        int status = 0;
        pid_t result = waitpid(background_pids[i], &status, blocking ? 0 : WNOHANG);
        if (result > 0) {
            background_pids.erase(background_pids.begin() + static_cast<long>(i));
            continue;
        }
        if (!blocking || result < 0) {
            ++i;
        }
    }
}
#endif

#ifdef _WIN32
/*
 * Helper to escape and join Windows command line arguments into a single string.
 */
std::string build_win32_cmdline(const std::vector<std::string>& args) {
    std::string cmdline;
    for (size_t i = 0; i < args.size(); ++i) {
        if (i > 0) cmdline += " ";
        // Wrap arguments containing spaces or tabs in quotes
        if (args[i].find(' ') != std::string::npos || args[i].find('\t') != std::string::npos) {
            cmdline += "\"" + args[i] + "\"";
        } else {
            cmdline += args[i];
        }
    }
    return cmdline;
}

/*
 * Executes a pipeline of commands on Windows using Win32 CreateProcess and CreatePipe API.
 */
bool run_pipeline(const Pipeline& pipeline, int& last_status, bool& should_exit, std::vector<ProcessHandle>& background_pids) {
    if (pipeline.commands.empty()) {
        return true;
    }

    // Execute built-in commands directly if single command without redirection
    if (pipeline.commands.size() == 1 &&
        pipeline.commands[0].input_file.empty() &&
        pipeline.commands[0].output_file.empty()) {
        const Command& command = pipeline.commands[0];
        if (handle_builtin(command, last_status, should_exit)) {
            return true;
        }
    }

    const size_t command_count = pipeline.commands.size();

    // Configure security attributes for inheritable pipe handles
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE prev_pipe_read = NULL;
    std::vector<HANDLE> pipeline_pids;
    pipeline_pids.reserve(command_count);

    // Iterate through each command in the pipeline
    for (size_t i = 0; i < command_count; ++i) {
        const Command& command = pipeline.commands[i];
        if (command.args.empty()) {
            std::cerr << "myshell: syntax error: empty command in pipeline\n";
            last_status = 2;
            if (prev_pipe_read) CloseHandle(prev_pipe_read);
            return false;
        }

        // Find executable location
        std::string executable = resolve_executable(command.args[0]);
        if (executable.empty()) {
            std::cerr << "myshell: " << command.args[0] << ": command not found\n";
            last_status = 127;
            if (prev_pipe_read) CloseHandle(prev_pipe_read);
            return false;
        }

        HANDLE hInput = NULL;
        HANDLE hOutput = NULL;

        // Configure Input Handle (File Redirection < OR Pipe Input OR Standard Input)
        if (!command.input_file.empty()) {
            hInput = CreateFileA(command.input_file.c_str(), GENERIC_READ, FILE_SHARE_READ, &saAttr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hInput == INVALID_HANDLE_VALUE) {
                std::cerr << "myshell: cannot open input file '" << command.input_file << "'\n";
                last_status = 1;
                if (prev_pipe_read) CloseHandle(prev_pipe_read);
                return false;
            }
        } else if (i > 0) {
            hInput = prev_pipe_read;  // Input from previous command's pipe
        } else {
            hInput = GetStdHandle(STD_INPUT_HANDLE);
        }

        // Create pipe for the current command if there is a next command in the pipeline
        HANDLE next_pipe_read = NULL;
        HANDLE next_pipe_write = NULL;
        if (i + 1 < command_count) {
            if (!CreatePipe(&next_pipe_read, &next_pipe_write, &saAttr, 0)) {
                std::cerr << "myshell: pipe failed\n";
                last_status = 1;
                if (prev_pipe_read) CloseHandle(prev_pipe_read);
                if (hInput != GetStdHandle(STD_INPUT_HANDLE) && hInput != prev_pipe_read) CloseHandle(hInput);
                return false;
            }
        }

        // Configure Output Handle (File Redirection > OR Pipe Output OR Standard Output)
        if (!command.output_file.empty()) {
            hOutput = CreateFileA(command.output_file.c_str(), GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, &saAttr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if (hOutput == INVALID_HANDLE_VALUE) {
                std::cerr << "myshell: cannot open output file '" << command.output_file << "'\n";
                last_status = 1;
                if (prev_pipe_read) CloseHandle(prev_pipe_read);
                if (next_pipe_read) CloseHandle(next_pipe_read);
                if (next_pipe_write) CloseHandle(next_pipe_write);
                return false;
            }
        } else if (i + 1 < command_count) {
            hOutput = next_pipe_write;  // Output to next command's pipe
        } else {
            hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
        }

        // Ensure handles are inheritable by the child process
        SetHandleInformation(hInput, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
        SetHandleInformation(hOutput, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

        // Configure startup information with standard handle redirection
        STARTUPINFOA si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        si.dwFlags = STARTF_USESTDHANDLES;
        si.hStdInput = hInput;
        si.hStdOutput = hOutput;
        si.hStdError = GetStdHandle(STD_ERROR_HANDLE);

        PROCESS_INFORMATION pi;
        ZeroMemory(&pi, sizeof(pi));

        std::string cmdline = build_win32_cmdline(command.args);
        
        // Spawn child process on Windows
        BOOL success = CreateProcessA(
            executable.c_str(),
            &cmdline[0],
            NULL, NULL,
            TRUE,  // Inherit handles
            0, NULL, NULL,
            &si, &pi
        );

        // Clean up handles in parent process after spawning
        if (hInput != GetStdHandle(STD_INPUT_HANDLE) && hInput != prev_pipe_read) {
            CloseHandle(hInput);
        }
        if (prev_pipe_read) {
            CloseHandle(prev_pipe_read);
            prev_pipe_read = NULL;
        }
        if (hOutput != GetStdHandle(STD_OUTPUT_HANDLE) && hOutput != next_pipe_write) {
            CloseHandle(hOutput);
        }
        if (next_pipe_write) {
            CloseHandle(next_pipe_write);
        }

        if (!success) {
            std::cerr << "myshell: failed to execute '" << command.args[0] << "'\n";
            last_status = 1;
            if (next_pipe_read) CloseHandle(next_pipe_read);
            return false;
        }

        CloseHandle(pi.hThread);
        pipeline_pids.push_back(pi.hProcess);
        prev_pipe_read = next_pipe_read;
    }

    // Handle background process request ('&')
    if (pipeline.background) {
        for (HANDLE hProc : pipeline_pids) {
            background_pids.push_back(hProc);
        }
        last_status = 0;
        return true;
    }

    // Wait for all foreground pipeline processes to finish
    for (HANDLE hProc : pipeline_pids) {
        WaitForSingleObject(hProc, INFINITE);
        DWORD exit_code = 0;
        if (GetExitCodeProcess(hProc, &exit_code)) {
            last_status = static_cast<int>(exit_code);
        } else {
            last_status = 1;
        }
        CloseHandle(hProc);
    }

    return true;
}
#else
// Helper to close file descriptor safely
void close_fd(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

// Applies input/output redirections for child process on POSIX systems
bool apply_redirections(const Command& command, int input_fd, int output_fd, std::string& error) {
    if (input_fd >= 0) {
        if (dup2(input_fd, STDIN_FILENO) < 0) {
            error = std::string("myshell: dup2 stdin failed: ") + std::strerror(errno);
            return false;
        }
    } else if (!command.input_file.empty()) {
        int fd = open(command.input_file.c_str(), O_RDONLY);
        if (fd < 0) {
            error = std::string("myshell: cannot open input file '") + command.input_file +
                    "': " + std::strerror(errno);
            return false;
        }
        if (dup2(fd, STDIN_FILENO) < 0) {
            close_fd(fd);
            error = std::string("myshell: dup2 stdin failed: ") + std::strerror(errno);
            return false;
        }
        close_fd(fd);
    }

    if (output_fd >= 0) {
        if (dup2(output_fd, STDOUT_FILENO) < 0) {
            error = std::string("myshell: dup2 stdout failed: ") + std::strerror(errno);
            return false;
        }
    } else if (!command.output_file.empty()) {
        int fd = open(command.output_file.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd < 0) {
            error = std::string("myshell: cannot open output file '") + command.output_file +
                    "': " + std::strerror(errno);
            return false;
        }
        if (dup2(fd, STDOUT_FILENO) < 0) {
            close_fd(fd);
            error = std::string("myshell: dup2 stdout failed: ") + std::strerror(errno);
            return false;
        }
        close_fd(fd);
    }

    return true;
}

// Executes pipeline of commands on POSIX systems using fork(), pipe(), dup2(), execv()
bool run_pipeline(const Pipeline& pipeline, int& last_status, bool& should_exit, std::vector<ProcessHandle>& background_pids) {
    if (pipeline.commands.empty()) {
        return true;
    }

    if (pipeline.commands.size() == 1 &&
        pipeline.commands[0].input_file.empty() &&
        pipeline.commands[0].output_file.empty()) {
        const Command& command = pipeline.commands[0];
        if (handle_builtin(command, last_status, should_exit)) {
            return true;
        }
    }

    const size_t command_count = pipeline.commands.size();
    std::vector<int> pipe_read_fds(command_count, -1);
    std::vector<int> pipe_write_fds(command_count, -1);

    for (size_t i = 0; i + 1 < command_count; ++i) {
        int pipe_fds[2];
        if (pipe(pipe_fds) < 0) {
            std::cerr << "myshell: pipe failed: " << std::strerror(errno) << '\n';
            last_status = 1;
            return false;
        }
        pipe_read_fds[i + 1] = pipe_fds[0];
        pipe_write_fds[i] = pipe_fds[1];
    }

    std::vector<pid_t> pipeline_pids;
    pipeline_pids.reserve(command_count);

    for (size_t i = 0; i < command_count; ++i) {
        const Command& command = pipeline.commands[i];
        if (command.args.empty()) {
            std::cerr << "myshell: syntax error: empty command in pipeline\n";
            last_status = 2;
            break;
        }

        const std::string executable = resolve_executable(command.args[0]);
        if (executable.empty()) {
            std::cerr << "myshell: " << command.args[0] << ": command not found\n";
            last_status = 127;
            break;
        }

        pid_t pid = fork();
        if (pid < 0) {
            std::cerr << "myshell: fork failed: " << std::strerror(errno) << '\n';
            last_status = 1;
            break;
        }

        if (pid == 0) {
            const int input_fd = (i == 0) ? -1 : pipe_read_fds[i];
            const int output_fd = (i + 1 == command_count) ? -1 : pipe_write_fds[i];

            std::string error;
            if (!apply_redirections(command, input_fd, output_fd, error)) {
                std::cerr << error << '\n';
                _exit(1);
            }

            for (size_t j = 0; j < command_count; ++j) {
                close_fd(pipe_read_fds[j]);
                close_fd(pipe_write_fds[j]);
            }

            std::vector<char*> argv;
            argv.reserve(command.args.size() + 1);
            for (const std::string& arg : command.args) {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            argv.push_back(nullptr);

            execv(executable.c_str(), argv.data());
            std::cerr << "myshell: " << command.args[0] << ": " << std::strerror(errno) << '\n';
            _exit(126);
        }

        pipeline_pids.push_back(pid);
    }

    for (size_t i = 0; i < command_count; ++i) {
        close_fd(pipe_read_fds[i]);
        close_fd(pipe_write_fds[i]);
    }

    if (pipeline.background) {
        for (pid_t pid : pipeline_pids) {
            background_pids.push_back(pid);
        }
        last_status = 0;
        return true;
    }

    for (pid_t pid : pipeline_pids) {
        int status = 0;
        if (waitpid(pid, &status, 0) < 0) {
            std::cerr << "myshell: waitpid failed: " << std::strerror(errno) << '\n';
            last_status = 1;
            return false;
        }

        if (WIFEXITED(status)) {
            last_status = WEXITSTATUS(status);
        } else if (WIFSIGNALED(status)) {
            last_status = 128 + WTERMSIG(status);
        } else {
            last_status = 1;
        }
    }

    return true;
}
#endif

}  // namespace

/*
 * Main REPL (Read-Eval-Print Loop) function of the shell.
 */
int main() {
    std::string line;
    int last_status = 0;
    std::vector<ProcessHandle> background_pids;

    // Main shell event loop
    while (true) {
        // Clean up any finished background processes non-blockingly
        reap_background_children(background_pids, false);

        // Display shell prompt
        std::cout << kPrompt << std::flush;

        // Read command line input from user
        if (!std::getline(std::cin, line)) {
            std::cout << '\n';
            break;  // Exit on EOF (Ctrl+D or end of stream)
        }

        // Tokenize line into words/operators
        const std::vector<std::string> tokens = tokenize(line);
        if (tokens.empty()) {
            continue;  // Skip empty lines
        }

        // Parse tokens into pipeline structure
        Pipeline pipeline;
        std::string error;
        if (!parse_pipeline(tokens, pipeline, error)) {
            std::cerr << error << '\n';
            last_status = 2;
            continue;
        }

        if (pipeline.commands.empty()) {
            continue;
        }

        // Execute the parsed pipeline
        bool should_exit = false;
        if (!run_pipeline(pipeline, last_status, should_exit, background_pids)) {
            continue;
        }

        // Exit loop if user ran 'exit' builtin command
        if (should_exit) {
            break;
        }
    }

    // Wait for remaining background processes before shell shuts down completely
    while (!background_pids.empty()) {
        reap_background_children(background_pids, true);
    }

    return last_status;
}
