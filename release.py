import os
import re
import subprocess
import sys

def get_current_version_folder():
    for name in os.listdir('.'):
        if os.path.isdir(name) and re.match(r'^lambda v\d+\.\d+\.\d+$', name):
            return name
    return None

def main():
    folder = get_current_version_folder()
    if not folder:
        print("Error: Could not find a folder matching 'lambda vX.Y.Z'")
        sys.exit(1)

    version_str = folder.replace("lambda v", "")
    print(f"Current version found: {version_str} (in folder {folder})")

    # 1. Compile Check
    print("Testing: Compiling engine...")
    c_file_path = os.path.join(folder, "main.c")
    exe_name = "lambda.exe" if os.name == 'nt' else "lambda"
    exe_path = os.path.join(folder, exe_name)
    exe_path_abs = os.path.abspath(exe_path)
    
    # Remove existing binary if present
    if os.path.exists(exe_path):
        os.remove(exe_path)
        
    compile_res = subprocess.run(["gcc", c_file_path, "-o", exe_path])
    if compile_res.returncode != 0:
        print("Error: Compilation failed. Aborting release.")
        sys.exit(1)
        
    # 2. Execution Check
    print("Testing: Running tests...")
    # Run test in the folder context using the absolute path to the executable
    test_res = subprocess.run([exe_path_abs, "test.lmba"], cwd=folder, input=b"TestInput\n", capture_output=True)
    if test_res.returncode != 0:
        print("Error: Executable failed with non-zero exit code. Aborting release.")
        # Cleanup binary
        if os.path.exists(exe_path):
            os.remove(exe_path)
        sys.exit(1)

    print("Tests passed successfully!")
    
    # Cleanup compiled binary so it isn't tracked in git
    if os.path.exists(exe_path):
        os.remove(exe_path)

    # Increment version
    parts = list(map(int, version_str.split('.')))
    parts[2] += 1
    new_version_str = ".".join(map(str, parts))
    new_folder = f"lambda v{new_version_str}"
    
    print(f"Incrementing to version: {new_version_str}")
    print(f"Renaming folder '{folder}' to '{new_folder}'...")
    
    # Rename folder
    os.rename(folder, new_folder)
    
    # Git stage, commit and tag
    try:
        subprocess.run(["git", "rev-parse", "--is-inside-work-tree"], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        subprocess.run(["git", "add", "."], check=True)
        subprocess.run(["git", "commit", "-m", f"Release v{new_version_str}"], check=True)
        subprocess.run(["git", "tag", f"v{new_version_str}"], check=True)
        print(f"Successfully committed and tagged v{new_version_str} in Git!")
    except subprocess.CalledProcessError as e:
        print(f"Warning: Git command failed ({e}). Please ensure Git is configured and initialized.")

if __name__ == '__main__':
    main()
