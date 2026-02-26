#version 330 core

out vec4 FragColor;

in vec3 ourColor;

//uniform float visable;

void main()
{
	//FragColor = mix(texture(texture1, TexCoord), texture(texture2, TexCoord), visable);
	FragColor = vec4(ourColor, 1.0);
}
