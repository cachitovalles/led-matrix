#include "led-matrix.h"

#include "pixel-mapper.h"
#include "graphics.h"

#include <assert.h>
#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <algorithm>

using std::min;
using std::max;

#define TERM_ERR  "\033[1;31m"
#define TERM_NORM "\033[0m"

using namespace rgb_matrix;

volatile bool interrupt_received = false;
static void InterruptHandler(int signo) {
  interrupt_received = true;
}

//Automata

class Life {
protected:
  Life(Canvas *canvas) : canvas_(canvas) {}
  inline Canvas *canvas() { return canvas_; }

public:
  virtual ~Life() {}
  virtual void Run() = 0;

private:
  Canvas *const canvas_;
};

class GameLife : public Life {
public:
  GameLife(Canvas *m, int delay_ms=1000, bool torus=true)
    : Life(m), delay_ms_(delay_ms), torus_(torus) {
    width_ = canvas()->width();
    height_ = canvas()->height();

    // Allocate memory
    GameState_ = new int*[width_];
    for (int x=0; x<width_; ++x) {
      GameState_[x] = new int[height_];
    }
    newGameState_ = new int*[width_];
    for (int x=0; x<width_; ++x) {
      newGameState_[x] = new int[height_];
    }

    // Init GameState randomly
    srand(time(NULL));
    for (int x=0; x<width_; ++x) {
      for (int y=0; y<height_; ++y) {
        GameState_[x][y]=rand()%2;
      }
    }
    r_ = rand()%255;
    g_ = rand()%255;
    b_ = rand()%255;

    if (r_<150 && g_<150 && b_<150) {
      int c = rand()%3;
      switch (c) {
      case 0:
        r_ = 200;
        break;
      case 1:
        g_ = 200;
        break;
      case 2:
        b_ = 200;
        break;
      }
    }
  }

  ~GameLife() {
    for (int x=0; x<width_; ++x) {
      delete [] GameState_[x];
    }
    delete [] GameState_;
    for (int x=0; x<width_; ++x) {
      delete [] newGameState_[x];
    }
    delete [] newGameState_;
  }

  void Run() override {
    while (!interrupt_received) {

      updateValues();

      for (int x=0; x<width_; ++x) {
        for (int y=0; y<height_; ++y) {
          if (GameState_[x][y])
            canvas()->SetPixel(x, y, r_, g_, b_);
          else
            canvas()->SetPixel(x, y, 0, 0, 0);
        }
      }
      usleep(delay_ms_ * 1000); // ms
    }
  }


private:
  int numAliveNeighbours(int x, int y) {
    int num=0;
    if (torus_) {
      // Edges are connected (torus)
      num += GameState_[(x-1+width_)%width_][(y-1+height_)%height_];
      num += GameState_[(x-1+width_)%width_][y                    ];
      num += GameState_[(x-1+width_)%width_][(y+1        )%height_];
      num += GameState_[(x+1       )%width_][(y-1+height_)%height_];
      num += GameState_[(x+1       )%width_][y                    ];
      num += GameState_[(x+1       )%width_][(y+1        )%height_];
      num += GameState_[x                  ][(y-1+height_)%height_];
      num += GameState_[x                  ][(y+1        )%height_];
    }
    else {
      // Edges are not connected (no torus)
      if (x>0) {
        if (y>0)
          num += GameState_[x-1][y-1];
        if (y<height_-1)
          num += GameState_[x-1][y+1];
        num += GameState_[x-1][y];
      }
      if (x<width_-1) {
        if (y>0)
          num += GameState_[x+1][y-1];
        if (y<31)
          num += GameState_[x+1][y+1];
        num += GameState_[x+1][y];
      }
      if (y>0)
        num += GameState_[x][y-1];
      if (y<height_-1)
        num += GameState_[x][y+1];
    }
    return num;
  }

  void updateValues() {
        int count = count + 1; 
    // Copy GameState to newGameState
    for (int x=0; x<width_; ++x) {
      for (int y=0; y<height_; ++y) {
        newGameState_[x][y] = GameState_[x][y];
      }
    }
    // update newGameState based on GameState
    for (int x=0; x<width_; ++x) {
      for (int y=0; y<height_; ++y) {
        int num = numAliveNeighbours(x,y);
        if (GameState_[x][y]) {
          // cell is alive
          if (num < 2 || num > 3)
            newGameState_[x][y] = 0;
        }
        else{
          // cell is dead
          if (num == 3)
            newGameState_[x][y] = 1;
        }
        if (count == 10){
            newGameState_[x/2+21][y/2+21] = 1;
            newGameState_[x/2+22][y/2+22] = 1;
            newGameState_[x/2+22][y/2+23] = 1;
            newGameState_[x/2+21][y/2+23] = 1;
            newGameState_[x/2+20][y/2+23] = 1;
        }
        
      }
      Console.Write(count);
    }
    // copy newGameState to GameState
    for (int x=0; x<width_; ++x) {
      for (int y=0; y<height_; ++y) {
        GameState_[x][y] = newGameState_[x][y];
      }
    }
  }


  int** GameState_;
  int** newGameState_;
  int delay_ms_;
  int r_;
  int g_;
  int b_;
  int width_;
  int height_;
  bool torus_;
};

//Opciones
int main(int argc, char *argv[]) {

  int scroll_ms = 30;

  RGBMatrix::Options matrix_options;
  rgb_matrix::RuntimeOptions runtime_opt;

  // These are the defaults when no command-line flags are given.
  matrix_options.rows = 16;
  matrix_options.chain_length = 2;
  matrix_options.parallel = 1;

  // First things first: extract the command line flags that contain
  // relevant matrix options.
  //if (!ParseOptionsFromFlags(&argc, &argv, &matrix_options, &runtime_opt)) {
    //return usage(argv[0]);
  //}

  int opt;
  while ((opt = getopt(argc, argv, "dD:r:P:c:p:b:m:LR:")) != -1) {
    switch (opt) {

    case 'm':
      scroll_ms = atoi(optarg);
      break;

    default: /* '?' */
      return 0;
    }
  }

  RGBMatrix *matrix = RGBMatrix::CreateFromOptions(matrix_options, runtime_opt);
  if (matrix == NULL)
    return 1;

  printf("Size: %dx%d. Hardware gpio mapping: %s\n",
         matrix->width(), matrix->height(), matrix_options.hardware_mapping);

  Canvas *canvas = matrix;

  // The **DemoRunner->cambia a Life** objects are filling
  // the matrix continuously.
  Life *life = new GameLife(canvas, scroll_ms);
  

  // Set up an interrupt handler to be able to stop animations while they go
  // on. Each demo tests for while (!interrupt_received) {},
  // so they exit as soon as they get a signal.
  signal(SIGTERM, InterruptHandler);
  signal(SIGINT, InterruptHandler);

  printf("Press <CTRL-C> to exit and reset LEDs\n");

  // Now, run our particular demo; it will exit when it sees interrupt_received.
  life->Run();

  delete life;
  delete canvas;

  printf("Received CTRL-C. Exiting.\n");
  return 0;
}
