#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

// 頂点属性
layout (location = 0) in vec4 pv;                   // ローカル座標系の頂点位置

// ラスタライザに送る頂点属性
out vec2 t;                                         // テクスチャ座標

void main(void)
{
  t = pv.xy * vec2(0.375, -0.46875) + vec2(0.375, 0.46875);
  gl_Position = pv;
}
