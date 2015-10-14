
#version 150

/* Per-vertex input from application */
in vec3 vPosition;
in vec2 vTexCoord;

/* Uniform input from application */
/* Same value for all vertices */
uniform float time;
uniform mat4 ModelViewProjectionMatrix;
uniform vec3 vCameraPos;

/* Tangent basis */
out vec3	Binormal,
			Tangent,
			Normal;

/* View vector */
out vec3	View;

/* Multiple bump coordinates for animated bump mapping */
out vec2	bumpCoord0,
			bumpCoord1,
			bumpCoord2;

float wave_partial_derivative(float A, float D_x, float D_z, float f, float p, float k, float t, float x, float z, float D_partial);
float wave_generate(float A, float D_x, float D_z, float f, float x, float z, float p, float t, float k);

void main()
{
	vec4 P = vec4(vPosition, 1.0);
	
	View = normalize(vCameraPos - vPosition);

	/* TODO: Add waves to P */
	/* Height Y is parametrically from X and Z coordinates and two sine waves*/
	/* WAVE 1*/
	float amplitude = 1.0;
	float frequency = 0.2;
	float direction_x = -1.0;
	float direction_z = -0.7;
	float phase = 0.5;
	float sharpness = 2.0;

	float dG_dx_1 = wave_partial_derivative(amplitude, direction_x, direction_z, frequency, phase,
										    sharpness, time, vPosition.x, vPosition.z, direction_x);
	float dG_dz_1 = wave_partial_derivative(amplitude, direction_x, direction_z, frequency, phase,
										    sharpness, time, vPosition.x, vPosition.z, direction_z);
	float sine_result_1 = wave_generate(amplitude, direction_x, direction_z, frequency, vPosition.x,
										vPosition.z, phase, time, sharpness);
	/* WAVE 2 */
	amplitude = 0.5;
	frequency = 0.4;
	direction_x = 0.0;
	direction_z = 0.7;
	phase = 1.3;
	sharpness = 2.0;

	float dG_dx_2 = wave_partial_derivative(amplitude, direction_x, direction_z, frequency, phase,
										    sharpness, time, vPosition.x, vPosition.z, direction_x);
	float dG_dz_2 = wave_partial_derivative(amplitude, direction_x, direction_z, frequency, phase,
										    sharpness, time, vPosition.x, vPosition.z, direction_z);
	float sine_result_2 = wave_generate(amplitude, direction_x, direction_z, frequency, vPosition.x,
										vPosition.z, phase, time, sharpness);

	/* Apply changes to P */
	P.y = P.y + 3 * (sine_result_1 + sine_result_2);

	/* TODO: Compute B, T, N */
	float dH_dx = dG_dx_1 + dG_dx_2;
	float dH_dz = dG_dz_1 + dG_dz_2;

	Binormal = vec3(1, dH_dx, 0);
	Tangent =  vec3(0, dH_dz, 1);
	Normal  =  vec3(-dH_dx, 1, -dH_dz); 

	/* TODO: Compute bumpmap coordinates */
	vec2 texScale = vec2(8,4);
	float bumpTime = mod(time, 100.0);
	vec2 bumpSpeed = vec2(-0.05, 0);

	bumpCoord0.xy = vTexCoord.xy * texScale + bumpTime * bumpSpeed;
	bumpCoord1.xy = vTexCoord.xy * texScale * 2 + bumpTime * bumpSpeed * 4;
	bumpCoord2.xy = vTexCoord.xy * texScale * 4 + bumpTime * bumpSpeed * 8;
	
	gl_Position = ModelViewProjectionMatrix * P;
}

float wave_partial_derivative(float A, float D_x, float D_z, float f, float p, float k, float t, float x, float z, float D_partial) {
	return 0.5*k*f*A*pow((sin((D_x*x+D_z*z)*f+t*p)*0.5+0.5), k-1)*cos((D_x*x+D_z*z)*f+t*p)*D_partial;
}

float wave_generate(float A, float D_x, float D_z, float f, float x, float z, float p, float t, float k) {
	return A*pow((sin((D_x*x+D_z*z)*f+t*p)*0.5+0.5), k);
} 