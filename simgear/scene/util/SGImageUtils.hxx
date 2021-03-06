  /* -*-c++-*- */
  /* ImageUtils: copied from  osgEarth - Geospatial SDK for OpenSceneGraph
  * Copyright 2018 Pelican Mapping
  * http://osgearth.org
  *
  * osgEarth is free software; you can redistribute it and/or modify
  * it under the terms of the GNU Lesser General Public License as published by
  * the Free Software Foundation; either version 2 of the License, or
  * (at your option) any later version.
  *
  * This program is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  * GNU Lesser General Public License for more details.
  *
  * You should have received a copy of the GNU Lesser General Public License
  * along with this program.  If not, see <http://www.gnu.org/licenses/>
  */

#ifndef SIMGEAR_IMAGEUTILS_H
#define SIMGEAR_IMAGEUTILS_H

#include <osg/Image>
#include <osg/Texture>
#include <osg/GL>
#include <osg/NodeVisitor>
#include <osgDB/ReaderWriter>
#include <vector>

  //These formats were not added to OSG until after 2.8.3 so we need to define them to use them.
#ifndef GL_EXT_texture_compression_rgtc
#define GL_COMPRESSED_RED_RGTC1_EXT                0x8DBB
#define GL_COMPRESSED_SIGNED_RED_RGTC1_EXT         0x8DBC
#define GL_COMPRESSED_RED_GREEN_RGTC2_EXT          0x8DBD
#define GL_COMPRESSED_SIGNED_RED_GREEN_RGTC2_EXT   0x8DBE
#endif

#ifndef GL_IMG_texture_compression_pvrtc
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG      0x8C00
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG      0x8C01
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG     0x8C02
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG     0x8C03
#endif

namespace simgear
{
    class  ImageUtils
    {
    public:
        /**
        * Clones an image.
        *
        * Use this instead of the osg::Image copy construtor, which keeps the referenced to
        * its underlying BufferObject around. Calling dirty() on the new clone appears to
        * help, but just call this method instead to be sure.
        */
        static osg::Image* cloneImage(const osg::Image* image);

        /**
        * Tweaks an image for consistency. OpenGL allows enums like "GL_RGBA" et.al. to be
        * used in the internal texture format, when really "GL_RGBA8" is the proper things
        * to use. This method accounts for that. Some parts of osgEarth (like the texture-
        * array compositor) rely on the internal texture format being correct.
        * (http://http.download.nvidia.com/developer/Papers/2005/Fast_Texture_Transfers/Fast_Texture_Transfers.pdf)
        */
        static void fixInternalFormat(osg::Image* image);

        /**
        * Marks an image as containing un-normalized data values.
        *
        * Normally the values in an image are "normalized", i.e. scaled so they are in the
        * range [0..1]. This is normal for color values. But when the image is being used
        * for coverage data (a value lookup table) it is desireable to store the raw
        * values instead.
        */
        static void markAsUnNormalized(osg::Image* image, bool value);

        /** Inverse of above. */
        static void markAsNormalized(osg::Image* image, bool value) { markAsUnNormalized(image, !value); }

        /**
        * Whether the image has been marked as containing un-normalized values.
        */
        static bool isUnNormalized(const osg::Image* image);

        /**
        * Whether the image has been marked as containing normalized values.
        */
        static bool isNormalized(const osg::Image* image) { return !isUnNormalized(image); }

        /**
        * Copys a portion of one image into another.
        */
        static bool copyAsSubImage(
            const osg::Image* src,
            osg::Image*       dst,
            int dst_start_col, int dst_start_row);

        /**
        * Resizes an image. Returns a new image, leaving the input image unaltered.
        *
        * Note. If the output parameter is NULL, this method will allocate a new image and
        * resize into that new image. If the output parameter is non-NULL, this method will
        * assume that the output image is already allocated to the proper size, and will
        * do a resize+copy into that image. In the latter case, it is your responsibility
        * to make sure the output image is allocated to the proper size.
        *
        * If the output parameter is non-NULL, then the mipmapLevel is also considered.
        * This lets you resize directly into a particular mipmap level of the output image.
        */
        static bool resizeImage(
            const osg::Image* input,
            unsigned int new_s, unsigned int new_t,
            osg::ref_ptr<osg::Image>& output,
            unsigned int mipmapLevel = 0, bool bilinear = true);

