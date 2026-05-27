# Lambda Installer for Windows
# Creates a local installation at ~/.lambda/bin and registers it to PATH

$InstallDir = Join-Path $HOME ".lambda\bin"
If (!(Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
}

$exeName = "lambda.exe"
$exePath = Join-Path $InstallDir $exeName

# 1. Try to compile locally first
$compiler = Get-Command gcc, clang -ErrorAction SilentlyContinue | Select-Object -First 1
$versionFolder = Get-ChildItem -Directory -Filter "lambda v*" | Sort-Object Name -Descending | Select-Object -First 1

if ($compiler -and $versionFolder) {
    Write-Host "Found local compiler: $($compiler.Name)"
    Write-Host "Compiling Lambda from source ($($versionFolder.Name))..."
    
    $sourceFile = Join-Path $versionFolder.FullName "main.c"
    
    # Run compiler
    & $compiler.Name $sourceFile -o $exePath
    
    if (Test-Path $exePath) {
        Write-Host "Lambda compiled and installed successfully to $exePath"
    } else {
        Write-Warning "Compilation failed. Attempting binary download..."
        $compiler = $null
    }
}

# 2. If compilation is not possible, download pre-compiled binary from GitHub Releases
if (!$compiler -or !(Test-Path $exePath)) {
    Write-Host "No compiler found. Downloading pre-compiled binary from GitHub Releases..."
    
    $repo = "n3rogamn1ng-source/Lambda-Turing-Complete-Programming-Language"
    $releasesUrl = "https://api.github.com/repos/$repo/releases/latest"
    
    try {
        # Fetch latest release assets URL
        $release = Invoke-RestMethod -Uri $releasesUrl -UseBasicParsing
        $asset = $release.assets | Where-Object { $_.name -eq $exeName }
        
        if ($asset) {
            $downloadUrl = $asset.browser_download_url
            Write-Host "Downloading $downloadUrl..."
            Invoke-WebRequest -Uri $downloadUrl -OutFile $exePath -UseBasicParsing
            Write-Host "Lambda downloaded and installed successfully to $exePath"
        } else {
            Write-Error "No pre-compiled $exeName found in the latest release on GitHub."
            exit 1
        }
    } catch {
        Write-Error "Failed to download pre-compiled binary: $_"
        exit 1
    }
}

# 3. Add bin folder to User PATH if not already present
$pathKey = "PATH"
$userPath = [Environment]::GetEnvironmentVariable($pathKey, [EnvironmentVariableTarget]::User)

if ($userPath -split ';' -notcontains $InstallDir) {
    Write-Host "Adding $InstallDir to user PATH..."
    $newUserPath = "$userPath;$InstallDir"
    [Environment]::SetEnvironmentVariable($pathKey, $newUserPath, [EnvironmentVariableTarget]::User)
    Write-Host "Path updated! Please restart your terminal for changes to take effect."
} else {
    Write-Host "$InstallDir is already in your PATH."
}

Write-Host "Lambda installation complete! Run 'lambda' to start."
