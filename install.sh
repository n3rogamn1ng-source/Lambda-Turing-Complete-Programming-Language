#!/bin/bash
# Lambda Installer for Linux/macOS
# Creates a local installation at ~/.lambda/bin and registers it to PATH

INSTALL_DIR="$HOME/.lambda/bin"
mkdir -p "$INSTALL_DIR"
EXE_NAME="lambda"
EXE_PATH="$INSTALL_DIR/$EXE_NAME"

# 1. Try to compile locally first
if command -v gcc >/dev/null 2>&1 || command -v clang >/dev/null 2>&1; then
    COMPILER=$(command -v gcc || command -v clang)
    VERSION_FOLDER=$(find . -maxdepth 1 -type d -name "lambda v*" | sort -V | tail -n 1)
    
    if [ -n "$VERSION_FOLDER" ]; then
        echo "Found local compiler: $COMPILER"
        echo "Compiling Lambda from source ($VERSION_FOLDER)..."
        $COMPILER "$VERSION_FOLDER/main.c" -o "$EXE_PATH"
        if [ -f "$EXE_PATH" ]; then
            echo "Lambda compiled and installed successfully to $EXE_PATH"
            chmod +x "$EXE_PATH"
        fi
    fi
fi

# 2. Download pre-compiled binary if compilation failed/skipped
if [ ! -f "$EXE_PATH" ]; then
    echo "No compiler found. Downloading pre-compiled binary from GitHub..."
    REPO="n3rogamn1ng-source/Lambda-Turing-Complete-Programming-Language"
    DOWNLOAD_URL="https://github.com/${REPO}/releases/latest/download/${EXE_NAME}"
    
    curl -fsSL "$DOWNLOAD_URL" -o "$EXE_PATH"
    if [ -f "$EXE_PATH" ]; then
        echo "Lambda downloaded and installed successfully to $EXE_PATH"
        chmod +x "$EXE_PATH"
    else
        echo "Error: Failed to download pre-compiled binary."
        exit 1
    fi
fi

# 3. Add to PATH
SHELL_PROFILE=""
if [[ "$SHELL" == */zsh ]]; then
    SHELL_PROFILE="$HOME/.zshrc"
elif [[ "$SHELL" == */bash ]]; then
    SHELL_PROFILE="$HOME/.bashrc"
fi

if [ -n "$SHELL_PROFILE" ]; then
    if ! grep -q "$INSTALL_DIR" "$SHELL_PROFILE"; then
        echo "Adding $INSTALL_DIR to PATH in $SHELL_PROFILE..."
        echo "export PATH=\"\$PATH:$INSTALL_DIR\"" >> "$SHELL_PROFILE"
        echo "Please restart your terminal or run: source $SHELL_PROFILE"
    fi
else
    echo "Please add $INSTALL_DIR to your PATH manually."
fi

echo "Lambda installation complete!"
