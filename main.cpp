#include <iostream>

#include <opencv2/highgui/highgui.hpp>

#ifdef _WIN32
#  include <Windows.h>
#  define CV_VERSION_STR CVAUX_STR(CV_MAJOR_VERSION) CVAUX_STR(CV_MINOR_VERSION) CVAUX_STR(CV_SUBMINOR_VERSION)
#  ifdef _DEBUG
#    define CV_EXT_STR "d.lib"
#  else
#    define CV_EXT_STR ".lib"
#  endif
#  pragma comment(lib, "opencv_core" CV_VERSION_STR CV_EXT_STR)
#  pragma comment(lib, "opencv_highgui" CV_VERSION_STR CV_EXT_STR)
#else
#  include <pthread.h>
#  include <unistd.h>
#endif

// �⏕�v���O����
#include "gg.h"
using namespace gg;

// �V�F�[�_
#include "GgPointShader.h"

// �f�B�X�v���C�̃T�C�Y
#define DISPWIDTH 1280
#define DISPHEIGHT 800

// �L���v�`������摜�T�C�Y
#define CAPTWIDTH 640
#define CAPTHEIGHT 480

// �L���v�`������t���[�����[�g
#define CAPTFPS 30

// �e�N�X�`���T�C�Y
#define TEXWIDTH 512
#define TEXHEIGHT 512

// �g�p����e�N�X�`���̗̈�
#define IMGASPECT 1 // (DISPWIDTH / DISPHEIGHT / 2)
#define IMGWIDTH (CAPTHEIGHT * IMGASPECT)
#define IMGHEIGHT CAPTHEIGHT
#define IMGORIGINX ((CAPTWIDTH - IMGWIDTH) / 2)
#define IMGORIGINY 0

// �L���v�`���p�X���b�h
class CaptureWorker
{
  // �L���v�`��
  CvCapture *capture;

  // �e�N�X�`��
  GLenum format;
  GLsizei width, height;
  GLubyte *texture;
  GLfloat scale[2];

  // ���s���
  bool status;

  // �X���b�h�ƃ~���[�e�b�N�X
#ifdef _WIN32
  HANDLE thread;
  HANDLE mutex;
#else
  pthread_t thread;
  pthread_mutex_t mutex;
#endif

public:

  // �R���X�g���N�^
  CaptureWorker(int index, int width, int height, int fps)
  {
    // �J����������������
    capture = cvCreateCameraCapture(index);
    if (capture)
    {
      cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, static_cast<double>(width));
      cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, static_cast<double>(height));
      cvSetCaptureProperty(capture, CV_CAP_PROP_FPS, static_cast<double>(fps));
    }
    else
      std::cerr << "Cannot capture from camera " << index << std::endl;

    // �e�N�X�`���p�̃��������m�ۂ���
    texture = new GLubyte[width * height * 4];

    // �p�����[�^�ɏ����l��^���Ă���
    this->width = width;
    this->height = height;
    this->format = GL_BGRA;
    this->scale[0] = 0.5f * static_cast<GLfloat>(width) / static_cast<GLfloat>(TEXWIDTH);
    this->scale[1] = -0.5f * static_cast<GLfloat>(height) / static_cast<GLfloat>(TEXHEIGHT);

    // �X���b�h�ƃ~���[�e�b�N�X�𐶐�����
#ifdef _WIN32
    mutex = CreateMutex(NULL, TRUE, NULL);
    thread = CreateThread(NULL, 0, start, (LPVOID)this, 0, NULL);
#else
    pthread_mutex_init(&mutex, 0);
    pthread_create(&thread, 0, start, this);
#endif

    // �X���b�h�����s��Ԃł��邱�Ƃ��L�^����
    status = true;
  }

  // �f�X�g���N�^
  ~CaptureWorker()
  {
    // �X���b�h���~����
    stop();

#ifdef _WIN32
    // �X���b�h�̒�~��҂�
    WaitForSingleObject(thread, 0); 
    CloseHandle(thread);

    // �~���[�e�b�N�X��j������
    CloseHandle(mutex);
#else
    // �X���b�h�̒�~��҂�
    pthread_join(thread, 0);

    // �~���[�e�b�N�X��j������
    pthread_mutex_destroy(&mutex);
#endif

    // image �� release
    if (capture) cvReleaseCapture(&capture);

    // �������̉��
    delete[] texture;
  }

  // mutex �̃��b�N
  void lock(void)
  {
#ifdef _WIN32
    WaitForSingleObject(mutex, 0); 
#else
    pthread_mutex_lock(&mutex);
#endif
  }

  // mutex �̃����[�X
  void unlock(void)
  {
#ifdef _WIN32
    ReleaseMutex(mutex);
#else
    pthread_mutex_unlock(&mutex);
#endif
  }

  // �X���b�h�̊J�n
