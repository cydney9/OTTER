#include "Mesh.h"
const uint32_t p1 = mesh.AddVertex(positions[0], nNorm, uvs[0], col);
const uint32_t p2 = mesh.AddVertex(positions[1], nNorm, uvs[1], col);
const uint32_t p3 = mesh.AddVertex(positions[2], nNorm, uvs[2], col);
const uint32_t p4 = mesh.AddVertex(positions[3], nNorm, uvs[3], col);
mesh.AddIndexTri(p1, p3, p2);
mesh.AddIndexTri(p1, p4, p3);