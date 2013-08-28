#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

// テクスチャ
uniform sampler2D tex;

// ラスタライザから受け取る頂点属性の補間値
in vec2 t;                                          // テクスチャ座標

// フレームバッファに出力するデータ
layout (location = 0) out vec4 fc;                  // フラグメントの色

void main(void)
{
  fc = texture(tex, t);
}