#ifdef _WIN32
  static DWORD WINAPI start(LPVOID arg)
#else
  static void *start(void *arg)
#endif
  {
    return reinterpret_cast<CaptureWorker *>(arg)->getTexture();
  }

  // �X���b�h�̒�~
  void stop(void)
  {
    lock();
    status = false;
    unlock();
  }

  // �X���b�h�̒�~����
  bool check(void)
  {
    bool ret;

    lock();
    ret = status;
    unlock();

    return ret;
  }

  // �e�N�X�`���쐬
#ifdef _WIN32
  DWORD WINAPI getTexture(void)
#else
  void *getTexture(void)
#endif
  {
    for (;;)
    {
      // �I�������̃e�X�g
      if (!check()) break;

      if (capture && cvGrabFrame(capture))
      {
        // �L���v�`���f������摜��؂�o��
        IplImage *image = cvRetrieveFrame(capture);

        if (image)
        {
          // �؂�o�����摜�̃T�C�Y�ƃe�N�X�`�����W�̃X�P�[��
          height = image->height;
          width = height * IMGASPECT;
          if (width > image->width) width = image->width;
          scale[0] = 0.5f * static_cast<GLfloat>(width) / static_cast<GLfloat>(TEXWIDTH);
          scale[1] = -0.5f * static_cast<GLfloat>(height) / static_cast<GLfloat>(TEXHEIGHT);

          // �؂�o�����摜�̎�ނ̔���
          if (image->nChannels == 3)
            format = GL_BGR;
          else if (image->nChannels == 4)
            format = GL_BGRA;
          else
            format = GL_RED;

          // �e�N�X�`���������ւ̓]��
          GLsizei size = IMGWIDTH * image->nChannels;
          GLsizei offset = size * IMGORIGINY + IMGORIGINX * image->nChannels;
          lock();
          for (int y = 0; y < image->height; ++y)
            memcpy(texture + size * y, image->imageData + image->widthStep * y + offset, size);
          unlock();
        }
        else
        {
          // �P�t���[�����҂�
#ifdef _WIN32
          Sleep(1000 / CAPTFPS);
#else
          usleep(1000000 / CAPTFPS);
#endif
        }
      }
    }

    return 0;
  }

  // �e�N�X�`���̃X�P�[��
  const GLfloat *getScale(void)
  {
    return scale;
  }

  // �e�N�X�`���]��
  void subTexture(void)
  {
    lock();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, texture);
    unlock();
  }
};

//
// ��`�|���S���̍쐬
//
static GLuint rectangle(void)
{
  // ���_�z��I�u�W�F�N�g���쐬���Č�������
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // ���_�o�b�t�@�I�u�W�F�N�g���쐬���Č�������
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  // �o�b�t�@�I�u�W�F�N�g���m�ۂ���
  static const GLfloat p[] =
  {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    1.0f,  1.0f,
    -1.0f,  1.0f
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof p, p, GL_STATIC_DRAW);

  // ���_�ʒu�� index == 0 �� in �ϐ����瓾��
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  return vao;
}

//
// �e�N�X�`���̍쐬
//
static GLuint texture(GLsizei width, GLsizei height)
{
  // �e�N�X�`�����쐬���Č�������
  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  // �S�̂����F�̉摜����������
  GLubyte *texture = new GLubyte[width * height * 4];
  memset(texture, 0, width * height * 4);

  // �e�N�X�`�����������m�ۂ���
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, texture);

  // �摜�Ɏg�p�������������J������
  delete[] texture;

  // �e�N�X�`���̓�����ݒ肷��
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  return tex;
}

