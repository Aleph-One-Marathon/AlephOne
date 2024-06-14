R"(

const int MAX_POINT_LIGHT_ELEMENTS = 448; //Must match MAX_POINT_LIGHT_ELEMENTS defined elsewhere
const int POINT_LIGHT_DATA_SIZE = 7; //Each point light needs 7 elements

const int MAX_SPOT_LIGHT_ELEMENTS = 72; //Must match MAX_SPOT_LIGHT_ELEMENTS defined elsewhere
const int SPOT_LIGHT_DATA_SIZE = 12; //Each spot light needs 12 elements

precision highp float;
uniform vec4 clipPlane0;
uniform vec4 clipPlane1;
uniform vec4 clipPlane5;
uniform sampler2D texture0;
uniform sampler2D texture1;
uniform float pulsate;
uniform float wobble;
uniform float glow;
uniform float flare;
uniform float selfLuminosity;
uniform float pointLights[MAX_POINT_LIGHT_ELEMENTS];
uniform float spotLights[MAX_SPOT_LIGHT_ELEMENTS];
uniform float areaLights[84];	//Must match MAX_AREA_LIGHT_ELEMENTS
uniform float fogMode;
uniform float fogStart;
uniform float fogEnd;

varying vec4 fogColor;
varying vec2 textureUV;
varying vec3 c;
varying vec3 viewXY;
varying vec3 viewDir;
varying vec4 vertexColor;
varying float FDxLOG2E;
varying mat3 tbnMatrix;
varying vec4 vPosition_eyespace;
varying vec3 eyespaceNormal;
varying mat3 normalMatrix;

float getFogFactor(float distance) {
	if (fogMode == 0.0) {
		return clamp((fogEnd - distance) / (fogEnd - fogStart), 0.0, 1.0);
	} else if (fogMode == 1.0) {
		return clamp(exp(-fogColor.a * distance), 0.0, 1.0);
	} else if (fogMode == 2.0) {
		return clamp(exp(-fogColor.a * fogColor.a * distance * distance), 0.0, 1.0);
	} else {
		return 1.0;
	}
}

vec4 pointLighting(vec4 baseColor, vec3 norm) {
	
	vec3 t = vec3(tbnMatrix[0][0], tbnMatrix[1][0], tbnMatrix[2][0]);
	vec3 b = vec3(tbnMatrix[0][1], tbnMatrix[1][1], tbnMatrix[2][1]);
	vec3 n = vec3(tbnMatrix[0][2], tbnMatrix[1][2], tbnMatrix[2][2]);
	
	vec4 lightAddition = vec4(0.0, 0.0, 0.0, 1.0);
	for(int i = 0; i < MAX_POINT_LIGHT_ELEMENTS; i += POINT_LIGHT_DATA_SIZE) {
		//Format of point light data: position, color, size (7 elements per light).
		//Unpack the data, and abort if size goes to near nothing (indicating end-of-list).
		float size = pointLights[i+6];
		if( size < .1) { break; } //End of light list
		vec3 lightPosition = vec3(pointLights[i], pointLights[i+1], pointLights[i+2]);
		vec4 lightColor = vec4(pointLights[i+3], pointLights[i+4], pointLights[i+5], 1.0);
		
		float distance = length(lightPosition - vPosition_eyespace.xyz);
		vec3 lightVector = normalize(lightPosition - vPosition_eyespace.xyz);
		
		vec3 lightVecTangent;
		lightVecTangent.x = dot(lightVector, t);
		lightVecTangent.y = dot(lightVector, b);
		lightVecTangent.z = dot(lightVector, n);
		lightVecTangent = normalize(lightVecTangent);
		
		float diffuse = max(dot(lightVecTangent, norm), 0.0);
		diffuse = diffuse * max((size*size - distance*distance)/(size*size), 0.0 ); //Attenuation
		
		lightAddition = lightAddition + baseColor * diffuse * lightColor;
	}
	
	return lightAddition;
}

