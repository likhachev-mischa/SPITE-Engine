#!/usr/bin/env python3
# filepath: c:\Computer Graphics\Vulkan\Projects\SPITE\shaders\compile_shaders.py
import os
import subprocess
import glob
import argparse
from pathlib import Path

def compile_shaders(vulkan_sdk_path=None):
    # Set default SDK path if not provided
    if not vulkan_sdk_path:
        # Default path from the batch file
        vulkan_sdk_path = "C:/VulkanSDK/1.3.290.0"
    
    # Ensure the path exists
    if not os.path.exists(vulkan_sdk_path):
        print(f"Error: Vulkan SDK path not found at {vulkan_sdk_path}")
        return False
    
    # Construct path to glslc compiler
    glslc_path = os.path.join(vulkan_sdk_path, "Bin", "glslc.exe")
    
    if not os.path.exists(glslc_path):
        print(f"Error: glslc compiler not found at {glslc_path}")
        return False
        
    # Find all shader files
    vert_shaders = glob.glob("*.vert")
    frag_shaders = glob.glob("*.frag")
    
    # Track success
    success = True
    compiled_count = 0
    
    # Compile all vertex shaders
    for shader in vert_shaders:
        output_file = Path(shader).stem + ".spv"
        print(f"Compiling {shader} to {output_file}...")
        
        try:
            result = subprocess.run(
                [glslc_path, shader, "-o", output_file],
                check=True,
                capture_output=True,
                text=True
            )
            compiled_count += 1
        except subprocess.CalledProcessError as e:
            print(f"Error compiling {shader}:")
            print(e.stderr)
            success = False
    
    # Compile all fragment shaders
    for shader in frag_shaders:
        output_file = Path(shader).stem + ".spv"
        print(f"Compiling {shader} to {output_file}...")
        
        try:
            result = subprocess.run(
                [glslc_path, shader, "-o", output_file],
                check=True,
                capture_output=True,
                text=True
            )
            compiled_count += 1
        except subprocess.CalledProcessError as e:
            print(f"Error compiling {shader}:")
            print(e.stderr)
            success = False
    
    print(f"\nCompilation complete. {compiled_count} shader files compiled.")
    return success

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compile GLSL shaders to SPIR-V")
    parser.add_argument("--sdk-path", help="Path to Vulkan SDK installation")
    args = parser.parse_args()
    
    success = compile_shaders(args.sdk_path)
    exit(0 if success else 1)