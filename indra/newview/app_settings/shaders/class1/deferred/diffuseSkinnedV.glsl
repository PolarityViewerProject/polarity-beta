/** 
 * @file diffuseSkinnedV.glsl
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

uniform mat4 projection_matrix;
uniform mat4 texture_matrix0;
uniform mat4 modelview_matrix;

ATTRIBUTE vec3 position;
ATTRIBUTE vec4 diffuse_color;
ATTRIBUTE vec3 normal;
ATTRIBUTE vec2 texcoord0;

VARYING vec3 vary_normal;
VARYING vec4 vertex_color;
VARYING vec2 vary_texcoord0;

mat4 getObjectSkinnedTransform();

void main()
{
	vary_texcoord0 = (texture_matrix0 * vec4(texcoord0,0,1)).xy;
		
	mat4 mat = modelview_matrix * getObjectSkinnedTransform();
	
	vec3 pos = (mat*vec4(position.xyz, 1.0)).xyz;
	
	vec4 norm = vec4(position.xyz, 1.0);
	norm.xyz += normal.xyz;
	norm.xyz = (mat*norm).xyz;
	norm.xyz = normalize(norm.xyz-pos.xyz);

	vary_normal = norm.xyz;
			
	vertex_color = diffuse_color;
	
	gl_Position = projection_matrix*vec4(pos, 1.0);
}
