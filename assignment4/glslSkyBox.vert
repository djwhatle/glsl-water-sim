
in vec3 vPos;
in vec2 vTexCoord;

out vec2 texCoord;
uniform mat4 MVP;

void main()
{
	texCoord = vTexCoord.xy;
	gl_Position = MVP * vPos;
}