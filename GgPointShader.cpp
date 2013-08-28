/*
** �_�����Ƃ��ĕ\������
*/
#include "GgPointShader.h"

gg::GgPointShader::GgPointShader(const char *vert, const char *frag,
  const char *geom, GLint nvarying, const char **varyings)
  : GgShader(vert, frag, geom, nvarying, varyings)
{
  // �v���O������
  GLuint program = get();

  // �ϊ��s��� uniform �ϐ��̏ꏊ
  loc.mc = glGetUniformLocation(program, "mc");
  loc.mw = glGetUniformLocation(program, "mw");
}

void gg::GgPointShader::use(void) const
{
  // ���N���X�̃V�F�[�_�̐ݒ���Ăяo��
  GgShader::use();

  // �ϊ�
  glUniformMatrix4fv(loc.mc, 1, GL_FALSE, m.c);
  glUniformMatrix4fv(loc.mw, 1, GL_FALSE, m.w);
}

void gg::GgPointShader::unuse(void) const
{
  // ���N���X�̃V�F�[�_�̐ݒ���Ăяo��
  GgShader::unuse();
}

void gg::GgPointShader::loadMatrix(const GgMatrix &mp, const GgMatrix &mw)
{
  m.loadModelViewProjectionMatrix(mp * mw);
  m.loadModelViewMatrix(mw);
}

void gg::GgPointShader::loadMatrix(const GLfloat *mp, const GLfloat *mw)
{
  GgMatrix tmp(mp), tmw(mw);
  loadMatrix(tmp, tmw);
}