        /**
        * Crops the input image to the dimensions provided and returns a
        * new image. Returns a new image, leaving the input image unaltered.
        * Note:  The input destination bounds are modified to reflect the bounds of the
        *        actual output image.  Due to the fact that you cannot crop in the middle of a pixel
        *        The specified destination extents and the output extents may vary slightly.
        *@param src_minx
        *       The minimum x coordinate of the input image.
        *@param src_miny
        *       The minimum y coordinate of the input image.
        *@param src_maxx
        *       The maximum x coordinate of the input image.
        *@param src_maxy
        *       The maximum y coordinate of the input image.
        *@param dst_minx
        *       The desired minimum x coordinate of the cropped image.
        *@param dst_miny
        *       The desired minimum y coordinate of the cropped image.
        *@param dst_maxx
        *       The desired maximum x coordinate of the cropped image.
        *@param dst_maxy
        *       The desired maximum y coordinate of the cropped image.
        */
        static osg::Image* cropImage(
            const osg::Image* image,
            double src_minx, double src_miny, double src_maxx, double src_maxy,
            double &dst_minx, double &dst_miny, double &dst_maxx, double &dst_maxy);

        /**
        * Creates an Image that "blends" two images into a new image in which "primary"
        * occupies mipmap level 0, and "secondary" occupies all the other mipmap levels.
        *
        * WARNING: this method assumes that primary and seconday are the same exact size
        * and the same exact format.
        */
        static osg::Image* createMipmapBlendedImage(
            const osg::Image* primary,
            const osg::Image* secondary);

        /**
        * Creates a new image containing mipmaps built with nearest-neighbor
        * sampling.
        */
        static osg::Image* buildNearestNeighborMipmaps(
            const osg::Image* image);

        /**
        * Blends the "src" image into the "dest" image, based on the "a" value.
        * The two images must be the same.
        */
        static bool mix(osg::Image* dest, const osg::Image* src, float a);

        /**
        * Creates and returns a copy of the input image after applying a
        * sharpening filter. Returns a new image, leaving the input image unaltered.
        */
        static osg::Image* createSharpenedImage(const osg::Image* image);

        /**
        * For each "layer" in the input image (each bitmap in the "r" dimension),
        * create a new, separate image with r=1. If the input image is r=1, it is
        * simply placed onto the output vector (no copy).
        * Returns true upon sucess, false upon failure
        */
        static bool flattenImage(osg::Image* image, std::vector<osg::ref_ptr<osg::Image> >& output);

        /**
        * Gets whether the input image's dimensions are powers of 2.
        */
        static bool isPowerOfTwo(const osg::Image* image);

        /**
        * Gets a transparent, single pixel image used for a placeholder
        */
        static osg::Image* createEmptyImage();

        /**
        * Gets a transparent image used for a placeholder with the specified dimensions
        */
        static osg::Image* createEmptyImage(unsigned int s, unsigned int t);

        /**
        * Creates a one-pixel image.
        */
        static osg::Image* createOnePixelImage(const osg::Vec4& color);

        /**
        * Tests an image to see whether it's "empty", i.e. completely transparent,
        * within an alpha threshold.
        */
        static bool isEmptyImage(const osg::Image* image, float alphaThreshold = 0.01);

        /**
        * Tests an image to see whether it's "single color", i.e. completely filled with a single color,
        * within an threshold (threshold is tested on each channel).
        */
        static bool isSingleColorImage(const osg::Image* image, float threshold = 0.01);

        /**
        * Returns true if it is possible to convert the image to the specified
        * format/datatype specification.
        */
        static bool canConvert(const osg::Image* image, GLenum pixelFormat, GLenum dataType);

        /**
        * Converts an image to the specified format.
        */
        static osg::Image* convert(const osg::Image* image, GLenum pixelFormat, GLenum dataType);

        /**
        *Converts the given image to RGB8
        */
        static osg::Image* convertToRGB8(const osg::Image* image);

        /**
        *Converts the given image to RGBA8
        */
        static osg::Image* convertToRGBA8(const osg::Image* image);

        /**
        * True if the two images are of the same format (pixel format, data type, etc.)
        * though not necessarily the same size, depth, etc.
        */
        static bool sameFormat(const osg::Image* lhs, const osg::Image* rhs);

        /**
        * True if the two images have the same format AND size, and can therefore
        * be used together in a texture array.
        */
        static bool textureArrayCompatible(const osg::Image* lhs, const osg::Image* rhs);

        /**
        *Compares the image data of two images and determines if they are equivalent
        */
        static bool areEquivalent(const osg::Image *lhs, const osg::Image *rhs);

        /**
        * Whether two colors are roughly equivalent.
        */
        static bool areRGBEquivalent(const osg::Vec4& lhs, const osg::Vec4& rhs, float epsilon = 0.01f) {
            return
                fabs(lhs.r() - rhs.r()) < epsilon &&
                fabs(lhs.g() - rhs.g()) < epsilon &&
                fabs(lhs.b() - rhs.b()) < epsilon;
        }

