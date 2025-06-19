# SPITE Vulkan Graphics Engine - Comprehensive Architecture Documentation

## Table of Contents
1. [Executive Summary](#executive-summary)
2. [Architectural Overview](#architectural-overview)
3. [Layer Architecture](#layer-architecture)
4. [ECS Framework Design](#ecs-framework-design)
5. [Vulkan Integration Strategy](#vulkan-integration-strategy)
6. [Rendering Pipeline](#rendering-pipeline)
7. [Resource Management](#resource-management)
7. [Performance Characteristics](#performance-characteristics)

## Executive Summary

The SPITE (Scalable Performance-Intensive Technology Engine) is a modern 3D graphics engine built on data-oriented design principles and leveraging the Vulkan API for maximum performance. The engine employs an Entity-Component-System (ECS) architecture to manage complex rendering scenarios while maintaining high performance and scalability.

# ECS FRAMEWORK REVAMP INCOMING

### Key Features
- **Data-Oriented Design**: Optimized for cache locality and parallel processing
- **ECS Architecture**: Modular and extensible component-based system
- **Vulkan Integration**: Low-overhead GPU control with explicit resource management
- **Deferred Rendering**: Multi-pass rendering pipeline for complex lighting scenarios
- **Cross-Platform**: Designed for cross platform potential

## Architectural Overview

### High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                    SPITE ENGINE ARCHITECTURE                │
├─────────────────────────────────────────────────────────────┤
│                                                             │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐   │
│  │ APPLICATION   │  │    ENGINE     │  │     BASE      │   │
│  │    LAYER      │  │    LAYER      │  │    LAYER      │   │
│  │               │  │               │  │               │   │
│  │ • Window Mgmt │  │ • ECS Core    │  │ • Memory      │   │
│  │ • Input Sys   │  │ • Rendering   │  │ • Logging     │   │
│  │ • Event Mgmt  │  │ • Resources   │  │ • File I/O    │   │
│  │ • Time Mgmt   │  │ • Scene Graph │  │ • Math Utils  │   │
│  └───────────────┘  └───────────────┘  └───────────────┘   │
│           │                 │                 │             │
│           └─────────────────┼─────────────────┘             │
│                             │                               │
│                  ┌─────────────────────┐                    │
│                  │    VULKAN API       │                    │
│                  │  (Graphics Driver)  │                    │
│                  └─────────────────────┘                    │
└─────────────────────────────────────────────────────────────┘
```

### Design Principles

1. **Data-Oriented Design (DOD)**
   - Components store only data, no behavior
   - Systems operate on component data in bulk
   - Memory layout optimized for cache efficiency
   - Parallel processing capabilities

2. **Explicit Resource Management**
   - Manual GPU memory allocation using VMA
   - Explicit synchronization and command buffer management
   - Resource lifetime tracking and cleanup
   - Minimal driver overhead

3. **Modular Architecture**
   - Clear separation of concerns between layers
   - Plugin-style system architecture
   - Configurable rendering pipeline
   - Easy testing and debugging

## Layer Architecture

### Base Layer
**Purpose**: Platform abstraction and fundamental utilities

**Components**:
- **Memory Management**: Custom allocators (HeapAllocator, TLSF)
- **File System**: Cross-platform file I/O operations
- **Logging**: Multi-level logging system with filtering
- **Math Library**: GLM integration for vector/matrix operations
- **Platform Abstraction**: OS-specific functionality encapsulation

**Key Files**:
```
base/
├── Memory.cpp/.hpp         # Custom memory allocators
├── Logging.cpp/.hpp        # Logging system
├── File.cpp/.hpp          # File operations
├── Math.hpp               # Math utilities
├── Platform.hpp           # Platform-specific defines
├── VmaUsage.cpp/.hpp      # Vulkan Memory Allocator integration
└── Common.hpp             # Common types and macros
```

### Application Layer
**Purpose**: Application framework and windowing system

**Components**:
- **WindowManager**: SDL3 based window creation and management
- **InputManager**: Input event processing and action mapping
- **EventDispatcher**: Centralized event distribution system
- **TimeManager**: Frame timing and delta time calculation

**Key Files**:
```
application/
├── WindowManager.cpp/.hpp      # Window management
├── EventDispatcher.cpp/.hpp    # Event system
├── Time.cpp/.hpp              # Time management
└── input/
    ├── InputManager.cpp/.hpp   # Input processing
    ├── InputActionMap.cpp/.hpp # Key mapping
    └── Keycodes.hpp           # Input constants
```

### Engine Layer
**Purpose**: Core graphics engine functionality

**Components**:
- **ECS Framework**: Entity-component-system implementation
- **Vulkan Integration**: Vulkan API wrapper and resource management
- **Rendering Systems**: Multi-pass rendering pipeline
- **Resource Management**: Asset loading and GPU resource management

**Key Files**:
```
engine/
├── Common.cpp/.hpp             # Engine-wide utilities
├── Vulkan*.cpp/.hpp           # Vulkan subsystems
├── components/                # ECS components
│   ├── CoreComponents.hpp     # Basic components
│   ├── VulkanComponents.hpp   # Vulkan-specific components
│   └── InputEventComponents.hpp
└── systems/                   # ECS systems
    ├── core/                  # Core rendering systems
    └── MovementSystems.hpp    # Gameplay systems
```

## ECS Framework Design

### Core Concepts

#### Entities
- Unique identifiers (64-bit ID + version)
- Containers for components
- No behavior, only data aggregation

#### Components
Four component types based on usage patterns:

1. **IComponent**: Standard per-entity data
   ```cpp
   struct TransformComponent : IComponent {
       glm::vec3 position{0.0f};
       glm::vec3 rotation{0.0f};
       glm::vec3 scale{1.0f};
   };
   ```

2. **ISingletonComponent**: Global application state
   ```cpp
   struct VulkanInstanceComponent : ISingletonComponent {
       vk::Instance instance;
   };
   ```

3. **ISharedComponent**: Immutable shared data
   ```cpp
   struct TextureComponent : ISharedComponent {
       Image texture;
       vk::ImageView imageView;
       vk::Sampler sampler;
   };
   ```

4. **IEventComponent**: Asynchronous communication
   ```cpp
   struct ModelLoadRequest : IEventComponent {
       Entity entity;
       std::string objFilePath;
       std::string vertShaderPath;
       std::string fragShaderPath;
   };
   ```

#### Systems
- Process components in bulk
- Stateless where possible
- Dependency tracking for activation/deactivation

### Memory Management Strategy

#### Component Storage
- **Structure of Arrays**: Components stored in contiguous arrays
- **Cache-friendly**: Optimal memory layout for system iteration
- **TODO: Aspect**
#### Query Caching
- Cached query results for frequently accessed component combinations
- Automatic invalidation on structural changes
- Minimal overhead for system execution

## Vulkan Integration Strategy

### Component-Based Vulkan Resources

All Vulkan objects are represented as ECS components:

```cpp
// Infrastructure components (Singletons)
struct VulkanInstanceComponent : ISingletonComponent {
    vk::Instance instance;
};

struct DeviceComponent : ISingletonComponent {
    vk::Device device;
    QueueFamilyIndices queueFamilyIndices;
};

// Resource components
struct ShaderComponent : IComponent {
    vk::ShaderModule shaderModule;
    vk::ShaderStageFlagBits stage;
    std::string filePath;
};

struct PipelineComponent : IComponent {
    vk::Pipeline pipeline;
    Entity pipelineLayoutEntity;
    VertexInputData vertexInputData;
};
```

### Resource Lifecycle Management

1. **Creation**: Systems create Vulkan resources as components
2. **Usage**: Rendering systems query for resources by component type
3. **Cleanup**: Dedicated cleanup system handles resource destruction

### Synchronization Strategy

- **Frame-in-flight tracking**: Multiple command buffers for parallel frame processing
- **Explicit synchronization**: Manual fence and semaphore management
- **Resource state tracking**: Component-based state management

## Rendering Pipeline

### Deferred Rendering Architecture

The engine implements a multi-pass deferred rendering pipeline:

#### Pass 1: Depth Prepass
```cpp
class DepthPassSystem : public SystemBase {
    void onUpdate(float deltaTime) override {
        // Z-only rendering for early Z-rejection
        // Populate depth buffer
        // Minimal vertex shader, no fragment processing
    }
};
```

#### Pass 2: Geometry Pass (G-Buffer Generation)
```cpp
class GeometryPassSystem : public SystemBase {
    void onUpdate(float deltaTime) override {
        // Render to multiple render targets:
        // - Position (RGB16F)
        // - Normal (RGB16F)  
        // - Albedo (RGBA8)
        // - Material properties
    }
};
```

#### Pass 3: Lighting Pass
```cpp
class LightPassSystem : public SystemBase {
    void onUpdate(float deltaTime) override {
        // Fullscreen quad rendering
        // Sample G-Buffer textures
        // Calculate lighting for all light types
        // Output final color
    }
};
```

#### Pass 4: Presentation
```cpp
class PresentationSystem : public SystemBase {
    void onUpdate(float deltaTime) override {
        // Submit command buffers
        // Present to swapchain
        // Handle frame synchronization
    }
};
```

### Command Buffer Strategy

- **Primary command buffers**: One per frame in flight
- **Secondary command buffers**: For parallel recording (TODO)
- **Command buffer reuse**: Reset and rerecord each frame
- **Multi-threaded recording**: System-level parallelization

## Resource Management

### Memory Allocation Strategy

#### CPU Memory
- **HeapAllocator**: General-purpose allocation for application data
- **TLSF Allocator**: Real-time allocation for time-critical operations
- **Stack Allocators**: Temporary allocations within frame scope

#### GPU Memory
- **VMA Integration**: Vulkan Memory Allocator for optimal GPU memory management
- **Memory Type Selection**: Automatic selection based on usage patterns
- **Suballocation**: Efficient use of large memory blocks

### Asset Loading Pipeline

#### Asynchronous Loading
- **Event-driven**: ModelLoadRequest/TextureLoadRequest events
- **System processing**: Dedicated loading systems handle requests
- **GPU upload**: Staging buffers for efficient transfer

#### Resource Caching
- **Shared components**: Automatic deduplication of identical resources
- **Reference counting**: Automatic cleanup when no longer referenced

## Performance Characteristics

### Cache Optimization
- **Data locality**: Component arrays enable efficient CPU cache usage
- **Bulk operations**: Systems process multiple entities in tight loops
- **Minimal indirection**: Direct access to component data

### GPU Performance
- **Command buffer optimization**: Minimal state changes and draw calls
- **Memory bandwidth**: Efficient use of GPU memory hierarchies
- **Parallelization**: Multi-pass rendering enables GPU parallelism

### Scalability Features
- **System dependency tracking**: Automatic activation/deactivation
- **Component archetypes**: Efficient storage for different entity types
- **Resource pooling**: Reuse of frequently allocated resources

### Architecture Evolution
- **Significant performance optimizations/TODO**
- **Render graph**: Automatic dependency resolution for render passes
- **Asset streaming**: Dynamic loading/unloading based on proximity
- **Editor integration**: Visual scene editing and debugging tools

