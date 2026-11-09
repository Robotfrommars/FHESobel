#include <helib/helib.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
using namespace std;
using namespace helib;
#include <opencv2/opencv.hpp>
//Useful functions
//taken from demo code
using Clock = chrono::steady_clock;
double msSince(Clock::time_point t0)
{
  return chrono::duration<double, milli>(Clock::now() - t0).count();
}
//client side code
int clientSide(const string& filename, long m, long p, long bits)
{
//Use opencv to get the image in greyscale
    cv::Mat img = cv::imread(filename, cv::IMREAD_GRAYSCALE);  // loads directly as grayscale
    if (img.empty()) {
        std::cerr << "Could not open image: " << filename << "\n";
        return 1;
    }
//get pixel width of image for helib usage
    int width=img.cols;
    int height=img.rows;

    //debug cout<<width;
//set up context
    Context ctx = ContextBuilder<BGV>().m(m).p(p).r(1).bits(bits).c(2).build();
    long nslots = ctx.getEA().size();
    std::cout << "slots: " << nslots << "\n";

  //convert image to a usable scale for Helib
    // int side = (int)std::floor(std::sqrt((double)nslots));
    // if (img.rows * img.cols > nslots)
    // cv::resize(img, img, cv::Size(side, side), 0, 0, cv::INTER_AREA);

  // Keys
    helib::SecKey sk(ctx);
    sk.GenSecKey();
    const helib::PubKey& pk = sk;

  // Get ready to encrypt and send over to server
    helib::Ptxt<helib::BGV> ptxt(ctx);   
    for (int r = 0; r < img.rows; ++r)
        for (int c = 0; c < img.cols; ++c)
            ptxt[r * img.cols + c] = img.at<uchar>(r, c);

  // Encrypt data
    auto t0 = Clock::now();
    helib::Ctxt ctxt(pk);
    pk.Encrypt(ctxt, ptxt);
    double tEnc = msSince(t0);

  
//call server function with encrypted data and width as variable
    auto t1 = Clock::now();
//await server
    double tServ = msSince(t1);
//print wait time
//convert back to an image and save image
    auto t2 = Clock::now();
    helib::Ptxt<helib::BGV> out(ctx);
    sk.Decrypt(out, ctxt);
    double tDec = msSince(t2);
    cv::Mat rec(img.rows, img.cols, CV_8UC1);   
    for (int r = 0; r < img.rows; ++r) {
        for (int c = 0; c < img.cols; ++c) {
            NTL::ZZX poly = out[r * img.cols + c].getData();   // PolyMod -> ZZX
            long v = NTL::conv<long>(NTL::coeff(poly, 0));     // constant term = the pixel
            rec.at<uchar>(r, c) = static_cast<uchar>(v);
        }
    }
    if (!cv::imwrite("output.jpg", rec)) {
        std::cerr << "Error: Failed to save the image." << std::endl;
        return -1;
    }
//clean-up as needed
    cout << "Time to encrypt: "<<tEnc<<endl<<"Time for server: "<<tServ<<endl<<"Time to decrypt: "<<tDec<<endl;
    return 1;
}
int main(int argc, char* argv[])
{
  //call client side code
  if (argc != 2) {
       cerr << "usage: " << argv[0] << " <image path>\n";
       return 1;
    }
    string filename=argv[1];
    return clientSide(filename,32768,65537,500);
}
