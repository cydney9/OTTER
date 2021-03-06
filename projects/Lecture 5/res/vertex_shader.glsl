#version 410

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
//Lecture 5
layout(location = 2) in vec3 inNormal;

layout(location = 1) out vec3 outColor;

uniform mat4 MVP;

void main() {

	gl_Position = MVP * vec4(inPosition, 1.0);

	outColor = inColor;

}
	