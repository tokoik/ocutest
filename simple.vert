#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

// ���_����
layout (location = 0) in vec4 pv;                   // ���[�J�����W�n�̒��_�ʒu

// ���X�^���C�U�ɑ��钸�_����
out vec2 t;                                         // �e�N�X�`�����W

// �|���S���̒��S�ʒu
uniform vec2 ScreenCenter;

// �e�N�X�`�����W�̃X�P�[��
uniform vec2 TextureScale;

void main(void)
{
  t = pv.xy * TextureScale;
  gl_Position = vec4(pv.x * 0.5 + ScreenCenter.x, pv.y + ScreenCenter.y, 0.0, 1.0);
}
