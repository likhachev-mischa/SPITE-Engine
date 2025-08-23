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
        # This is now the base directory for shaders
        shader_dir = script_dir / "../shaders"
    
    # Resolve to absolute path
    shader_dir = Path(shader_dir).resolve()
    
    source_dir = shader_dir / "source"
    output_dir = shader_dir / "bin"

    # Create output directory if it doesn't exist
    output_dir.mkdir(parents=True, exist_ok=True)

    # Check if source directory exists
    if not source_dir.is_dir():
        print(f"Error: Shader source directory not found at {source_dir}")
        return False
    
    print(f"Compiling shaders from: {source_dir} to {output_dir}")
    
    # Set default SDK path if not provided
    if not vulkan_sdk_path:
        # Default path from the batch file
        vulkan_sdk_path = "C:/VulkanSDK"
    
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
    shader_files = glob.glob(str(source_dir / "*.vert")) + glob.glob(str(source_dir / "*.frag"))
    
    # Track success
    success = True
    compiled_count = 0
    
    # Compile all shaders
    for shader_path_str in shader_files:
        shader_path = Path(shader_path_str)
        shader_filename = shader_path.name
        output_file_path = output_dir / (shader_filename + ".spv")
        
        print(f"Compiling {shader_filename} to {output_file_path}...")
        
        try:
            result = subprocess.run(
                [glslc_path, str(shader_path), "-o", str(output_file_path)],
                check=True,
                capture_output=True,
                text=True
            )
            compiled_count += 1
        except subprocess.CalledProcessError as e:
            print(f"Error compiling {shader_filename}:")
            print(e.stderr)
            success = False
            
    print(f"Compilation complete. {compiled_count} shader files compiled.")
    return success

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Compile GLSL shaders to SPIR-V")
    parser.add_argument("--sdk-path", help="Path to Vulkan SDK installation")
    parser.add_argument("--shader-dir", help="Path to shader directory (default: ../shaders relative to script)")
    args = parser.parse_args()
    
    success = compile_shaders(args.sdk_path, args.shader_dir)
    exit(0 if success else 1)
