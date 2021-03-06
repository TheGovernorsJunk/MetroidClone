#ifndef TE_MESH_H
#define TE_MESH_H

#include "types.h"

#include <SDL_opengl.h>

namespace te {

template <typename PositionVec, typename TexVec>
struct Mesh {
	Vertex_array<PositionVec, TexVec> vertices;
	GLuint texture_id;
	GLenum mode;
};

using Mesh2 = Mesh<vec2, vec2>;
using Mesh3 = Mesh<vec3, vec2>;

template <typename PositionVec, typename TexVec>
void draw(const Mesh<PositionVec, TexVec>& mesh)
{
	glBindTexture(GL_TEXTURE_2D, mesh.texture_id);
	glBegin(mesh.mode);
	for (auto vertex : mesh.vertices) {
		glTexCoord2f(vertex.tex_coords.x, vertex.tex_coords.y);
		detail::draw_position_vertex(vertex.position);
	}
	glEnd();
}

struct Sprite_record;
Mesh2 make_mesh(const Sprite_record&, GLuint gl_texture_id);

namespace detail
{

template <typename V>
inline void draw_position_vertex(V);
template <>
inline void draw_position_vertex(vec2 position)
{
	glVertex2f(position.x, position.y);
}
template <>
inline void draw_position_vertex(vec3 position)
{
	glVertex3f(position.x, position.y, position.z);
}

} // namespace detail

} // namespace te

#endif
