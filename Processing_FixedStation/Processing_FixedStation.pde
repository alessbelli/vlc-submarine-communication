import processing.serial.*;

Serial myPort;
boolean firstContact = false;        // Whether we've heard from the microcontroller
byte[] b;
int blockSize = 64;
int blockCount;

void setup() {
  //  myPort.bufferUntil('\n');

  b = loadBytes("/home/alessbelli/Documents/smalltest.jpeg"); 
  blockCount = (b.length + blockSize -1) / blockSize;
  //byte block[];
  /*for (int i = 0; i < 2; i++) { //blockCount; i++) { 
   // Every tenth number, start a new line 
   while (!canCarryOn) {
   }
   
   if (i == blockCount - 1) {
   block = subset(b, i * blockSize, b.length % blockSize);
   } else {
   block = subset(b, i * blockSize, blockSize);
   }
   // bytes are from -128 to 127, this converts to 0 to 255 
   println(block);
   myPort.write(block);
   }
   */
    println(Serial.list());
    myPort = new Serial(this, Serial.list()[0], 115200);
}
void draw() {
}
void serialEvent(Serial myPort) {
  // get the ASCII string:
  byte[] inBuffer;
  int i = 0;
  int[] array;
  byte block[];
  int inByte = myPort.read();
  println("received: ", inByte);
  if (!firstContact) {
    if (inByte == 'A') {
      firstContact = true;
      println("firstcontact was made");

      println("writing ", byte(blockCount));
      myPort.clear();
      myPort.write(byte(blockCount));
    }
  } else {
    if (inByte <= blockCount) {
      if (inByte == blockCount - 1) {
        block = subset(b, inByte * blockSize, b.length % blockSize);
      } else {
        block = subset(b, inByte * blockSize, blockSize);
      }
      println("writing", block[0]);
      myPort.write(block);
    } else {
      println("arduino asked for an impossible block, restarting firstContact!");
      println("writing ", byte(blockCount));
      myPort.clear();
      myPort.write(byte(blockCount));
    }
  }
}
