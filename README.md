# Person/Not Person
For Kendryte Standalone SDK. Tested on Sipeed Dan Dock.

Uses Header-only C library for Binary Neural Network (EBNN) to predict if an image is of a person.

## Dependencies
- [libjpeg](https://github.com/LuaDist/libjpeg) (9.1)
- [libgd](https://github.com/libgd/libgd) (2.2.5)
- [Kendryte Standalone SDK](https://github.com/kendryte/kendryte-standalone-sdk) (0.5.2)

Static libraries for libgd and libjpeg are include in `lib/`

To compile libjpeg from source for k210 create a makefile using makefile.ansi as a template.  Set the Kendryte toolchain and flags in their respective variables.

To compile libgd from source for k210 use the cmake build.  Edit CMakeLists.txt to use the Kendryte toolchain and set the correct flags.   

## Building
Replace `executable.cmake` in Kendryte Standalone SDK with the version included in this project. Modifications are added here to link the dependencies.


## Running
For sipeed m1, press *boot* key to capture picture and run inference.

For KD233, press GPIO key (near the blue rotary switch)


## Architecture
The architecture of our model is a small VGGnet

The layers definitions using the EBNN Chainer classes are:  
```python
self.conv1 = BL.ConvBNBST(n_in, 32, 3, pad=1, stride=1)
self.conv2 = BL.BinaryConvPoolBNBST(32, 32, 3, stride=1, pksize=3, pstride=2)
self.conv3 = BL.BinaryConvBNBST(32, 32, 3, stride=1)
self.fc1 = BL.BinaryLinearBNSoftmax(None, n_out)
```

Except for the first and last layer, the input and output size of each layer is 32.  
The kernel size for each layer is (3, 3).  
The stride for each layer is (1, 1).  
The kernel for pooling is also (3, 3), and the pooling stride is (2, 2).  

The Chainer `__call__` for the model is:  
```python
h = self.conv1(x)  
h = self.conv2(h)  
h = F.dropout(h, ratio=0.25)  
h = self.conv3(h)  
h = F.dropout(h, ratio=0.4)  
h = self.fc1(h)  

loss = F.softmax_cross_entropy(h, t)
accuracy = F.accuracy(h, t)

report = {
  'loss': loss,
  'acc': accuracy
}
```

## Training
Train using the 3 labels: person, not person, noise.

## Data
Used portrait style pictures of people as well as only faces for “person” and empty indoor spaces, walls, and doors for “not person”.

Images were pre-processed to increase the amount of training data.

1. Resize each original image to a square
2. Each original image had 5 crops taken
  * Each corner and center
3. Resize each processed crop to 32x32
4. Save a Color, and a grayscale version for each crop
5. Apply some effects to each crop for both color, and grayscale versions
  * None
  * Gaussian blur
  * Salt and pepper
  * Distorted the image so much it is not recognizable
