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

    # Extract version
    version_str = folder.replace("lambda v", "")
    print(f"Current version found: {version_str} (in folder {folder})")

    # Increment patch version (X.Y.Z -> X.Y.Z+1)
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
        # Check if git is initialized
        subprocess.run(["git", "rev-parse", "--is-inside-work-tree"], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        
        # Stage everything
        subprocess.run(["git", "add", "."], check=True)
        
        # Commit
        subprocess.run(["git", "commit", "-m", f"Release v{new_version_str}"], check=True)
        
        # Tag
        subprocess.run(["git", "tag", f"v{new_version_str}"], check=True)
        
        print(f"Successfully committed and tagged v{new_version_str} in Git!")
    except subprocess.CalledProcessError as e:
        print(f"Warning: Git command failed ({e}). Please ensure Git is configured and initialized.")

if __name__ == '__main__':
    main()
