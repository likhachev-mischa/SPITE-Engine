// Example usage of SPITE memory allocators
// This file demonstrates practical usage patterns

#include "MemoryOptimized.hpp"
#include "AllocatorGuide.hpp"
#include <iostream>

namespace spite
{
    // Example: Renderer using different allocators appropriately
    class ExampleRenderer
    {
    private:
        // Persistent render data - use global allocator
        global_vector<Mesh> loaded_meshes_;
        global_vector<Material> materials_;
        global_vector<Texture> textures_;
        
        // Small collections that are typically small - use SBO
        sbo_vector<Light*, 4> scene_lights_;      // Most scenes have 1-4 lights
        sbo_vector<Camera*, 2> active_cameras_;   // Usually 1-2 cameras
        
        // Pool allocator for frequent same-size allocations
        PoolAllocator<RenderCommand, 4096> render_command_pool_;
        
        // Frame scratch allocator for temporary data
        ScratchAllocator frame_scratch_;
        
    public:
        ExampleRenderer() : frame_scratch_(2 * 1024 * 1024, "RendererScratch") // 2MB scratch
        {
        }
        
        void render_frame()
        {
            // Reset scratch memory at frame start
            frame_scratch_.reset();
            
            // Use scratch allocator for temporary frame data
            ScratchAllocatorAdapter<EntityID> scratch_adapter(frame_scratch_);
            scratch_vector<EntityID> visible_entities(scratch_adapter);
            
            ScratchAllocatorAdapter<RenderCommand*> cmd_adapter(frame_scratch_);
            scratch_vector<RenderCommand*> render_queue(cmd_adapter);
            
            // Collect visible entities (temporary operation)
            collect_visible_entities(visible_entities);
            
            // Generate render commands using pool allocator
            for (EntityID entity : visible_entities) {
                RenderCommand* cmd = render_command_pool_.allocate();
                setup_render_command(cmd, entity);
                render_queue.push_back(cmd);
            }
            
            // Sort render queue (working with temporary data)
            std::sort(render_queue.begin(), render_queue.end(), 
                     [](const RenderCommand* a, const RenderCommand* b) {
                         return a->depth < b->depth;
                     });
            
            // Execute render commands
            for (RenderCommand* cmd : render_queue) {
                execute_render_command(cmd);
                render_command_pool_.deallocate(cmd); // Return to pool
            }
            
            // Scratch memory automatically available for reuse next frame
            // render_queue and visible_entities are automatically "freed"
        }
        
    private:
        void collect_visible_entities(scratch_vector<EntityID>& out_entities)
        {
            // Use scratch allocator for temporary culling data
            ScratchAllocatorAdapter<Frustum> frustum_adapter(frame_scratch_);
            scratch_vector<Frustum> camera_frustums(frustum_adapter);
            
            // Build frustums for all cameras
            for (Camera* camera : active_cameras_) {
                camera_frustums.emplace_back(build_frustum(camera));
            }
            
            // Cull entities against frustums (details omitted)
            // Results stored in out_entities
        }
        
        void setup_render_command(RenderCommand* cmd, EntityID entity)
        {
            // Setup render command (implementation details omitted)
        }
        
        void execute_render_command(RenderCommand* cmd)
        {
            // Execute render command (implementation details omitted)
        }
        
        Frustum build_frustum(Camera* camera)
        {
            return Frustum{}; // Placeholder
        }
    };
    
    // Example: String processing with scratch allocator
    class ConfigLoader
    {
    public:
        void load_config(const char* filename)
        {
            // Use frame scratch for temporary string operations
            auto& scratch = FrameScratchAllocator::get();
            
            // Create scratch-allocated containers
            auto lines = FrameScratchAllocator::make_vector<scratch_string8>();
            auto key_value_pairs = FrameScratchAllocator::make_map<scratch_string8, scratch_string8>();
            
            // Read file into scratch string
            auto file_content = FrameScratchAllocator::make_string();
            read_file_to_string(filename, file_content);
            
            // Split into lines using scratch memory
            split_string(file_content, '\n', lines);
            
            // Parse key-value pairs
            for (const auto& line : lines) {
                auto temp_line = line;
                trim_whitespace(temp_line);
                
                if (temp_line.empty() || temp_line[0] == '#') continue;
                
                size_t equals_pos = temp_line.find('=');
                if (equals_pos != scratch_string8::npos) {
                    auto key = FrameScratchAllocator::make_string();
                    auto value = FrameScratchAllocator::make_string();
                    
                    key.assign(temp_line.begin(), temp_line.begin() + equals_pos);
                    value.assign(temp_line.begin() + equals_pos + 1, temp_line.end());
                    
                    trim_whitespace(key);
                    trim_whitespace(value);
                    
                    key_value_pairs[key] = value;
                }
            }
            
            // Convert temporary data to persistent storage
            store_config_permanently(key_value_pairs);
            
            // All temporary strings and containers automatically cleaned up
            // when scratch allocator is reset
        }
        
    private:
        // Persistent config storage
        global_vector<ConfigEntry> config_entries_;
        
