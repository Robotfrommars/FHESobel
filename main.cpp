#include <helib/helib.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>


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
    
//get pixel width of image

//convert image to a usable scale for Helib

//set up context
    Context ctx = ContextBuilder<BGV>().m(m).p(p).r(1).bits(bits).c(2).build();
    long n = ctx.getEA().size();
//call server function with encrypted data and width as variable
//await server
//print wait time
//convert back to an image and save image

//clean-up as needed
}
int main(int argc, char* argv[])
{
  //call client side code
  if(argc == 1)
    string filename =argv[0];
    cerr << "usage: " << argv[0] << " Should be a file Path\n";
}