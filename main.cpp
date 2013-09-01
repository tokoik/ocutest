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

// 補助プログラム
#include "gg.h"
using namespace gg;

// シェーダ
#include "GgPointShader.h"

// ディスプレイのサイズ
#define DISPWIDTH 1280
#define DISPHEIGHT 800

// キャプチャする画像サイズ
#define CAPTWIDTH 640
#define CAPTHEIGHT 480

// キャプチャするフレームレート
#define CAPTFPS 30

// テクスチャサイズ
#define TEXWIDTH 512
#define TEXHEIGHT 512

// 使用するテクスチャの領域
#define IMGASPECT 1 // (DISPWIDTH / DISPHEIGHT / 2)
#define IMGWIDTH (CAPTHEIGHT * IMGASPECT)
#define IMGHEIGHT CAPTHEIGHT
#define IMGORIGINX ((CAPTWIDTH - IMGWIDTH) / 2)
#define IMGORIGINY 0

// キャプチャ用スレッド
class CaptureWorker
{
  // キャプチャ
  CvCapture *capture;

  // テクスチャ
  GLenum format;
  GLsizei width, height;
  GLubyte *texture;
  GLfloat scale[2];

  // 実行状態
  bool status;

  // スレッドとミューテックス
#ifdef _WIN32
  HANDLE thread;
  HANDLE mutex;
#else
  pthread_t thread;
  pthread_mutex_t mutex;
#endif

public:

  // コンストラクタ
  CaptureWorker(int index, int width, int height, int fps)
  {
    // カメラを初期化する
    capture = cvCreateCameraCapture(index);
    if (capture)
    {
      cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_WIDTH, static_cast<double>(width));
      cvSetCaptureProperty(capture, CV_CAP_PROP_FRAME_HEIGHT, static_cast<double>(height));
      cvSetCaptureProperty(capture, CV_CAP_PROP_FPS, static_cast<double>(fps));
    }
    else
      std::cerr << "Cannot capture from camera " << index << std::endl;

    // テクスチャ用のメモリを確保する
    texture = new GLubyte[width * height * 4];

    // パラメータに初期値を与えておく
    this->width = width;
    this->height = height;
    this->format = GL_BGRA;
    this->scale[0] = 0.5f * static_cast<GLfloat>(width) / static_cast<GLfloat>(TEXWIDTH);
    this->scale[1] = -0.5f * static_cast<GLfloat>(height) / static_cast<GLfloat>(TEXHEIGHT);

    // スレッドとミューテックスを生成する
#ifdef _WIN32
    mutex = CreateMutex(NULL, TRUE, NULL);
    thread = CreateThread(NULL, 0, start, (LPVOID)this, 0, NULL);
#else
    pthread_mutex_init(&mutex, 0);
    pthread_create(&thread, 0, start, this);
#endif

    // スレッドが実行状態であることを記録する
    status = true;
  }

  // デストラクタ
  ~CaptureWorker()
  {
    // スレッドを停止する
    stop();

#ifdef _WIN32
    // スレッドの停止を待つ
    WaitForSingleObject(thread, 0); 
    CloseHandle(thread);

    // ミューテックスを破棄する
    CloseHandle(mutex);
#else
    // スレッドの停止を待つ
    pthread_join(thread, 0);

    // ミューテックスを破棄する
    pthread_mutex_destroy(&mutex);
#endif

    // image の release
    if (capture) cvReleaseCapture(&capture);

    // メモリの解放
    delete[] texture;
  }

  // mutex のロック
  void lock(void)
  {
#ifdef _WIN32
    WaitForSingleObject(mutex, 0); 
#else
    pthread_mutex_lock(&mutex);
#endif
  }

  // mutex のリリース
  void unlock(void)
  {
#ifdef _WIN32
    ReleaseMutex(mutex);
#else
    pthread_mutex_unlock(&mutex);
#endif
  }

  // スレッドの開始
#ifdef _WIN32
  static DWORD WINAPI start(LPVOID arg)
#else
  static void *start(void *arg)
#endif
  {
    return reinterpret_cast<CaptureWorker *>(arg)->getTexture();
  }

  // スレッドの停止
  void stop(void)
  {
    lock();
    status = false;
    unlock();
  }

  // スレッドの停止判定
  bool check(void)
  {
    bool ret;

    lock();
    ret = status;
    unlock();

    return ret;
  }

  // テクスチャ作成
#ifdef _WIN32
  DWORD WINAPI getTexture(void)
#else
  void *getTexture(void)
