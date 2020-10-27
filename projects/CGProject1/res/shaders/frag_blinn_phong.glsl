// Specular
vec3 viewDir = normalize(u_CamPos - inPos);
vec3 h = normalize(lightDir + viewDir);
float spec = pow(max(dot(N, h), 0.0), u_Shininess); // Shininess coefficient (can be a uniform)