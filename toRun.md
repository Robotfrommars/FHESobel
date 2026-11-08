g++ -std=c++17 main.cpp -o main \
  -I$HOME/helib_install/include \
  -L$HOME/helib_install/lib \
  -Wl,-rpath,$HOME/helib_install/lib \
  $(pkg-config --cflags --libs opencv4) \
  -lhelib -lntl -lgmp -lpthread
