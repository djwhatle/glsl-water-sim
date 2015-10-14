
out vec4 fColor;

uniform sampler2D Sampler;
in vec2 texCoord;

void main()
{
	fColor = texture(Sampler, texCoord);
}