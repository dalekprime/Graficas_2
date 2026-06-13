#pragma once
#ifndef SOLID_H
#define SOLID_H

#include "Node.h"
#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

class Solid {
public:
    static std::unique_ptr<Node> GenerateRevolution(const std::vector<glm::vec2>& profile, int segments, int axis, glm::vec3 color, const std::string& nodeName = "Solido_Revolucion");
    static std::unique_ptr<Node> GeneratePlane(float size);
    static std::vector<glm::vec2> EvaluateBezier(const std::vector<glm::vec2>& controlPoints, int resolution);
};

#endif