        /**
        * Checks whether the image has an alpha component
        */
        static bool hasAlphaChannel(const osg::Image* image);

        /**
        * Checks whether an image has transparency; i.e. whether
        * there are any pixels with an alpha component whole value
        * falls below the specified threshold.
        */
        static bool hasTransparency(const osg::Image* image, float alphaThreshold = 1.0f);

        /**
        * Finds pixels with alpha less than [maxAlpha] and sets their color
        * to match that or neighboring non-alpha pixels. This facilitates multipass
        * blending or abutting tiles by overlapping them slightly. Specify "maxAlpha"
        * as the maximum value to consider when searching for fully-transparent pixels.
        *
        * Returns false if there is no reader or writer for the image's format.
        */
        static bool featherAlphaRegions(osg::Image* image, float maxAlpha = 0.0f);

        /**
        * Converts an image (in place) to premultiplied-alpha format.
        * Returns False is the conversion fails, e.g., if there is no reader
        * or writer for the image format.
        */
        static bool convertToPremultipliedAlpha(osg::Image* image);

        /**
        * Checks whether the given image is compressed
        */
        static bool isCompressed(const osg::Image* image);

        /**
        * Generated a bump map image for the input image
        */
        static osg::Image* createBumpMap(const osg::Image* input);

        /**
        * Is it a floating-point texture format?
        */
        static bool isFloatingPointInternalFormat(GLint internalFormat);

        /**
        * Compute a texture compression format suitable for the image.
        */
        static bool computeTextureCompressionMode(
            const osg::Image* image,
            osg::Texture::InternalFormatMode& out_mode);


        /**
        * Bicubic upsampling in a quadrant. Target image is already allocated.
        */
        static bool bicubicUpsample(
            const osg::Image* source,
            osg::Image* target,
            unsigned quadrant,
            unsigned stride);

        /**
        *
        */
        static osg::Image* upSampleNN(const osg::Image* src, int quadrant);

        /**
        * Activates mipmapping for a texture image if the correct filters exist.
        *
        * If OSG has an ImageProcessor service installed, this method will use that
        * to generate mipmaps. If not, the method will be a NOP and the GPU wil
        * generate mipmaps (if necessary) upon GPU transfer.
        */
        static void activateMipMaps(osg::Texture* texture);

        /**
        * Gets an osgDB::ReaderWriter for the given input stream.
        * Returns NULL if no ReaderWriter can be found.
        */
        static osgDB::ReaderWriter* getReaderWriterForStream(std::istream& stream);

        /**
        * Reads an osg::Image from the given input stream.
        * Returns NULL if the image could not be read.
        */
        static osg::Image* readStream(std::istream& stream, const osgDB::Options* options);

        /**
        * Reads color data out of an image, regardles of its internal pixel format.
        */
        class  PixelReader
        {
        public:
            /**
            * Constructs a pixel reader. "Normalized" means that the values in the source
            * image have been scaled to [0..1] and should be denormalized upon reading.
            */
            PixelReader(const osg::Image* image);

            /** Sets an image to read. */
            void setImage(const osg::Image* image);

            /** Whether to use bilinear interpolation when reading with u,v coords (default=true) */
            void setBilinear(bool value) { _bilinear = value; }

            /** Whether PixelReader supports a given format/datatype combiniation. */
            static bool supports(GLenum pixelFormat, GLenum dataType);

            /** Whether PixelReader can read from the specified image. */
            static bool supports(const osg::Image* image) {
                return image && supports(image->getPixelFormat(), image->getDataType());
            }

            /** Reads a color from the image */
            osg::Vec4 operator()(int s, int t, int r = 0, int m = 0) const {
                return (*_reader)(this, s, t, r, m);
            }

            /** Reads a color from the image */
            osg::Vec4 operator()(unsigned s, unsigned t, unsigned r = 0, int m = 0) const {
                return (*_reader)(this, s, t, r, m);
            }

            /** Reads a color from the image by unit coords [0..1] */
            osg::Vec4 operator()(float u, float v, int r = 0, int m = 0) const;
            osg::Vec4 operator()(double u, double v, int r = 0, int m = 0) const;

            // internals:
            const unsigned char* data(int s = 0, int t = 0, int r = 0, int m = 0) const {
                return m == 0 ?
                    _image->data() + s*_colMult + t*_rowMult + r*_imageSize :
                    _image->getMipmapData(m) + s*_colMult + t*(_rowMult >> m) + r*(_imageSize >> m);
            }

