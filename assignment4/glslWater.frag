
#version 150

out vec4 fColor;

/* Cube- and Bumpmaps */
uniform samplerCube SkyboxTexture;
uniform sampler2D   BumpMapTexture;

/* Tangent basis */
in vec3	Binormal,
		Tangent,
		Normal;
				
/* View vector */
in vec3	View;

/* Multiple bump coordinates for animated bump mapping */
in vec2	bumpCoord0,
		bumpCoord1,
		bumpCoord2;


void main()
{   
	/* TODO: add color */
	vec4 color_deep = vec4(0.0, 0.0, 0.1, 1.0);
	vec4 color_shallow = vec4(0.0, 0.5, 0.5, 1.0);
	float facing = 1 - max(dot(View, Normal), 0);
	fColor = mix(color_deep, color_shallow, facing);

	/* TODO: add reflection */
	fColor = fColor + reflect(-View, Normal);
	/* TODO: add animated bump mapping */

	/* TODO: add refraction */

	/* TODO: add fresnel */


	/* fColor = vec4(1.0, 0.0, 1.0, 1.0); */
}