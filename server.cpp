#include "net.h"
#include <iostream>
using namespace helib;
using namespace std;

//run Sobel
Ctxt sobelRow(const EncryptedArray& ea, const Ctxt& a, const Ctxt& b, const Ctxt& c) {
  //           [ 1  2  1]       [-1 0 1]
  //Sobel gy=  [ 0  0  0]  gx = [-2 0 2]
  //           [-1 -2 -1]       [-1 0 1]
  Ctxt v = a;  v += b;  v += b;  v += c;
  //difference between top and bottom row
  Ctxt d = c;  d -= a;
  Ctxt vL = v, vR = v;
  ea.rotate(vL, 1);  ea.rotate(vR, -1);
  //gx calculated as shown above
  Ctxt gx = vR;  gx -= vL;
  Ctxt dL = d, dR = d;
  ea.rotate(dL, 1);  ea.rotate(dR, -1);
  //gy calculated
  //d has to be added twice to match sobel, didnt have to be done
  Ctxt gy = dL;  gy += d;  gy += d;  gy += dR;
  //square both of these contexts and add
  gx.multiplyBy(gx);
  gy.multiplyBy(gy);
  gx += gy;
  return gx;
}

int main() {
  //setup server
  int lfd = socket(AF_INET, SOCK_STREAM, 0);
  int yes = 1;
  setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes);
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  addr.sin_port = htons(5555);
  bind(lfd, (sockaddr*)&addr, sizeof addr);
  listen(lfd, 1);
  cout << "server waiting on 127.0.0.1:5555\n";
  //accept client connection
  int fd = accept(lfd, nullptr, nullptr);

  
  //get context
  stringstream cs(recvBlob(fd));
  //use readfrom to read context properly
  Context context = Context::readFrom(cs);
  //get public key
  stringstream ks(recvBlob(fd));
  //use the readFrom for keys
  PubKey pk = PubKey::readFrom(ks, context);
  //get the number of rows that will be sent in total
  uint64_t numMsgs;
  readAll(fd, &numMsgs, sizeof numMsgs);
  const EncryptedArray& ea = context.getEA();
  cout << "setup received, expecting " << numMsgs << " rows\n";
  //uint64_t bytesSent=0; debug code, removed
  // 1st two rows
  Ctxt a = recvCtxt(fd, pk);
  Ctxt b = recvCtxt(fd, pk);
  //go through the rest of the rows
  for (uint64_t i = 2; i < numMsgs; ++i) {
    //get bottom row
    Ctxt c = recvCtxt(fd, pk);
    //run sobel
    Ctxt res = sobelRow(ea, a, b, c);
    uint64_t x = sendCtxt(fd, res); //x does not need to be used
    //slide window down one row
    a = b;  b = c;
    cout << "processed row " << (i - 2) << "\n" << "Sent " << x << " bytes \n"; 
  }
  //cleanup
  //close client connection
  close(fd);
  //close server 
  close(lfd);
  //output these should be equivelant towhat client outputs
  cout << "Sent "<< bytesSent << " bytes to the Client.\n";
  cout << "Received "<< bytesRecv << " bytes from the Client.\n";
}