//
// �����ݒ�
//
static int init(const char *title)
{
  // GLFW ������������
  if (glfwInit() == GL_FALSE)
  {
    // �������Ɏ��s����
    std::cerr << "Error: Failed to initialize GLFW." << std::endl;
    return 1;
  }

  // OpenGL Version 3.2 Core Profile ��I������
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
  glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // �E�B���h�E���J��
  if (glfwOpenWindow(DISPWIDTH, DISPHEIGHT, 8, 8, 8, 8, 24, 0, GLFW_FULLSCREEN) == GL_FALSE)
  {
    // �E�B���h�E���J���Ȃ�����
    std::cerr << "Error: Failed to open GLFW window." << std::endl;
    return 1;
  }

  // �J�����E�B���h�E�ɑ΂���ݒ�
  glfwSwapInterval(1);
  glfwSetWindowTitle(title);

  // �⏕�v���O�����ɂ�鏉����
  ggInit();

  // �B�ʏ����̐ݒ�
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  // ����������
  return 0;
}

//
// �E�B���h�E�̃T�C�Y�ύX���̏���
//
static void GLFWCALL resize(int w, int h)
{
  // �E�B���h�E�S�̂��r���[�|�[�g�ɂ���
  glViewport(0, 0, w, h);
}

//
// �L�[�{�[�h
//
static void GLFWCALL keyboard(int key, int action)
{
  if (action == GLFW_PRESS)
  {
    switch (key)
    {
    case GLFW_KEY_SPACE:
      break;
    case GLFW_KEY_BACKSPACE:
    case GLFW_KEY_DEL:
      break;
    case GLFW_KEY_UP:
      break;
    case GLFW_KEY_DOWN:
      break;
    case GLFW_KEY_RIGHT:
      break;
    case GLFW_KEY_LEFT:
      break;
    default:
      break;
    }
  }
}

//
// ���C���v���O����
//
int main(int argc, const char * argv[])
{
  // �����ݒ�
  if (init("Oculus Test")) return 1;

  // �L���v�`���p�X���b�h�𐶐�����
  CaptureWorker cam0(1, CAPTWIDTH, CAPTHEIGHT, CAPTFPS);
  CaptureWorker cam1(2, CAPTWIDTH, CAPTHEIGHT, CAPTFPS);

  // �|���S���̍쐬
  GLuint rect = rectangle();

  // �e�N�X�`���̍쐬
  GLuint tex = texture(TEXWIDTH, TEXHEIGHT);

  // �V�F�[�_
  GLuint shader = ggLoadShader("simple.vert", "simple.frag");
  GLuint texLoc = glGetUniformLocation(shader, "tex");
  GLuint ScreenCenterLoc = glGetUniformLocation(shader, "ScreenCenter");
  GLuint LensCenterLoc = glGetUniformLocation(shader, "LensCenter");
  GLuint TextureScaleLoc = glGetUniformLocation(shader, "TextureScale");
  GLuint TextureSizeLoc = glGetUniformLocation(shader, "TextureSize");

  // �E�B���h�E�̃T�C�Y�ύX���ɌĂяo�������̐ݒ�
  glfwSetWindowSizeCallback(resize);

  // �L�[�{�[�h���쎞�ɌĂяo�������̐ݒ�
  glfwSetKeyCallback(keyboard);

  // �V�F�[�_�̎g�p
  glUseProgram(shader);
  glUniform1i(texLoc, 0);

  // �e�N�X�`�����W�̃X�P�[��
  glUniform2f(TextureScaleLoc, 1.0, (GLfloat)DISPHEIGHT / (GLfloat)(DISPWIDTH / 2));

  // �e�N�X�`���̎g�p
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);

  // �`�悷��}�`�̎w��
  glBindVertexArray(rect);

  // �E�B���h�E���J���Ă���Ԃ���Ԃ��`�悷��
  while (glfwGetWindowParam(GLFW_OPENED) && glfwGetKey(GLFW_KEY_ESC) == GLFW_RELEASE)
  {
    // �`��
    cam0.subTexture();
    glUniform2f(ScreenCenterLoc, -0.5f, 0.0f);
    glUniform2f(LensCenterLoc,  0.1453f, 0.0f);
    glUniform2fv(TextureSizeLoc, 1, cam0.getScale());
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
    cam1.subTexture();
    glUniform2f(ScreenCenterLoc,  0.5f, 0.0f);
    glUniform2f(LensCenterLoc, -0.1453f, 0.0f);
    glUniform2fv(TextureSizeLoc, 1, cam1.getScale());
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // �_�u���o�b�t�@�����O
    glfwSwapBuffers();
  }

  return 0;
}
