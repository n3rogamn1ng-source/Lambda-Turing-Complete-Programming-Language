# Lambda Programming Language

Lambda is a super minimal, Turing-complete interpreted scripting language written from scratch in C. Under the hood, it compiles source code in-memory to bytecode and executes it on a fast Virtual Machine (VM) operand stack.

### Design Philosophy: Strict Backward Compatibility
As of `v0.1.0`, Lambda guarantees strict backward compatibility. Any code that runs on an older version of Lambda (from `v0.1.0` onwards) will compile and run on all future versions without modification. 

---

## ⚡ Quick Start

### 1. Simple Variable and Input Pipe Example
```lmba
# Capture console input directly into a custom variable
inprint: "Enter your name: " -> user_name
outprint: "Hello, "
outprint: user_name

# Perform math and print variables
set result = 100 - 5
outprint: "Math result:"
outprint: result
endf
```

### 2. Turing Completeness: Fibonacci Sequence (`fib.lmba`)
```lmba
set a = 0
set b = 1
set count = 0

label loop
outprint: a

set next = a + b
set a = b
set b = next

set count = count + 1
if count < 10 then:
    goto loop
endif
endf
```

---

## 💾 Installation

You can install Lambda automatically. The installer will compile Lambda locally (if you have `gcc`/`clang`) or download the pre-compiled binary from GitHub, register it in your home folder under `~/.lambda/bin`, and add it to your system PATH.

### Windows (PowerShell)
Run this command in PowerShell:
```powershell
Set-ExecutionPolicy Bypass -Scope Process -Force; [System.Net.ServicePointManager]::SecurityProtocol = [System.Net.ServicePointManager]::SecurityProtocol -bor 3072; iex ((New-Object System.Net.WebClient).DownloadString('https://raw.githubusercontent.com/n3rogamn1ng-source/Lambda-Turing-Complete-Programming-Language/main/install.ps1'))
```

### Linux / macOS (Shell)
Run this command in your terminal:
```bash
curl -fsSL https://raw.githubusercontent.com/n3rogamn1ng-source/Lambda-Turing-Complete-Programming-Language/main/install.sh | bash
```

---

## 🛠️ How to Compile and Run (Manual)

1. Go into the active version folder:
   ```bash
   cd "lambda v0.1.0"
   ```
2. Compile the engine using any C compiler:
   ```bash
   gcc main.c -o lambda
   ```
3. Run your `.lmba` file:
   ```bash
   ./lambda test.lmba
   ```

---

## 🚀 Automated Release Workflow

Lambda features a built-in safety-checking release script. When you run `python release.py` in the root directory, it will automatically:
1. Compile your C engine to verify there are no compiler errors.
2. Execute the test suite (`test.lmba`) to verify no runtime regressions exist.
3. If tests pass, it increments the release version, renames the folder (e.g. `lambda v0.1.0` -> `lambda v0.1.1`), and automatically commits and tags the release in Git.

---

## 📄 License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
