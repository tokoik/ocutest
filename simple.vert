#version 150 core
#extension GL_ARB_explicit_attrib_location : enable

// ���_����
layout (location = 0) in vec4 pv;                   // ���[�J�����W�n�̒��_�ʒu

// ���X�^���C�U�ɑ��钸�_����
out vec2 t;                                         // �e�N�X�`�����W

void main(void)
{
  t = pv.xy * vec2(0.375, -0.46875) + vec2(0.375, 0.46875);
  gl_Position = pv;
}
