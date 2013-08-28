#ifndef __GG_POINTSHADER_H__
#define __GG_POINTSHADER_H__

/*
** �|�C���g
*/
#include "gg.h"

namespace gg
{

  class GgPointShader
    : public GgShader
  {
    // �ϊ�
    struct
    {
      GLfloat c[16];    // ���f���r���[�E���e�ϊ��s��
      void loadModelViewProjectionMatrix(const GgMatrix &m) { m.get(c); }
      GLfloat w[16];    // ���f���r���[�ϊ��s��
      void loadModelViewMatrix(const GgMatrix &m) { m.get(w); }
    } m;

    // �ꏊ
    struct
    {
      GLint mc;         // ���f���r���[�E���e�ϊ��s��� uniform �ϐ��̏ꏊ
      GLint mw;         // ���f���r���[�ϊ��s��� uniform �ϐ��̏ꏊ
    } loc;

  public:

    // �f�X�g���N�^
    virtual ~GgPointShader(void) {}

    // �R���X�g���N�^
    GgPointShader(void) {}
    GgPointShader(const char *vert, const char *frag = 0,
      const char *geom = 0, GLint nvarying = 0, const char **varyings = 0);
    GgPointShader(const GgPointShader &o)
      : GgShader(o), m(o.m), loc(o.loc) {}

    // ���
    GgPointShader &operator=(const GgPointShader &o)
    {
      if (&o != this)
      {
        GgShader::operator=(o);
        m = o.m;
        loc = o.loc;
      }
      return *this;
    }

    // �V�F�[�_�̎g�p�J�n
    virtual void use(void) const;

    // �V�F�[�_�̎g�p�I��
    virtual void unuse(void) const;

    // �ϊ��s��̐ݒ�
    virtual void loadMatrix(const GgMatrix &mp, const GgMatrix &mw);
    virtual void loadMatrix(const GLfloat *mp, const GLfloat *mw);
  };

}

#endif
