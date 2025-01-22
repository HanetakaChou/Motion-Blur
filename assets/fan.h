#ifndef _FAN_H_
#define _FAN_H_ 1

static constexpr unsigned int const fan_num_faces = 5168U;

extern unsigned int const *const fan_indices;

static constexpr unsigned int const fan_num_vertices = 15504U;

extern float const *const fan_vertices;

extern float const *const fan_normals;

extern float const *const fan_texture_coords;

#endif
