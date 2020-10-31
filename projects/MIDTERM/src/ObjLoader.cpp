#include "ObjLoader.h"

#include <string>
#include <sstream>
#include <fstream>
#include <iostream>

//Burrowed from GDW Project

// Borrowed from https://stackoverflow.com/questions/216823/whats-the-best-way-to-trim-stdstring
#pragma region String Trimming

// trim from start (in place)
static inline void ltrim(std::string& s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
		return !std::isspace(ch);
		}));
}

// trim from end (in place)
static inline void rtrim(std::string& s) {
	s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
		return !std::isspace(ch);
		}).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string& s) {
	ltrim(s);
	rtrim(s);
}

#pragma endregion 

VertexArrayObject::sptr ObjLoader::LoadFromFile(const std::string& filename)
{
	std::ifstream file;
	file.open(filename, std::ios::binary);

	if (!file)
		throw std::runtime_error("Failed to open file");

	MeshBuilder<VertexPosNormTexCol> mesh;
	std::string line;

	std::vector<float> vertexPos, texture, normal;
	std::string smoothing;
	std::istringstream ss;

	while (std::getline(file, line))
	{
		trim(line);

		if (line.substr(0, 1) != "#")
		{
			if (line.substr(0, 2) == "v ")
			{
				ss = std::istringstream(line.substr(2));

				vertexPos.push_back(0.0f);
				vertexPos.push_back(0.0f);
				vertexPos.push_back(0.0f);
				ss >> vertexPos[vertexPos.size() - 3] >> vertexPos[vertexPos.size() - 2] >> vertexPos[vertexPos.size() - 1];
			}
			else if (line.substr(0, 2) == "vt")
			{
				ss = std::istringstream(line.substr(3));

				texture.push_back(0.0f);
				texture.push_back(0.0f);
				ss >> texture[texture.size() - 2] >> texture[texture.size() - 1];
			}
			else if (line.substr(0, 2) == "vn")
			{
				ss = std::istringstream(line.substr(3));

				normal.push_back(0.0f);
				normal.push_back(0.0f);
				normal.push_back(0.0f);
				ss >> normal[normal.size() - 3] >> normal[normal.size() - 2] >> normal[normal.size() - 1];
			}
			else if (line.substr(0, 2) == "f ")
			{
				std::string FaceInfo = line.substr(2);

				for (int i = 0; i < FaceInfo.size(); i++)
				{
					if (FaceInfo.at(i) == '/')
						FaceInfo = FaceInfo.substr(0, i) + " " + FaceInfo.substr(i + 1);
				}

				ss = std::istringstream(FaceInfo);

				glm::vec3 p1Info, p2Info, p3Info;

				ss >> p1Info.x >> p1Info.y >> p1Info.z
					>> p2Info.x >> p2Info.y >> p2Info.z
					>> p3Info.x >> p3Info.y >> p3Info.z;

				glm::vec3 v1 = glm::vec3(vertexPos[(p1Info.x - 1) * 3], vertexPos[((p1Info.x - 1) * 3) + 1], vertexPos[((p1Info.x - 1) * 3) + 2]);
				glm::vec2 vt1 = glm::vec2(texture[(p1Info.y - 1) * 2], texture[((p1Info.y - 1) * 2) + 1]);
				glm::vec3 vn1 = glm::vec3(-normal[(p1Info.z - 1) * 3], -normal[((p1Info.z - 1) * 3) + 1], -normal[((p1Info.z - 1) * 3) + 2]);;

				glm::vec3 v2 = glm::vec3(vertexPos[(p2Info.x - 1) * 3], vertexPos[((p2Info.x - 1) * 3) + 1], vertexPos[((p2Info.x - 1) * 3) + 2]);
				glm::vec2 vt2 = glm::vec2(texture[(p2Info.y - 1) * 2], texture[((p2Info.y - 1) * 2) + 1]);
				glm::vec3 vn2 = glm::vec3(-normal[(p2Info.z - 1) * 3], -normal[((p2Info.z - 1) * 3) + 1], -normal[((p2Info.z - 1) * 3) + 2]);;

				glm::vec3 v3 = glm::vec3(vertexPos[(p3Info.x - 1) * 3], vertexPos[((p3Info.x - 1) * 3) + 1], vertexPos[((p3Info.x - 1) * 3) + 2]);
				glm::vec2 vt3 = glm::vec2(texture[(p3Info.y - 1) * 2], texture[((p3Info.y - 1) * 2) + 1]);
				glm::vec3 vn3 = glm::vec3(-normal[(p3Info.z - 1) * 3], -normal[((p3Info.z - 1) * 3) + 1], -normal[((p3Info.z - 1) * 3) + 2]);;

				for (int i = 0; i < vertexPos.size(); i++)
				{
					const uint32_t p1 = mesh.AddVertex(v1, glm::vec3(vt1.x, vt1.y, 0.0f), vn1, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
					const uint32_t p2 = mesh.AddVertex(v2, glm::vec3(vt2.x, vt2.y, 0.0f), vn2, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
					const uint32_t p3 = mesh.AddVertex(v3, glm::vec3(vt3.x, vt3.y, 0.0f), vn3, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

					mesh.AddIndexTri(p1, p3, p2);
				}
			}
			else if (line.substr(0, 2) == "s ")
			{
				smoothing = line.substr(2);
			}
		}
	}

	return mesh.Bake();
}
