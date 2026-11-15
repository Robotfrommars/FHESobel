#include "net.h"
#include <opencv2/opencv.hpp>
#include <cmath>
#include <iostream>
using namespace helib;
using namespace std;
using namespace cv;
void encryptRow(const PubKey& pk, const Context& ctx, const cv::Mat& img,int y, long nslots, Ctxt& out) {
  //default row of 255 so we can do top and bottom rows
  vector<long> vals(nslots, 255);
  //check if top or bottom row, 
  if (y >= 0 && y < img.rows)
    //go through rows and update vector with actual values
    for (int x = 0; x < img.cols; ++x) 
      vals[x] = img.at<uchar>(y, x);
  //create plaintext of row
  Ptxt<BGV> pt(ctx, vals);
  //encrypt the row and store in out
  pk.Encrypt(out, pt);
}
//helper function taken from demo for time
using Clock = chrono::steady_clock;
double msSince(Clock::time_point t0)
{
return chrono::duration<double, milli>(Clock::now() - t0).count();
}
int main(int argc, char* argv[]) {
  //read in image
  auto t = Clock::now(); //used for total runtime
  cv::Mat img;
  double tEnc=0, tDec=0,tServ=0;
  int m,p,bits;
  if(argc < 2) //default to mandrill image
  {img = cv::imread("mandril_256x256.jpg", cv::IMREAD_GRAYSCALE);
  if (img.empty()) { std::cerr << "load failed\n"; return 1; }
    m=16384;
    p=65537;
    bits=200;
  }
  else if(argc == 2){ //file given
    img = cv::imread(argv[1], cv::IMREAD_GRAYSCALE);
    if (img.empty()) { std::cerr << "load failed\n"; return 1; }
    m=16384;
    p=65537;
    bits=200;
  }
  else{
    cerr<<"Invalid call";
    return 1;
  }
  //setup context p>256*256
  Context context = ContextBuilder<BGV>().m(m).p(p).r(2).bits(bits).c(2).build();
  //nslots for usage (Lots of issues with this when try one ciphertext for the entire image)
  long nslots = context.getEA().size();
  //create private key
  SecKey sk(context);
  sk.GenSecKey();
  //create public key
  addSome1DMatrices(sk);                 
  const PubKey& pk = sk;

  // Connect
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = htons(5555);
  if (connect(fd, (sockaddr*)&addr, sizeof addr) < 0) {
    cerr << "connect failed (is the server running?)\n";
    return 1;
  }

  // Send context so server can initialize the same context
  stringstream ss1; context.writeTo(ss1); sendBlob(fd, ss1.str()); 
  //send public key for server usage
  stringstream ss2; pk.writeTo(ss2);      sendBlob(fd, ss2.str()); 
  uint64_t numMsgs = img.rows + 2;
  writeAll(fd, &numMsgs, sizeof numMsgs);

  // 2. Stream rows, read results as they come back
  cv::Mat mag(img.rows, img.cols, CV_8UC1);
  Ctxt row(pk);
  //send blank row of zeroes for row above image
  auto t0 = Clock::now();
  encryptRow(pk, context, img, -1, nslots, row);
  tEnc += msSince(t0);
  sendCtxt(fd, row);
  //send row i=0
  t0 = Clock::now();
  encryptRow(pk, context, img,  0, nslots, row); 
  tEnc += msSince(t0);
  sendCtxt(fd, row);

  for (int y = 0; y < img.rows; ++y) {
    //encrypt row below and send
    auto t0 = Clock::now();
    encryptRow(pk, context, img, y + 1, nslots, row);
    tEnc += msSince(t0);

    //send row to server
    auto t1 = Clock::now();
    sendCtxt(fd, row);

    //get response back from server for row i/y
    Ctxt res = recvCtxt(fd, pk);
    tServ += msSince(t1); //time since sending context to server (increment to total over the entire runtime)
    //mimic how the server sent data the data, not the best way of doing this
    res.writeTo(ss1);
  
    
    //setup a plaintext to store the decrypted text
    Ptxt<BGV> out(context);
    //decrypt plain and simple
    auto t2 = Clock::now();
  
    sk.Decrypt(out, res);
    tDec += msSince(t2); //increment total decrypt time since each row is handled separately


    //convert row back into image and store row in output image mag
    for (int x = 0; x < img.cols; ++x) {
      long g2 = NTL::conv<long>(NTL::coeff(out[x].getData(), 0));
      mag.at<uchar>(y, x) = (uchar)std::min(255.0, std::sqrt((double)g2));
    }
    if(y%15==0) //less output to console 255%15=0
    std::cout << "row " << y << " done\n";
  }
  
  cv::imwrite("sobel_encrypted.jpg", mag);
  close(fd);
  double tTot = msSince(t);
  cout<< "Total runtime: "<<tTot<<endl;
  cout<< "Server runtime: "<<tServ<<endl;
  cout<< "Encryption runtime: "<<tEnc<<endl;
  cout<< "Decrytion runtime: "<<tDec<<endl;
  cout << "Sent "<< bytesSent << " bytes to the Server.\n";
  cout << "Received "<< bytesRecv << " bytes from the Server.\n";
}
