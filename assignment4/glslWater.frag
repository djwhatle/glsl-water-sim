
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
	/* DONE: add animated bump mapping */
	mat3 btn_matrix = mat3(Binormal, Tangent, Normal);

	/* sampling from three locations on bump map for extra realism */
	vec4 n0 = texture(BumpMapTexture, bumpCoord0)*2 - 1;
	vec4 n1 = texture(BumpMapTexture, bumpCoord1)*2 - 1;
	vec4 n2 = texture(BumpMapTexture, bumpCoord2)*2 - 1;
	vec3 n_bump = normalize(n0.xyz + n1.xyz + n2.xyz);
	
	/* computing new surface normal from bump map characteristic for location */
	vec3 bump_normal = btn_matrix * n_bump;

	/* DONE: add color */
	vec4 color_deep = vec4(0.0, 0.0, 0.1, 1.0);
	vec4 color_shallow = vec4(0.0, 0.5, 0.5, 1.0);
	float facing = 1 - max(dot(View, bump_normal), 0);
	fColor = mix(color_deep, color_shallow, facing);

	/* DONE: compute skybox reflection */
	/* reflecting  */
	vec4 reflection = texture(SkyboxTexture, reflect(-View, bump_normal));

	/* DONE: compute fresnel reflection */
	float r0 = 0.02037;
	float fast_fresnel = r0 + (1-r0)*pow((1-dot(View, bump_normal)), 5);

	/* DONE: compute fresnel refraction */
	/* Index of refraction is 1.0/1.33 for light moving from water to air */
	vec3 refraction = texture(SkyboxTexture, refract(-View, bump_normal, 1.0/1.33)).xyz;

	/* DONE: combine reflections and refraction */
	fColor = fColor + reflection*fast_fresnel
					+ refraction*(1-fast_fresnel);
}