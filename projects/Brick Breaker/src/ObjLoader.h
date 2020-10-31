#pragma once

#include "MeshBuilder.h"
#include "MeshFactory.h"

class ObjLoader
{
public:
	static VertexArrayObject::sptr LoadFile(const std::string& filename);
private:
	ObjLoader() = default;
	~ObjLoader() = default;
};