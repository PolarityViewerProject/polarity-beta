/**
 * @file shinyV.glsl
 *
 * $LicenseInfo:firstyear=2007&license=viewerlgpl$
 * Second Life Viewer Source Code
 * Copyright (C) 2007, Linden Research, Inc.
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation;
 * version 2.1 of the License only.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 * 
 * Linden Research, Inc., 945 Battery Street, San Francisco, CA  94111  USA
 * $/LicenseInfo$
 */

uniform mat3 normal_matrix;
uniform mat4 texture_matrix0;
uniform mat4 texture_matrix1;
uniform mat4 modelview_matrix;
uniform mat4 modelview_projection_matrix;

attribute vec4 position;
attribute vec2 texcoord0;
attribute vec3 normal;
attribute vec4 diffuse_color;

vec4 calcLighting(vec3 pos, vec3 norm, vec4 color, vec4 baseCol);

void calcAtmospherics(vec3 inPositionEye);

varying float vary_texture_index;

uniform vec4 origin;

void main()
{
	//transform vertex
	vec4 vert = vec4(position.xyz,1.0);
	vary_texture_index = position.w;
	vec4 pos = (modelview_matrix * vert);
	gl_Position = modelview_projection_matrix*vec4(position.xyz, 1.0);
		
	vec3 norm = normalize(normal_matrix * normal);
	vec3 ref = reflect(pos.xyz, -norm);

	gl_TexCoord[0] = texture_matrix0 * vec4(texcoord0,0,1);
	gl_TexCoord[1] = texture_matrix1*vec4(ref,1.0);

	calcAtmospherics(pos.xyz);

	gl_FrontColor = calcLighting(pos.xyz, norm, diffuse_color, vec4(0.0));

	gl_FogFragCoord = pos.z;
}
