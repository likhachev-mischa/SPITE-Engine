import os
import subprocess
import sys

# Get the absolute path of the directory where this script is located.
script_dir = os.path.dirname(os.path.abspath(__file__))

# Get the filename of this script to avoid running it again.
this_script_name = os.path.basename(__file__)

print(f"Running scripts from: {script_dir}")

# Iterate over all files in the script's directory.
for filename in os.listdir(script_dir):
    # Check if the file is a Python script and not the current script.
    if filename.endswith(".py") and filename != this_script_name:
        script_path = os.path.join(script_dir, filename)
        print(f"\n--- Running script: {filename} ---")
        try:
            # Execute the script using the same Python interpreter that is running this script.
            # check=True will raise an exception if the script returns a non-zero exit code.
            subprocess.run([sys.executable, script_path], check=True)
            print(f"--- Finished script: {filename} ---")
        except subprocess.CalledProcessError as e:
            print(f"--- Error running {filename}: Returned non-zero exit status {e.returncode} ---")
        except Exception as e:
            print(f"--- An unexpected error occurred while running {filename}: {e} ---")

print("\nAll scripts have been executed.")