//
// Created by Plutex on 2026-03-09.
//

#ifndef PLUENGINE_PHYSICSPOINTRENDERER_H
#define PLUENGINE_PHYSICSPOINTRENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "JoltShapeExtractor.h"
#include "PluSTL_FWD.h"

namespace Plu
{
	class JoltPointRenderer : public JoltShapeExtractor
	{
	public:
		JoltPointRenderer();
		~JoltPointRenderer();

		void BeginFrame();
		void AddBody(const JPH::Body& body, const glm::vec3& color = { 1.0f, 0.3f, 0.0f });
		void Render(const glm::mat4& viewProj, float pointSize = 4.0f);

	private:
		struct Point
		{
			glm::vec3 pos;
			glm::vec3 color;
		};

		GLuint m_vao = 0;
		GLuint m_vbo = 0;
		GLuint m_shader = 0;

		DynamicArray<Point> m_points;

		void Init();
		void Cleanup();
		static GLuint BuildShader();
	};
}

#endif //PLUENGINE_PHYSICSPOINTRENDERER_H