#pragma once
#include <stdexcept>
#include <vector>
#include <string>
#include <fstream>
#include <glm/glm.hpp>

glm::vec4 lerp(const glm::vec4& a, const glm::vec4& b, float t);

float magnitudeSqr(const glm::vec4& vec);

std::vector<char> readBinaryFile(const std::string& filename);

void readModelInfoFile(const std::string& filename, std::vector<glm::vec2>& vertices,
                       std::vector<uint32_t>& indices);