        void read_file_to_string(const char* filename, scratch_string8& out_content)
        {
            // Implementation details omitted
        }
        
        void split_string(const scratch_string8& input, char delimiter, 
                         scratch_vector<scratch_string8>& out_parts)
        {
            // Implementation details omitted
        }
        
        void trim_whitespace(scratch_string8& str)
        {
            // Implementation details omitted
        }
        
        void store_config_permanently(const scratch_unordered_map<scratch_string8, scratch_string8>& kv_pairs)
        {
            for (const auto& pair : kv_pairs) {
                ConfigEntry entry;
                // Convert scratch strings to persistent strings
                entry.key = eastl::string(pair.first.c_str());
                entry.value = eastl::string(pair.second.c_str());
                config_entries_.push_back(entry);
            }
        }
        
        struct ConfigEntry {
            eastl::string key;
            eastl::string value;
        };
    };
    
    // Example: ECS system using appropriate allocators
    class ParticleSystem
    {
    private:
        // Pool for particle instances (frequent alloc/dealloc)
        PoolAllocator<Particle, 8192> particle_pool_;
        
        // Active particles - use global vector (persistent)
        global_vector<Particle*> active_particles_;
        
        // Emitters - typically few, use SBO
        sbo_vector<ParticleEmitter*, 8> emitters_;
        
    public:
        void update(float delta_time)
        {
            // Use scratch allocator for temporary update data
            auto particles_to_remove = FrameScratchAllocator::make_vector<size_t>();
            
            // Update existing particles
            for (size_t i = 0; i < active_particles_.size(); ++i) {
                Particle* particle = active_particles_[i];
                particle->lifetime -= delta_time;
                
                if (particle->lifetime <= 0.0f) {
                    particles_to_remove.push_back(i);
                } else {
                    update_particle(particle, delta_time);
                }
            }
            
            // Remove dead particles (iterate backwards to maintain indices)
            for (auto it = particles_to_remove.rbegin(); it != particles_to_remove.rend(); ++it) {
                size_t index = *it;
                Particle* particle = active_particles_[index];
                
                // Return to pool
                particle_pool_.deallocate(particle);
                
                // Remove from active list
                active_particles_.erase(active_particles_.begin() + index);
            }
            
            // Spawn new particles from emitters
            for (ParticleEmitter* emitter : emitters_) {
                spawn_particles_from_emitter(emitter, delta_time);
            }
            
            // particles_to_remove automatically cleaned up when scratch is reset
        }
        
    private:
        struct Particle {
            float position[3];
            float velocity[3];
            float lifetime;
            float max_lifetime;
            // ... other particle data
        };
        
        struct ParticleEmitter {
            float position[3];
            float spawn_rate;
            float last_spawn_time;
            // ... other emitter data
        };
        
        void update_particle(Particle* particle, float delta_time)
        {
            particle->position[0] += particle->velocity[0] * delta_time;
            particle->position[1] += particle->velocity[1] * delta_time;
            particle->position[2] += particle->velocity[2] * delta_time;
        }
        
        void spawn_particles_from_emitter(ParticleEmitter* emitter, float delta_time)
        {
            emitter->last_spawn_time += delta_time;
            
            while (emitter->last_spawn_time >= 1.0f / emitter->spawn_rate) {
                Particle* new_particle = particle_pool_.allocate();
                initialize_particle(new_particle, emitter);
                active_particles_.push_back(new_particle);
                
                emitter->last_spawn_time -= 1.0f / emitter->spawn_rate;
            }
        }
        
        void initialize_particle(Particle* particle, ParticleEmitter* emitter)
        {
            // Initialize particle from emitter (implementation details omitted)
        }
    };
    
    // Example usage in main loop
    void example_main_loop()
    {
        ExampleRenderer renderer;
        ConfigLoader config_loader;
        ParticleSystem particle_system;
        
        // Load configuration once
        config_loader.load_config("game_config.txt");
        
        while (true) { // Game loop
            // Reset frame scratch at start of each frame
            FrameScratchAllocator::reset_frame();
            
            // Update systems
            particle_system.update(0.016f); // 60 FPS
            
            // Render
            renderer.render_frame();
            
            // Frame scratch automatically resets next iteration
        }
    }
}

/*
Key takeaways from this example:

1. SCRATCH ALLOCATOR USAGE:
   - Perfect for temporary per-frame data
   - String processing and parsing operations
   - Temporary collections for algorithms
   - Reset once per frame for maximum efficiency

2. POOL ALLOCATOR USAGE:
   - Objects that are frequently created/destroyed
   - Same-sized objects (particles, entities, etc.)
   - Avoids fragmentation and provides fast allocation

3. SBO VECTOR USAGE:
   - Small collections with predictable typical size
   - Avoids allocation for common case
   - Good for component lists, light lists, etc.

4. GLOBAL VECTOR USAGE:
   - Persistent data that lives longer than a frame
   - When you don't know the typical size
   - General-purpose container replacement

5. MEMORY PATTERN:
   - Persistent data: global_vector, eastl containers
   - Temporary data: scratch allocators
   - Small collections: sbo_vector
   - Frequent same-size allocs: pool allocators
*/
