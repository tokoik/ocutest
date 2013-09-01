#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

// テクスチャ
uniform sampler2D tex;

// ラスタライザから受け取る頂点属性の補間値
in vec2 t;                                          // テクスチャ座標

// フレームバッファに出力するデータ
layout (location = 0) out vec4 fc;                  // フラグメントの色

// テクスチャ
uniform vec2 TextureSize;                           // テクスチャ空間上のテクスチャのサイズ
uniform vec2 LensCenter;                            // レンズ中心のスクリーン中心からのずれ

// レンズ補正
const vec4 k = vec4(1.0, 0.0, 0.0, 0.0);            // ピンクッション／バレルひずみ補正
const vec2 s = vec2(0.6, 0.6);                      // レンズ補正による縮小を調整する拡大係数

void main(void)
{
  vec2 d = t * s - LensCenter;
  vec4 o;
  o.x = dot(d, d);
  o.y = o.x * o.x;
  o.z = o.y * o.x;
  o.w = o.z * o.x;
  vec2 tc = (d * (1.0 + dot(o, k)) + vec2(1.0, -1.0)) * TextureSize;
  fc = texture(tex, tc);
}