            typedef osg::Vec4(*ReaderFunc)(const PixelReader* ia, int s, int t, int r, int m);
            ReaderFunc _reader;
            const osg::Image* _image;
            unsigned _colMult;
            unsigned _rowMult;
            unsigned _imageSize;
            bool     _normalized;
            bool     _bilinear;
        };

        /**
        * Writes color data to an image, regardles of its internal pixel format.
        */
        class  PixelWriter
        {
        public:
            /**
            * Constructs a pixel writer. "Normalized" means the values are scaled to [0..1]
            * before writing.
            */
            PixelWriter(osg::Image* image);

            /** Whether PixelWriter can write to an image with the given format/datatype combo. */
            static bool supports(GLenum pixelFormat, GLenum dataType);

            /** Whether PixelWriter can write to non-const version of an image. */
            static bool supports(const osg::Image* image) {
                return image && supports(image->getPixelFormat(), image->getDataType());
            }

            /** Writes a color to a pixel. */
            void operator()(const osg::Vec4& c, int s, int t, int r = 0, int m = 0) {
                (*_writer)(this, c, s, t, r, m);
            }

            void f(const osg::Vec4& c, float s, float t, int r = 0, int m = 0) {
                this->operator()(c,
                    (int)(s * (float)(_image->s() - 1)),
                    (int)(t * (float)(_image->t() - 1)),
                    r, m);
            }

            // internals:
            osg::Image* _image;
            unsigned _colMult;
            unsigned _rowMult;
            unsigned _imageSize;
            bool     _normalized;

            unsigned char* data(int s = 0, int t = 0, int r = 0, int m = 0) const {
                return m == 0 ?
                    _image->data() + s*_colMult + t*_rowMult + r*_imageSize :
                    _image->getMipmapData(m) + s*_colMult + t*(_rowMult >> m) + r*(_imageSize >> m);
            }

            typedef void(*WriterFunc)(const PixelWriter* iw, const osg::Vec4& c, int s, int t, int r, int m);
            WriterFunc _writer;
        };

        /**
        * Functor that visits every pixel in an image
        */
        template<typename T>
        struct PixelVisitor : public T
        {
            /**
            * Traverse an image, and call this method on the superclass:
            *
            *   bool operator(osg::Vec4& pixel);
            *
            * If that method returns true, write the value back at the same location.
            */
            void accept(osg::Image* image) {
                PixelReader _reader(image);
                PixelWriter _writer(image);
                for (int r = 0; r<image->r(); ++r) {
                    for (int t = 0; t<image->t(); ++t) {
                        for (int s = 0; s<image->s(); ++s) {
                            osg::Vec4f pixel = _reader(s, t, r);
                            if ((*this)(pixel))
                                _writer(pixel, s, t, r);
                        }
                    }
                }
            }

            /**
            * Traverse an image, and call this method on the superclass:
            *
            *   bool operator(const osg::Vec4& srcPixel, osg::Vec4& destPixel);
            *
            * If that method returns true, write destPixel back at the same location
            * in the destination image.
            */
            void accept(const osg::Image* src, osg::Image* dest) {
                PixelReader _readerSrc(src);
                PixelReader _readerDest(dest);
                PixelWriter _writerDest(dest);
                for (int r = 0; r<src->r(); ++r) {
                    for (int t = 0; t<src->t(); ++t) {
                        for (int s = 0; s<src->s(); ++s) {
                            const osg::Vec4f pixelSrc = _readerSrc(s, t, r);
                            osg::Vec4f pixelDest = _readerDest(s, t, r);
                            if ((*this)(pixelSrc, pixelDest))
                                _writerDest(pixelDest, s, t, r);
                        }
                    }
                }
            }
        };

        /**
        * Simple functor to copy pixels from one image to another.
        *
        * Usage:
        *    PixelVisitor<CopyImage>().accept( fromImage, toImage );
        */
        struct CopyImage {
            bool operator()(const osg::Vec4f& src, osg::Vec4f& dest) {
                dest = src;
                return true;
            }
        };
    };

    /** Visitor that finds and operates on textures and images */
    class  TextureAndImageVisitor : public osg::NodeVisitor
    {
    public:
        TextureAndImageVisitor();
        virtual ~TextureAndImageVisitor() { }

    public:
        /** Visits a texture and, by default, all its components images */
        virtual void apply(osg::Texture& texture);

        /** Visits an image inside a texture */
        virtual void apply(osg::Image& image) { }

    public: // osg::NodeVisitor
        virtual void apply(osg::Node& node);
        virtual void apply(osg::StateSet& stateSet);
    };
}

#endif //SIMGEAR_IMAGEUTILS_H
