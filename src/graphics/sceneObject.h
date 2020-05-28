#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class Mesh;

class SceneObject
{
public:
  SceneObject(Mesh* mesh);

private:
  // keep transform, translation and rotation in sync?
  glm::mat4 m_transform;
  glm::vec3 m_translation = { 0, 0, 0 };
  glm::vec3 m_scale = { 1, 1, 1 };
  glm::quat m_rotation;

  Mesh* m_mesh;
};
