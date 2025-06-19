import os
import subprocess
import glob
import argparse
from pathlib import Path

def compile_shaders(vulkan_sdk_path=None, shader_dir=None):
    # Get the directory where this script is located
    script_dir = Path(__file__).parent
    
    # Set default shader directory relative to script location
    if not shader_dir:
        shader_dir = script_dir / "../shaders"
    
    # Resolve to absolute path
    shader_dir = Path(shader_dir).resolve()
    
    # Check if shader directory exists
    if not shader_dir.is_dir():
        print(f"Error: Shader directory not found at {shader_dir}")
        return False
    
    print(f"Compiling shaders from: {shader_dir}")
    
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
    
    # Change to shader directory to find shader files
    original_cwd = os.getcwd()
    os.chdir(shader_dir)
    
    try:
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
        
    finally:
        # Always restore the original working directory
        os.chdir(original_cwd)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compile GLSL shaders to SPIR-V")
    parser.add_argument("--sdk-path", help="Path to Vulkan SDK installation")
    parser.add_argument("--shader-dir", help="Path to shader directory (default: ../shaders relative to script)")
    args = parser.parse_args()
    
    success = compile_shaders(args.sdk_path, args.shader_dir)
    exit(0 if success else 1)