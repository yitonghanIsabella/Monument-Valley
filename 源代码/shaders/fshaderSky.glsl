#version 330 core
in vec3 TexCoords;	// 用一个三维方向向量来表示立方体贴图纹理的坐标
out vec4 color;        

uniform samplerCube skybox;  // 立方体贴图纹理采样器

void main()
{
    color = texture(skybox, TexCoords);
}