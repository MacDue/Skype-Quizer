#ifndef V4L2_LINUX_H_
#define V4L2_LINUX_H_

#include <vector>
#include <exception>
#include <algorithm>

#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <CImg.h>
#include <formatxx/std_string.h>

using namespace cimg_library;

class v4l2_cimg final {
  int fd = -1;
  int width, height;
  std::vector<uint8_t> frame_buffer;

  public:
    v4l2_cimg(char const * webcam, int width, int height)
      : width{width}, height{height}
    {
      if ((fd = open(webcam, O_WRONLY | O_SYNC)) == -1) {
        throw std::runtime_error("Unable to open video output!");
      }

      struct v4l2_format vid_format;
      vid_format.type = V4L2_BUF_TYPE_VIDEO_OUTPUT;
      vid_format.fmt.pix.pixelformat  = V4L2_PIX_FMT_YUYV;
      vid_format.fmt.pix.width        = width;
      vid_format.fmt.pix.height       = height;
      vid_format.fmt.pix.field        = V4L2_FIELD_NONE;
      vid_format.fmt.pix.bytesperline = width * 2;
      vid_format.fmt.pix.sizeimage    = width * height * 2;
      vid_format.fmt.pix.colorspace   = V4L2_COLORSPACE_JPEG;
      if (ioctl(fd, VIDIOC_S_FMT, &vid_format) == -1) {
        throw std::runtime_error(
            formatxx::format_string("Unable to set video format! Errno: {}", errno));
      }

      frame_buffer.resize(vid_format.fmt.pix.sizeimage);
    }

    template <typename T>
    void display(CImg<T> const & canvas) {
      if (
           canvas.width() != width
        || canvas.height() != height
        || canvas.spectrum() != 3
      ) {
        throw std::runtime_error("Canvas must match v4l2 resolution!");
      }

      // Write YUYV frame data
      bool skip = true;
      cimg_forXY(canvas, cx, cy) {
        size_t row = cy * width * 2;
        T r, g, b;
        uint8_t y;
        r = canvas(cx, cy, 0);
        g = canvas(cx, cy, 1);
        b = canvas(cx, cy, 2);

        y = std::clamp<uint8_t>(r * .299000 + g * .587000 + b * .114000, 0, 255);
        frame_buffer[row + cx * 2] = y;
        if (!skip) {
          uint8_t u, v;
          u = std::clamp<uint8_t>(r * -.168736 + g * -.331264 + b * .500000 + 128, 0, 255);
          v = std::clamp<uint8_t>(r * .500000 + g * -.418688 + b * -.081312 + 128, 0, 255);
          frame_buffer[row + (cx - 1) * 2 + 1] = u;
          frame_buffer[row + (cx - 1) * 2 + 3] = v;
        }
        skip = !skip;
      }
      write(fd, frame_buffer.data(), frame_buffer.size());
    }

    ~v4l2_cimg() {
      if (fd != -1) {
        close(fd);
      }
    }
};

#endif
