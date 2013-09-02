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
const vec4 k = vec4(0.2, 0.1, 0.0, 0.0);            // �s���N�b�V�����^�o�����Ђ��ݕ␳
const vec2 s = vec2(0.7, 0.7);                      // �����Y�␳�ɂ��k���𒲐�����g��W��

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
