/** 
 * @file multiPointLightV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * $/LicenseInfo$
 */


attribute vec3 position;
attribute vec4 diffuse_color;

varying vec4 vary_fragcoord;

void main()
{
	//transform vertex
	vec4 pos = gl_ModelViewProjectionMatrix * vec4(position.xyz, 1.0);
	vary_fragcoord = pos;

	gl_Position = pos;
	gl_FrontColor = diffuse_color;
}
