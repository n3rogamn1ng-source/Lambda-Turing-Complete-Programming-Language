Lambda is a super minimal interpreted scripting language written in C. It parses files line-by-line and executes them sequentially. 

### Design Philosophy: Strict Backward Compatibility
As of v0.0.5, we guarantee strict backward compatibility. Any code that runs on an older version of Lambda must run on all future versions without modifications. We do not break working code.

The syntax is dead simple:
- `inprint: "Prompt here"` -> Prints a prompt and waits for console input. It saves whatever you type into a built-in variable named `!`.
- `outprint: "Hello"` -> Prints a string literal.
- `outprint: !` -> Prints whatever is currently stored in the `!` variable.
- `endf` -> Stops execution immediately. Anything below this line is ignored.

To run code:
1. Go into the active version folder (like `lambda v0.0.5`).
2. Compile the engine: `gcc main.c -o lambda`
3. Run your file: `./lambda test.lmba`

To release a new version:
Just run `python release.py` in the root folder. The script will automatically:
1. Find the current version folder.
2. Compile and run tests to ensure no broken code is released.
3. Increment the patch version.
4. Rename the folder so the workspace stays clean.
5. Git commit and tag the new version.