vec4 spotLighting(vec4 baseColor, vec3 norm) {
	
	vec3 t = vec3(tbnMatrix[0][0], tbnMatrix[1][0], tbnMatrix[2][0]);
	vec3 b = vec3(tbnMatrix[0][1], tbnMatrix[1][1], tbnMatrix[2][1]);
	vec3 n = vec3(tbnMatrix[0][2], tbnMatrix[1][2], tbnMatrix[2][2]);
	
	vec4 lightAddition = vec4(0.0, 0.0, 0.0, 1.0);
	for(int i = 0; i < MAX_SPOT_LIGHT_ELEMENTS; i += SPOT_LIGHT_DATA_SIZE) {
		
		//Format of spot light data: position, direction, innerAngleRadiansCos, outerAngleRadiansCos, color, size (12 elements per light).
		//Unpack the data, and abort if size goes to near nothing (indicating end-of-list).
		float size = spotLights[i+11];
		if( size < .1) { break; } //End of light list
		vec3 lightPosition = vec3(spotLights[i], spotLights[i+1], spotLights[i+2]);
		vec3 spotlightDirection = normalize(vec3(spotLights[i+3], spotLights[i+4], spotLights[i+5])); //Input should be an eyespace vector.
		float innerLimitCos = spotLights[i+6]; //Cosine of inside angle (in radians). Light is at 100% inside of this angle.
		float outerLimitCos = spotLights[i+7]; //Cosine of outside angle (in radians). There is no light beyond this angle.
		vec4 lightColor = vec4(spotLights[i+8], spotLights[i+9], spotLights[i+10], 1.0);
		
		float distance = length(lightPosition - vPosition_eyespace.xyz);
		vec3 lightVector = normalize(lightPosition - vPosition_eyespace.xyz);
		
		vec3 lightVecTangent;
		lightVecTangent.x = dot(lightVector, t);
		lightVecTangent.y = dot(lightVector, b);
		lightVecTangent.z = dot(lightVector, n);
		lightVecTangent = normalize(lightVecTangent);
		
		float diffuse = max(dot(lightVecTangent, norm), 0.0);
		diffuse = diffuse * max((size*size - distance*distance)/(size*size), 0.0 ); //Attenuation
		
		float dotFromDirection = dot(-lightVector, spotlightDirection);
		float inLight = smoothstep(outerLimitCos, innerLimitCos, dotFromDirection);
		
		if(dot(-lightVector, spotlightDirection) > outerLimitCos) {
			diffuse *= inLight; // apply the spotlight effect to the diffuse component
		} else {
			diffuse = 0.0; // light is outside of the spotlight cone
		}
		
		lightAddition = lightAddition + baseColor * diffuse * lightColor;
	}
	
	return lightAddition;
}

void main (void) {
   if( dot( vPosition_eyespace, clipPlane0) < 0.0 ) {discard;}
   if( dot( vPosition_eyespace, clipPlane1) < 0.0 ) {discard;}
   if( dot( vPosition_eyespace, clipPlane5) < 0.0 ) {discard;}
   vec3 texCoords = vec3(textureUV.xy, 0.0);
    vec3 normXY = normalize(viewXY);
    texCoords += vec3(normXY.y * -pulsate, normXY.x * pulsate, 0.0);
    texCoords += vec3(normXY.y * -wobble * texCoords.y, wobble * texCoords.y, 0.0);
    float mlFactor = clamp(selfLuminosity + flare - (length(viewDir)/8192.0), 0.0, 1.0);
    vec3 intensity;
    if (vertexColor.r > mlFactor) {
        intensity = vertexColor.rgb + (mlFactor * 0.5); }
    else {
        intensity = (vertexColor.rgb * 0.5) + mlFactor; }
    vec3 viewv = normalize(viewDir);
    // iterative parallax mapping
    float scale = 0.010;
    float bias = -0.005;
    for(int i = 0; i < 4; ++i) {
        vec4 normal = texture2D(texture1, texCoords.xy);
        float h = normal.a * scale + bias;
        texCoords.x += h * viewv.x;
        texCoords.y -= h * viewv.y;
    }
    vec3 norm = (texture2D(texture1, texCoords.xy).rgb - 0.5) * 2.0;
    float diffuse = 0.5 + abs(dot(norm, viewv))*0.5;
   if (glow > 0.001) {
       diffuse = 1.0;
   }
    vec4 color = texture2D(texture0, texCoords.xy);
    intensity = clamp(intensity * diffuse, glow, 1.0);
#ifdef GAMMA_CORRECTED_BLENDING
    intensity = intensity * intensity; // approximation of pow(intensity, 2.2)
#endif
	float fogFactor = getFogFactor(length(viewDir));
	
	vec4 lightAddition = pointLighting(color, norm);
	lightAddition += spotLighting(color, norm);
	//lightAddition += areaLighting(color, norm);

    gl_FragColor = vec4(mix(fogColor.rgb, lightAddition.rgb + color.rgb * intensity, fogFactor), vertexColor.a * color.a);
}

)"