#endif
  {
    for (;;)
    {
      // 終了条件のテスト
      if (!check()) break;

      if (capture && cvGrabFrame(capture))
      {
        // キャプチャ映像から画像を切り出す
        IplImage *image = cvRetrieveFrame(capture);

        if (image)
        {
          // 切り出した画像のサイズとテクスチャ座標のスケール
          height = image->height;
          width = height * IMGASPECT;
          if (width > image->width) width = image->width;
          scale[0] = 0.5f * static_cast<GLfloat>(width) / static_cast<GLfloat>(TEXWIDTH);
          scale[1] = -0.5f * static_cast<GLfloat>(height) / static_cast<GLfloat>(TEXHEIGHT);

          // 切り出した画像の種類の判別
          if (image->nChannels == 3)
            format = GL_BGR;
          else if (image->nChannels == 4)
            format = GL_BGRA;
          else
            format = GL_RED;

          // テクスチャメモリへの転送
          GLsizei size = IMGWIDTH * image->nChannels;
          GLsizei offset = size * IMGORIGINY + IMGORIGINX * image->nChannels;
          lock();
          for (int y = 0; y < image->height; ++y)
            memcpy(texture + size * y, image->imageData + image->widthStep * y + offset, size);
          unlock();
        }
        else
        {
          // １フレーム分待つ
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

  // テクスチャのスケール
  const GLfloat *getScale(void)
  {
    return scale;
  }

  // テクスチャ転送
  void subTexture(void)
  {
    lock();
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, texture);
    unlock();
  }
};

//
// 矩形ポリゴンの作成
//
static GLuint rectangle(void)
{
  // 頂点配列オブジェクトを作成して結合する
  GLuint vao;
  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  // 頂点バッファオブジェクトを作成して結合する
  GLuint vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);

  // バッファオブジェクトを確保する
  static const GLfloat p[] =
  {
    -1.0f, -1.0f,
    1.0f, -1.0f,
    1.0f,  1.0f,
    -1.0f,  1.0f
  };
  glBufferData(GL_ARRAY_BUFFER, sizeof p, p, GL_STATIC_DRAW);

  // 頂点位置は index == 0 の in 変数から得る
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
  glEnableVertexAttribArray(0);

  return vao;
}

//
// テクスチャの作成
//
static GLuint texture(GLsizei width, GLsizei height)
{
  // テクスチャを作成して結合する
  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  // 全体が黒色の画像を準備する
  GLubyte *texture = new GLubyte[width * height * 4];
  memset(texture, 0, width * height * 4);

  // テクスチャメモリを確保する
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, texture);

  // 画像に使用したメモリを開放する
  delete[] texture;

  // テクスチャの特性を設定する
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  return tex;
}

//
// 初期設定
//
static int init(const char *title)
{
  // GLFW を初期化する
  if (glfwInit() == GL_FALSE)
  {
    // 初期化に失敗した
    std::cerr << "Error: Failed to initialize GLFW." << std::endl;
    return 1;
  }

  // OpenGL Version 3.2 Core Profile を選択する
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MAJOR, 3);
  glfwOpenWindowHint(GLFW_OPENGL_VERSION_MINOR, 2);
  glfwOpenWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  // ウィンドウを開く
  if (glfwOpenWindow(DISPWIDTH, DISPHEIGHT, 8, 8, 8, 8, 24, 0, GLFW_WINDOW) == GL_FALSE)
  {
    // ウィンドウが開けなかった
    std::cerr << "Error: Failed to open GLFW window." << std::endl;
    return 1;
  }

  // 開いたウィンドウに対する設定
  glfwSwapInterval(1);
  glfwSetWindowTitle(title);

  // 補助プログラムによる初期化
  ggInit();

  // 隠面消去の設定
  glDisable(GL_DEPTH_TEST);
  glDisable(GL_CULL_FACE);

  // 初期化成功
  return 0;
}

//
// ウィンドウのサイズ変更時の処理
//
static void GLFWCALL resize(int w, int h)
{
  // ウィンドウ全体をビューポートにする
  glViewport(0, 0, w, h);
}

//
// キーボード
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
// メインプログラム
//
int main(int argc, const char * argv[])
{
  // 初期設定
  if (init("Oculus Test")) return 1;

  // キャプチャ用スレッドを生成する
  CaptureWorker cam0(1, CAPTWIDTH, CAPTHEIGHT, CAPTFPS);
  CaptureWorker cam1(2, CAPTWIDTH, CAPTHEIGHT, CAPTFPS);

  // ポリゴンの作成
  GLuint rect = rectangle();

  // テクスチャの作成
  GLuint tex = texture(TEXWIDTH, TEXHEIGHT);

  // シェーダ
  GLuint shader = ggLoadShader("simple.vert", "simple.frag");
  GLuint texLoc = glGetUniformLocation(shader, "tex");
  GLuint ScreenCenterLoc = glGetUniformLocation(shader, "ScreenCenter");
  GLuint LensCenterLoc = glGetUniformLocation(shader, "LensCenter");
  GLuint TextureScaleLoc = glGetUniformLocation(shader, "TextureScale");
  GLuint TextureSizeLoc = glGetUniformLocation(shader, "TextureSize");

  // ウィンドウのサイズ変更時に呼び出す処理の設定
  glfwSetWindowSizeCallback(resize);

  // キーボード操作時に呼び出す処理の設定
  glfwSetKeyCallback(keyboard);

  // シェーダの使用
  glUseProgram(shader);
  glUniform1i(texLoc, 0);

  // テクスチャ座標のスケール
  glUniform2f(TextureScaleLoc, 1.0, (GLfloat)DISPHEIGHT / (GLfloat)(DISPWIDTH / 2));

  // テクスチャの使用
  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, tex);

  // 描画する図形の指定
  glBindVertexArray(rect);

  // ウィンドウが開いている間くり返し描画する
  while (glfwGetWindowParam(GLFW_OPENED) && glfwGetKey(GLFW_KEY_ESC) == GLFW_RELEASE)
  {
    // 描画
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

    // ダブルバッファリング
    glfwSwapBuffers();
  }

  return 0;
}
