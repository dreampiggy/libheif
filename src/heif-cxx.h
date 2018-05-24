/*
 * C++ interface to libheif
 * Copyright (c) 2018 struktur AG, Dirk Farin <farin@struktur.de>
 *
 * This file is part of libheif.
 *
 * heif is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * heif is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with heif.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef LIBHEIF_HEIF_CXX_H
#define LIBHEIF_HEIF_CXX_H

#include <memory>
#include <string>
#include <vector>

extern "C" {
#include <libheif/heif.h>
}


namespace heif {

  class Error
  {
  public:
    Error() {
      m_code = heif_error_Ok;
      m_subcode = heif_suberror_Unspecified;
      m_message = "Ok";
    }

    Error(const heif_error& err) {
      m_code = err.code;
      m_subcode = err.subcode;
      m_message = err.message;
    }

    std::string get_message() const { return m_message; }

    heif_error_code get_code() const { return m_code; }

    heif_suberror_code get_subcode() const { return m_subcode; }

    operator bool() const { return m_code != heif_error_Ok; }

  private:
    heif_error_code m_code;
    heif_suberror_code m_subcode;
    std::string m_message;
  };


  class ImageHandle;
  class Image;


  class Context
  {
  public:
    Context();

    class ReadingOptions { };

    // throws Error
    void read_from_file(std::string filename, const ReadingOptions& opts = ReadingOptions());

    // throws Error
    void read_from_memory(const void* mem, size_t size, const ReadingOptions& opts = ReadingOptions());

    int get_number_of_top_level_images() const;

    bool is_top_level_image_ID(heif_item_id id) const;

    std::vector<heif_item_id> get_list_of_top_level_image_IDs() const;

    // throws Error
    heif_item_id get_primary_image_ID() const;

    // throws Error
    ImageHandle get_primary_image_handle() const;


    class Writer {
    public:
      virtual ~Writer() { }

      virtual heif_error write(Context&, const void* data, size_t size) = 0;
    };

    // throws Error
    void write(Writer&);

    // throws Error
    void write_to_file(std::string filename) const;

  private:
    std::shared_ptr<heif_context> m_context;

    friend struct ::heif_error heif_writer_trampoline_write(struct heif_context* ctx,
                                                            const void* data,
                                                            size_t size,
                                                            void* userdata);

    static Context wrap_without_releasing(heif_context*); // internal use in friend function only
  };


  class ImageHandle
  {
  public:
    ImageHandle() { }

    ImageHandle(heif_image_handle* handle);

    bool is_primary_image() const;

    int get_width() const;

    int get_height() const;

    bool has_alpha_channel() const;

    // ------------------------- depth images -------------------------

    // TODO

    // ------------------------- thumbnails -------------------------

    int get_number_of_thumbnails() const;

    std::vector<heif_item_id> get_list_of_thumbnail_IDs() const;

    // throws Error
    ImageHandle get_thumbnail(heif_item_id id);

    // ------------------------- metadata (Exif / XMP) -------------------------

    // TODO


    class DecodingOptions { };

    // throws Error
    Image decode_image(heif_colorspace colorspace, heif_chroma chroma,
                       const DecodingOptions& options = DecodingOptions());

  private:
    std::shared_ptr<heif_image_handle> m_image_handle;
  };


  class Image
  {
  public:
    Image() { }
    Image(heif_image* image);


    // throws Error
    void create(int width, int height,
                enum heif_colorspace colorspace,
                enum heif_chroma chroma);

    // throws Error
    void add_plane(enum heif_channel channel,
                   int width, int height, int bit_depth);

    heif_colorspace get_colorspace() const;

    heif_chroma get_chroma_format() const;

    int get_width(enum heif_channel channel) const;

    int get_height(enum heif_channel channel) const;

    int get_bits_per_pixel(enum heif_channel channel) const;

    const uint8_t* get_plane(enum heif_channel channel, int* out_stride) const;

    uint8_t* get_plane(enum heif_channel channel, int* out_stride);

    class ScalingOptions { };

    // throws Error
    Image scale_image(int width, int height,
                      const ScalingOptions& options = ScalingOptions()) const;

  private:
    std::shared_ptr<heif_image> m_image;
  };


  // ==========================================================================================
  //                                     IMPLEMENTATION
  // ==========================================================================================

  inline Context::Context() {
    heif_context* ctx = heif_context_alloc();
    m_context = std::shared_ptr<heif_context>(ctx,
                                              [] (heif_context* c) { heif_context_free(c); });
  }

  inline void Context::read_from_file(std::string filename, const ReadingOptions& opts) {
    Error err = Error(heif_context_read_from_file(m_context.get(), filename.c_str(), NULL));
    if (err) {
      throw err;
    }
  }

  inline void Context::read_from_memory(const void* mem, size_t size, const ReadingOptions& opts) {
    Error err = Error(heif_context_read_from_memory(m_context.get(), mem, size, NULL));
    if (err) {
      throw err;
    }
  }


  inline int Context::get_number_of_top_level_images() const {
    return heif_context_get_number_of_top_level_images(m_context.get());
  }

  inline bool Context::is_top_level_image_ID(heif_item_id id) const {
    return heif_context_is_top_level_image_ID(m_context.get(), id);
  }

  inline std::vector<heif_item_id> Context::get_list_of_top_level_image_IDs() const {
    int num = get_number_of_top_level_images();
    std::vector<heif_item_id> IDs(num);
    heif_context_get_list_of_top_level_image_IDs(m_context.get(), IDs.data(), num);
    return IDs;
  }

  inline heif_item_id Context::get_primary_image_ID() const {
    heif_item_id id;
    Error err = Error(heif_context_get_primary_image_ID(m_context.get(), &id));
    if (err) {
      throw err;
    }
    return id;
  }

  inline ImageHandle Context::get_primary_image_handle() const {
    heif_image_handle* handle;
    Error err = Error(heif_context_get_primary_image_handle(m_context.get(), &handle));
    if (err) {
      throw err;
    }

    return ImageHandle(handle);
  }

  Context Context::wrap_without_releasing(heif_context* ctx) {
    Context context;
    context.m_context = std::shared_ptr<heif_context>(ctx,
                                                      [] (heif_context* c) { /* NOP */ });
    return context;
  }

  inline struct ::heif_error heif_writer_trampoline_write(struct heif_context* ctx,
                                                          const void* data,
                                                          size_t size,
                                                          void* userdata) {
    Context::Writer* writer = (Context::Writer*)userdata;

    Context context = Context::wrap_without_releasing(ctx);
    return writer->write(context, data, size);
  }

  static struct heif_writer heif_writer_trampoline =
    {
      1,
      &heif_writer_trampoline_write
    };

  inline void Context::write(Writer& writer) {
    Error err = Error(heif_context_write(m_context.get(), &heif_writer_trampoline, &writer));
    if (err) {
      throw err;
    }
  }

  inline void Context::write_to_file(std::string filename) const {
    Error err = Error(heif_context_write_to_file(m_context.get(), filename.c_str()));
    if (err) {
      throw err;
    }
  }



  inline ImageHandle::ImageHandle(heif_image_handle* handle) {
    m_image_handle = std::shared_ptr<heif_image_handle>(handle,
                                                        [] (heif_image_handle* h) { heif_image_handle_release(h); });
  }

  inline bool ImageHandle::is_primary_image() const {
    return heif_image_handle_is_primary_image(m_image_handle.get()) != 0;
  }

  inline int ImageHandle::get_width() const {
    return heif_image_handle_get_width(m_image_handle.get());
  }

  inline int ImageHandle::get_height() const {
    return heif_image_handle_get_height(m_image_handle.get());
  }

  inline bool ImageHandle::has_alpha_channel() const {
    return heif_image_handle_has_alpha_channel(m_image_handle.get()) != 0;
  }

  // ------------------------- depth images -------------------------

  // TODO

  // ------------------------- thumbnails -------------------------

  inline int ImageHandle::get_number_of_thumbnails() const {
    return heif_image_handle_get_number_of_thumbnails(m_image_handle.get());
  }

  inline std::vector<heif_item_id> ImageHandle::get_list_of_thumbnail_IDs() const {
    int num = get_number_of_thumbnails();
    std::vector<heif_item_id> IDs(num);
    heif_image_handle_get_list_of_thumbnail_IDs(m_image_handle.get(), IDs.data(), num);
    return IDs;
  }

  inline ImageHandle ImageHandle::get_thumbnail(heif_item_id id) {
    heif_image_handle* handle;
    Error err = Error(heif_image_handle_get_thumbnail(m_image_handle.get(), id, &handle));
    if (err) {
      throw err;
    }

    return ImageHandle(handle);
  }

  inline Image ImageHandle::decode_image(heif_colorspace colorspace, heif_chroma chroma,
                                         const DecodingOptions& options) {
    heif_image* out_img;
    Error err = Error(heif_decode_image(m_image_handle.get(),
                                        &out_img,
                                        colorspace,
                                        chroma,
                                        nullptr)); //const struct heif_decoding_options* options);
    if (err) {
      throw err;
    }

    return Image(out_img);
  }



  inline Image::Image(heif_image* image) {
    m_image = std::shared_ptr<heif_image>(image,
                                          [] (heif_image* h) { heif_image_release(h); });
  }


  inline void Image::create(int width, int height,
                            enum heif_colorspace colorspace,
                            enum heif_chroma chroma) {
    heif_image* image;
    Error err = Error(heif_image_create(width, height, colorspace, chroma, &image));
    if (err) {
      m_image.reset();
      throw err;
    }
    else {
      m_image = std::shared_ptr<heif_image>(image,
                                            [] (heif_image* h) { heif_image_release(h); });
    }
  }

  inline void Image::add_plane(enum heif_channel channel,
                               int width, int height, int bit_depth) {
    Error err = Error(heif_image_add_plane(m_image.get(), channel, width, height, bit_depth));
    if (err) {
      throw err;
    }
  }

  inline heif_colorspace Image::get_colorspace() const {
    return heif_image_get_colorspace(m_image.get());
  }

  inline heif_chroma Image::get_chroma_format() const {
    return heif_image_get_chroma_format(m_image.get());
  }

  inline int Image::get_width(enum heif_channel channel) const {
    return heif_image_get_width(m_image.get(), channel);
  }

  inline int Image::get_height(enum heif_channel channel) const {
    return heif_image_get_height(m_image.get(), channel);
  }

  inline int Image::get_bits_per_pixel(enum heif_channel channel) const {
    return heif_image_get_bits_per_pixel(m_image.get(), channel);
  }

  inline const uint8_t* Image::get_plane(enum heif_channel channel, int* out_stride) const {
    return heif_image_get_plane_readonly(m_image.get(), channel, out_stride);
  }

  inline uint8_t* Image::get_plane(enum heif_channel channel, int* out_stride) {
    return heif_image_get_plane(m_image.get(), channel, out_stride);
  }

  inline Image Image::scale_image(int width, int height,
                                  const ScalingOptions& options) const {
    heif_image* img;
    Error err = Error(heif_image_scale_image(m_image.get(), &img, width,height,
                                             nullptr)); // TODO: scaling options not defined yet
    if (err) {
      throw err;
    }

    return Image(img);
  }

}


#endif