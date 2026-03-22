//
// Created by Plutex on 2026-03-09.
//

#ifndef PLUENGINE_PHYSICSWIREFRAMERENDERER_H
#define PLUENGINE_PHYSICSWIREFRAMERENDERER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "JoltShapeExtractor.h"
#include "PluSTL_FWD.h"

namespace JPH
{
	class Body;
}

namespace Plu
{
	class JoltWireframeRenderer : public JoltShapeExtractor
	{
	public:
		JoltWireframeRenderer();
		~JoltWireframeRenderer();

		void BeginFrame();
		void AddBody(const JPH::Body& body, const glm::vec3& color = { 0.0f, 1.0f, 0.0f });
		void Render(const glm::mat4& viewProj);

	private:
		struct Line
		{
			glm::vec3 a, b;
			glm::vec3 color;
		};

		GLuint m_vao = 0;
		GLuint m_vbo = 0;
		GLuint m_shader = 0;

		DynamicArray<Line> m_lines;

		void Init();
		void Cleanup();
		static GLuint BuildShader();
	};
}

#endif //PLUENGINE_PHYSICSWIREFRAMERENDERER_H