#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

// 頂点属性
layout (location = 0) in vec4 pv;                   // ローカル座標系の頂点位置

// ラスタライザに送る頂点属性
out vec2 t;                                         // テクスチャ座標

// ポリゴンの中心位置
uniform vec2 ScreenCenter;

// テクスチャ座標のスケール
uniform vec2 TextureScale;

void main(void)
{
  t = pv.xy * TextureScale;
  gl_Position = vec4(pv.x * 0.5 + ScreenCenter.x, pv.y + ScreenCenter.y, 0.0, 1.0);
}
