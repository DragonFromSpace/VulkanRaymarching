#version 460

layout(location = 0) in vec3 inColor;
layout(location = 1) in vec2 inTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform SceneData
{
    vec4 fogColor;
    vec4 fogDistances;
    vec4 ambientColor;
    vec4 sunlightDirection;
    vec4 sunlightColor;
}sceneData;

layout(set=1, binding = 4) uniform test
{
    vec3 nothing;
}Test;

layout(set = 2, binding = 0) uniform sampler2D tex1;

void main()
{
    vec3 col = texture(tex1, inTexCoord).xyz;
    outColor = vec4(col, 1.0f);
}