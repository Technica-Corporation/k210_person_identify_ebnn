#include "rgb2jpg.h"
#include "ff.h"
#include "jpeglib.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "gd.h"
#include "person_EX2.h"


int personNotPerson(uint8_t *buf, int width, int height) {
  JSAMPLE* image_buffer;	/* Points to large array of R,G,B-order data */
  JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
  int row_stride;		/* physical row width in image buffer */
  unsigned long jpg_size, out_size;
  int pixel_size = 3;
  uint8_t* out_buffer;
  struct jpeg_compress_struct cinfo;
  struct jpeg_error_mgr jerr;

  jpg_size = (unsigned long) width * height * pixel_size;
  row_stride = width * pixel_size;	/* JSAMPLEs per row in image_buffer */

  out_buffer = (uint8_t *) malloc(jpg_size);
  image_buffer = (JSAMPLE *) malloc(jpg_size);

  /* RGB565 to RGB888 conversion */
  uint16_t *ptr;
  uint32_t i, j;
  uint8_t red, green, blue;

  ptr = (uint16_t*)buf;
  j = 0;
  for (i = 0; i < width * height; i++) {
    red = (ptr[i] & 0xf800) >> 11;
    green = (ptr[i] & 0x07e0) >> 5;
    blue = ptr[i] & 0x001f;

    red = 255/31 * red;
    green = 255/63 * green;
    blue = 255/31 * blue;

    image_buffer[j] = red;
    image_buffer[j+1] = green;
    image_buffer[j+2] = blue;
    j += 3;
  }






  cinfo.err = jpeg_std_error(&jerr);

  jpeg_create_compress(&cinfo);
  jpeg_mem_dest(&cinfo, &out_buffer, &out_size);


  cinfo.image_width = width; 	/* image width and height, in pixels */
  cinfo.image_height = height;
  cinfo.input_components = pixel_size;		/* # of color components per pixel */
  cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */

  jpeg_set_defaults(&cinfo);
  jpeg_start_compress(&cinfo, TRUE);

  while (cinfo.next_scanline < cinfo.image_height) {
    row_pointer[0] = & image_buffer[cinfo.next_scanline * row_stride];
    (void) jpeg_write_scanlines(&cinfo, row_pointer, 1);
  }

  jpeg_finish_compress(&cinfo);
  jpeg_destroy_compress(&cinfo);

  /* Resize image using libgd */
  void* gd_buffer;
  gdImagePtr in, out;
  int resize_width=28, resize_height=28;
  void* tempOut;
  int frame_size = 0;

  in = gdImageCreateFromJpegPtr(jpg_size, out_buffer);
  gdImageSetInterpolationMethod(in, GD_BILINEAR_FIXED);
  out = gdImageScale(in, resize_width, resize_height);
  gd_buffer = gdImageJpegPtr(out, &frame_size, 90);

  gdImageDestroy(in);
  gdImageDestroy(out);

  /* extract RGB values of resized image */

  struct jpeg_decompress_struct dinfo;
  struct jpeg_error_mgr djerr;

  dinfo.err = jpeg_std_error(&djerr);

  jpeg_create_decompress(&dinfo);
  jpeg_mem_src(&dinfo, gd_buffer, (unsigned long)frame_size );

  int rc = jpeg_read_header(&dinfo, TRUE);
  if (rc != 1) {
      printf("File does not seem to be a normal JPEG");
      return 0;
  }

  jpeg_start_decompress(&dinfo);


  row_stride = resize_width * pixel_size;

  while (dinfo.output_scanline < dinfo.output_height) {
    unsigned char *buffer_array[1];
    buffer_array[0] = image_buffer + (dinfo.output_scanline) * row_stride;

    jpeg_read_scanlines(&dinfo, buffer_array, 1);
  }

  jpeg_finish_decompress(&dinfo);
  jpeg_destroy_decompress(&dinfo);


  /* Format RGB values and run through inference */
  static float csv_buffer[2352];  //Needs to be static for ebnn
  int totalValues = (resize_width*resize_height*pixel_size);
  int x;
  int count = 1;
  uint8_t output[1];


  //r
  csv_buffer[0] = (float)image_buffer[0];
  for(x = 3; x<totalValues; x+=3){
    csv_buffer[count] = (float)image_buffer[x] /255.0;
    count++;
  }

  //g
  for(x = 0; x<totalValues; x+=3){
    csv_buffer[count] = (float)image_buffer[x+1] /255.0;
    count++;
  }

  //b
  for(x = 0; x<totalValues; x+=3){
    csv_buffer[count] = (float)image_buffer[x+2] /255.0;
    count++;
  }


  free(gd_buffer);
  free(out_buffer);
  free(image_buffer);


  ebnn_compute(&csv_buffer[0], output);

  return output[0];
}
