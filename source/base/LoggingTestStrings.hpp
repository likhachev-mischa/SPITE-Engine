#pragma once
#include <string>

#include "Platform.hpp"
//only for unit testing purpose

constexpr std::string TESTLOG_ECS_STRUCTURAL_CHANGE_HAPPENED(const cstring typeName) { return (std::string(typeName) + " structural change happened\n"); }
constexpr std::string TESTLOG_ECS_STRUCTURAL_CHANGE_COMMITED() { return "Structural change commited\n"; }
constexpr std::string TESTLOG_ECS_NEW_QUERY_CREATED(const cstring typeName) { return (std::string(typeName) + " new query created\n"); }
constexpr std::string TESTLOG_ECS_QUERY_LOADED_FROM_CACHE(const cstring typeName) { return (std::string(typeName) + " query loaded from cache\n"); }
