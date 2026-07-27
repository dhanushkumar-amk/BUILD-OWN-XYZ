# myshell - A Simple Cross-Platform C++ Shell

`myshell` is a lightweight, cross-platform custom Command Line Interface (CLI) Shell written in C++17. It supports command parsing, execution of system commands, built-in shell commands, file I/O redirection, command pipelines, and background process execution.

---

## 🚀 Features

- **Cross-Platform Support**: Runs natively on both **Windows** (via Win32 API) and **Linux/macOS** (via POSIX `fork`/`exec`).
- **Built-in Commands**:
  - `cd [directory]` - Change current working directory.
  - `pwd` - Print current working directory.
  - `echo [arguments...]` - Print text to stdout.
  - `exit` - Terminate the shell session.
- **External Command Resolution**: Searches system `PATH` environment variable to run executables (e.g., `git`, `ping`, `whoami`).
- **Input & Output Redirection**:
  - `<` redirect standard input from a file.
  - `>` redirect standard output to a file.
- **Pipelining (`|`)**: Chain multiple commands together where the output of one command becomes the input to the next (`cmd1 | cmd2`).
- **Background Execution (`&`)**: Run commands asynchronously in the background.

---

## 🛠️ Prerequisites

- **C++ Compiler**: GCC (`g++`) with C++17 standard support.

---

## 🔨 How to Build

### Using `g++` directly:
```bash
g++ -std=c++17 -Wall -Wextra -O2 -o myshell myshell.cpp
```

### Using `make` (if installed):
```bash
make
```

---

## 💻 Usage Examples

### 1. Built-in Commands
```bash
myshell> pwd
D:\BUILD OWN XYZ\SHELL

myshell> echo Hello, World!
Hello, World!

myshell> cd ..
myshell> pwd
D:\BUILD OWN XYZ
```

### 2. File Redirection
Save command output to a file:
```bash
myshell> echo "Sample output text" > output.txt
```

Read input from a file:
```bash
myshell> sort < input.txt
```

### 3. Command Pipelines (`|`)
Chain multiple commands:
```bash
myshell> whoami | findstr "star"
```

### 4. Background Processes (`&`)
Run tasks in the background without blocking the shell:
```bash
myshell> ping 127.0.0.1 &
```

### 5. Exit Shell
```bash
myshell> exit
```

---

## 🏗️ Architecture & How It Works

1. **Tokenization (`tokenize`)**: Splits raw user input into words/tokens separated by whitespace.
2. **Parsing (`parse_pipeline`)**: Scans tokens to identify operators (`|`, `>`, `<`, `&`) and builds command structs.
3. **Executable Resolution (`resolve_executable`)**: Locates binary executables using path checking and the system `PATH` variable.
4. **Execution (`run_pipeline`)**:
   - **Win32 API (`_WIN32`)**: Uses `CreateProcessA` and `CreatePipe` for Windows process management.
   - **POSIX (`Linux/macOS`)**: Uses `fork()`, `execv()`, `dup2()`, and `pipe()` system calls.

---

## 📝 License
Free to use and modify for learning and educational purposes.
