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
#define TEXWIDTH 1024
#define TEXHEIGHT 512

// �g�p����e�N�X�`���̗̈�
#define REALWIDTH (CAPTHEIGHT * DISPWIDTH / DISPHEIGHT / 2)
#define REALHEIGHT CAPTHEIGHT
#define REALORIGINX ((CAPTWIDTH - REALWIDTH) / 2)
#define REALORIGINY 0

// �L���v�`���p�X���b�h
class CaptureWorker
{
  // �L���v�`��
  CvCapture *capture;

  // �e�N�X�`��
  GLenum format;
  GLsizei width, height;
  GLubyte *texture;

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
    if (capture == 0)
    {
      std::cerr << "cannot capture image" << std::endl;
      exit(1);
    }
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, static_cast<double>(width));
    cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, static_cast<double>(height));
    cvSetCaptureProperty(capture, CV_CAP_PROP_FPS, static_cast<double>(fps));

    // �e�N�X�`���p�̃��������m�ۂ���
    texture = new GLubyte[width * height * 4];

    // �p�����[�^�ɏ����l��^���Ă���
    this->width = width;
    this->height = height;
    this->format = GL_BGRA;

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
    cvReleaseCapture(&capture);

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

      if (cvGrabFrame(capture))
      {
        // �L���v�`���f������摜��؂�o��
        IplImage *image = cvRetrieveFrame(capture);

        if (image)
        {
          // �؂�o�����摜�̎�ނ̔���
          width = image->width;
          height = image->height;
          if (image->nChannels == 3)
            format = GL_BGR;
          else if (image->nChannels == 4)
            format = GL_BGRA;
          else
            format = GL_LUMINANCE;

          // �e�N�X�`���������ւ̓]��
          GLsizei size = REALWIDTH * image->nChannels;
          GLsizei offset = size * REALORIGINY + REALORIGINX * image->nChannels;
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

  // �e�N�X�`���]��
  void sendTexture(GLint x, GLint y)
  {
    lock();
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, 384, height, format, GL_UNSIGNED_BYTE, texture);
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

  // �e�N�X�`�����������m�ۂ���
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, 0);

  // �e�N�X�`���̓�����ݒ肷��
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
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

  // �w�i�F
  glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

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
    static bool blend = true;

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
  CaptureWorker cam0(0, CAPTWIDTH, CAPTHEIGHT, CAPTFPS);
  CaptureWorker cam1(1, CAPTWIDTH, CAPTHEIGHT, CAPTFPS);

  // �|���S���̍쐬
  GLuint rect = rectangle();

  // �e�N�X�`���̍쐬
  GLuint tex = texture(TEXWIDTH, TEXHEIGHT);

  // �V�F�[�_
  GLuint shader = ggLoadShader("simple.vert", "simple.frag");
  GLuint texLoc = glGetUniformLocation(shader, "tex");

  // �E�B���h�E�̃T�C�Y�ύX���ɌĂяo�������̐ݒ�
  glfwSetWindowSizeCallback(resize);

  // �L�[�{�[�h���쎞�ɌĂяo�������̐ݒ�
  glfwSetKeyCallback(keyboard);

  // �E�B���h�E���J���Ă���Ԃ���Ԃ��`�悷��
  while (glfwGetWindowParam(GLFW_OPENED) && glfwGetKey(GLFW_KEY_ESC) == GLFW_RELEASE)
  {
    // ��ʃN���A
    glClear(GL_COLOR_BUFFER_BIT);

    // �V�F�[�_�̎g�p
    glUseProgram(shader);
    glUniform1i(texLoc, 0);

    // �e�N�X�`���̎g�p
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);
    cam0.sendTexture(        0, 0);
    cam1.sendTexture(REALWIDTH, 0);

    // �`��
    glBindVertexArray(rect);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    // �_�u���o�b�t�@�����O
    glfwSwapBuffers();
  }

  return 0;
}