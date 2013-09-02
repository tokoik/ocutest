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
const vec4 k = vec4(0.2, 0.1, 0.0, 0.0);            // ピンクッション／バレルひずみ補正
const vec2 s = vec2(0.7, 0.7);                      // レンズ補正による縮小を調整する拡大係数

void main(void)
{
  vec2 d = t * s - LensCenter;
  vec4 r;
  r.x = dot(d, d);	// r^2
  r.y = r.x * r.x;	// r^4
  r.z = r.y * r.x;	// r^6
  r.w = r.z * r.x;	// r^8
  vec2 tc = (d * (1.0 + dot(r, k)) + vec2(1.0, -1.0)) * TextureSize;
  fc = texture(tex, tc);
}
