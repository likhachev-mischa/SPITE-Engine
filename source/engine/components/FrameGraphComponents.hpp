#pragma once  
#include "ecs/Core.hpp"  
#include <nlohmann/json.hpp>  
#include <string>

namespace spite  
{  
    enum FrameGraphResourceType  
    {  
        FrameGraphResourceType_Invalid = -1,  
        FrameGraphResourceType_Buffer = 0,  
        FrameGraphResourceType_Texture = 1,  
        FrameGraphResourceType_Attachment = 2,  
        FrameGraphResourceType_Reference = 3,  
    };  

    FrameGraphResourceType frameGraphResourceTypeFromString(const cstring str)  
    {  
        if (strcmp(str, "buffer") == 0) return FrameGraphResourceType_Buffer;  
        if (strcmp(str, "texture") == 0) return FrameGraphResourceType_Texture;  
        if (strcmp(str, "attachment") == 0) return FrameGraphResourceType_Attachment;  
        if (strcmp(str, "reference") == 0) return FrameGraphResourceType_Reference;  

        FrameGraphResourceType result = FrameGraphResourceType_Invalid;  
        SASSERTM(result != FrameGraphResourceType_Invalid,  
                 "Parsed frame graph resource type %s string is invalid\n")  
        return result;  
    }  

    struct FrameGraphResourceDeserialized  
    {  
        std::string type;  
        std::string name;  

        std::optional<std::string> format;  
        std::optional<std::array<int, 2>> resolution;  
        std::optional<std::string> op;  

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(FrameGraphResourceDeserialized, type, name, format, resolution, op)  
    };  

    struct FrameGraphResourceComponent : IComponent  
    {  
        eastl::string name;  
        u32 referenceCount{};  

        FrameGraphResourceType type{};  

        //image or buffer  
        Entity resource;  
        //producer FrameGraphNodeComponent  
        Entity producer;  
        //output resource  
        Entity output;  
    };  

    struct FrameGraphNodeComponent : IComponent  
    {  
        eastl::string name;  
        u32 referenceCount{};  

        Entity renderPass;  
        Entity framebuffer;  

        //FrameGraphResourceComponents  
        eastl::vector<Entity> inputs;  
        eastl::vector<Entity> outputs;  
        //other nodes  
        eastl::vector<Entity> edges;  
    };  
}
