#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

// �e�N�X�`��
uniform sampler2D tex;

// ���X�^���C�U����󂯎�钸�_�����̕�Ԓl
in vec2 t;                                          // �e�N�X�`�����W

// �t���[���o�b�t�@�ɏo�͂���f�[�^
layout (location = 0) out vec4 fc;                  // �t���O�����g�̐F

// �e�N�X�`��
uniform vec2 TextureSize;                           // �e�N�X�`����ԏ�̃e�N�X�`���̃T�C�Y
uniform vec2 LensCenter;                            // �����Y���S�̃X�N���[�����S����̂���

// �����Y�␳
const vec4 k = vec4(1.0, 0.0, 0.0, 0.0);            // �s���N�b�V�����^�o�����Ђ��ݕ␳
const vec2 s = vec2(0.6, 0.6);                      // �����Y�␳�ɂ��k���𒲐�����g��W��

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
