// SPDX-FileCopyrightText: Copyright 2018 yuzu Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <span>

#include "shader_recompiler/backend/glasm/emit_glasm.h"
#include "video_core/buffer_cache/buffer_cache.h"
#include "video_core/renderer_opengl/gl_buffer_cache.h"
#include "video_core/renderer_opengl/gl_device.h"
#include "video_core/renderer_opengl/maxwell_to_gl.h"

namespace OpenGL {
namespace {
using VideoCore::Surface::PixelFormat;

struct BindlessSSBO {
    GLuint64EXT address;
    GLsizei length;
    GLsizei padding;
};
static_assert(sizeof(BindlessSSBO) == sizeof(GLuint) * 4);

constexpr std::array PROGRAM_LUT{
    GL_VERTEX_PROGRAM_NV,   GL_TESS_CONTROL_PROGRAM_NV, GL_TESS_EVALUATION_PROGRAM_NV,
    GL_GEOMETRY_PROGRAM_NV, GL_FRAGMENT_PROGRAM_NV,
};

[[nodiscard]] GLenum GetTextureBufferFormat(GLenum gl_format) {
    switch (gl_format) {
    case GL_RGBA8_SNORM:
        return GL_RGBA8I;
    case GL_R8_SNORM:
        return GL_R8I;
    case GL_RGBA16_SNORM:
        return GL_RGBA16I;
    case GL_R16_SNORM:
        return GL_R16I;
    case GL_RG16_SNORM:
        return GL_RG16I;
    case GL_RG8_SNORM:
        return GL_RG8I;
    default:
        return gl_format;
    }
}
} // Anonymous namespace

Buffer::Buffer(BufferCacheRuntime&, VideoCommon::NullBufferParams null_params)
    : VideoCommon::BufferBase<VideoCore::RasterizerInterface>(null_params) {}

Buffer::Buffer(BufferCacheRuntime& runtime, VideoCore::RasterizerInterface& rasterizer_,
               VAddr cpu_addr_, u64 size_bytes_)
    : VideoCommon::BufferBase<VideoCore::RasterizerInterface>(rasterizer_, cpu_addr_, size_bytes_) {
    buffer.Create();
    const std::string name = fmt::format("Buffer 0x{:x}", CpuAddr());
    glObjectLabel(GL_BUFFER, buffer.handle, static_cast<GLsizei>(name.size()), name.data());
    glNamedBufferData(buffer.handle, SizeBytes(), nullptr, GL_DYNAMIC_DRAW);

    if (runtime.has_unified_vertex_buffers) {
        glGetNamedBufferParameterui64vNV(buffer.handle, GL_BUFFER_GPU_ADDRESS_NV, &address);
    }
}

void Buffer::ImmediateUpload(size_t offset, std::span<const u8> data) noexcept {
    glNamedBufferSubData(buffer.handle, static_cast<GLintptr>(offset),
                         static_cast<GLsizeiptr>(data.size_bytes()), data.data());
}

void Buffer::ImmediateDownload(size_t offset, std::span<u8> data) noexcept {
    glGetNamedBufferSubData(buffer.handle, static_cast<GLintptr>(offset),
                            static_cast<GLsizeiptr>(data.size_bytes()), data.data());
}

void Buffer::MakeResident(GLenum access) noexcept {
    // Abuse GLenum's order to exit early
    // GL_NONE (default) < GL_READ_ONLY < GL_READ_WRITE
    if (access <= current_residency_access || buffer.handle == 0) {
        return;
    }
    if (std::exchange(current_residency_access, access) != GL_NONE) {
        // If the buffer is already resident, remove its residency before promoting it
        glMakeNamedBufferNonResidentNV(buffer.handle);
    }
    glMakeNamedBufferResidentNV(buffer.handle, access);
}

GLuint Buffer::View(u32 offset, u32 size, PixelFormat format) {
    const auto it{std::ranges::find_if(views, [offset, size, format](const BufferView& view) {
        return offset == view.offset && size == view.size && format == view.format;
    })};
    if (it != views.end()) {
        return it->texture.handle;
    }
    OGLTexture texture;
    texture.Create(GL_TEXTURE_BUFFER);
    const GLenum gl_format{MaxwellToGL::GetFormatTuple(format).internal_format};
    const GLenum texture_format{GetTextureBufferFormat(gl_format)};
    glTextureBufferRange(texture.handle, texture_format, buffer.handle, offset, size);
    views.push_back({
        .offset = offset,
        .size = size,
        .format = format,
        .texture = std::move(texture),
    });
    return views.back().texture.handle;
}

BufferCacheRuntime::BufferCacheRuntime(const Device& device_,
                                       StagingBufferPool& staging_buffer_pool_)
    : device{device_}, staging_buffer_pool{staging_buffer_pool_},
      has_fast_buffer_sub_data{device.HasFastBufferSubData()},
      use_assembly_shaders{device.UseAssemblyShaders()},
      has_unified_vertex_buffers{device.HasVertexBufferUnifiedMemory()},
      stream_buffer{has_fast_buffer_sub_data ? std::nullopt : std::make_optional<StreamBuffer>()} {
    GLint gl_max_attributes;
    glGetIntegerv(GL_MAX_VERTEX_ATTRIBS, &gl_max_attributes);
    max_attributes = static_cast<u32>(gl_max_attributes);
    for (auto& stage_uniforms : fast_uniforms) {
        for (OGLBuffer& buffer : stage_uniforms) {
            buffer.Create();
            glNamedBufferData(buffer.handle, VideoCommon::DEFAULT_SKIP_CACHE_SIZE, nullptr,
                              GL_STREAM_DRAW);
        }
    }
    for (auto& stage_uniforms : copy_uniforms) {
        for (OGLBuffer& buffer : stage_uniforms) {
            buffer.Create();
            glNamedBufferData(buffer.handle, 0x10'000, nullptr, GL_STREAM_COPY);
        }
    }
    for (OGLBuffer& buffer : copy_compute_uniforms) {
        buffer.Create();
        glNamedBufferData(buffer.handle, 0x10'000, nullptr, GL_STREAM_COPY);
    }

    device_access_memory = [this]() -> u64 {
        if (device.CanReportMemoryUsage()) {
            return device.GetCurrentDedicatedVideoMemory() + 512_MiB;
        }
        return 2_GiB; // Return minimum requirements
    }();
}

StagingBufferMap BufferCacheRuntime::UploadStagingBuffer(size_t size) {
    return staging_buffer_pool.RequestUploadBuffer(size);
}

StagingBufferMap BufferCacheRuntime::DownloadStagingBuffer(size_t size) {
    return staging_buffer_pool.RequestDownloadBuffer(size);
}

u64 BufferCacheRuntime::GetDeviceMemoryUsage() const {
    if (device.CanReportMemoryUsage()) {
        return device_access_memory - device.GetCurrentDedicatedVideoMemory();
    }
    return 2_GiB;
}

void BufferCacheRuntime::CopyBuffer(GLuint dst_buffer, GLuint src_buffer,
                                    std::span<const VideoCommon::BufferCopy> copies, bool barrier) {
    if (barrier) {
        PreCopyBarrier();
    }
    for (const VideoCommon::BufferCopy& copy : copies) {
        glCopyNamedBufferSubData(src_buffer, dst_buffer, static_cast<GLintptr>(copy.src_offset),
                                 static_cast<GLintptr>(copy.dst_offset),
                                 static_cast<GLsizeiptr>(copy.size));
    }
    if (barrier) {
        PostCopyBarrier();
    }
}

void BufferCacheRuntime::CopyBuffer(GLuint dst_buffer, Buffer& src_buffer,
                                    std::span<const VideoCommon::BufferCopy> copies, bool barrier) {
    CopyBuffer(dst_buffer, src_buffer.Handle(), copies, barrier);
}

void BufferCacheRuntime::CopyBuffer(Buffer& dst_buffer, GLuint src_buffer,
                                    std::span<const VideoCommon::BufferCopy> copies, bool barrier) {
    CopyBuffer(dst_buffer.Handle(), src_buffer, copies, barrier);
}

void BufferCacheRuntime::CopyBuffer(Buffer& dst_buffer, Buffer& src_buffer,
                                    std::span<const VideoCommon::BufferCopy> copies) {
    CopyBuffer(dst_buffer.Handle(), src_buffer.Handle(), copies);
}

void BufferCacheRuntime::PreCopyBarrier() {
    // TODO: finer grained barrier?
    glMemoryBarrier(GL_ALL_BARRIER_BITS);
}

void BufferCacheRuntime::PostCopyBarrier() {
    glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT | GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT);
}

void BufferCacheRuntime::Finish() {
    glFinish();
}

void BufferCacheRuntime::ClearBuffer(Buffer& dest_buffer, u32 offset, size_t size, u32 value) {
    glClearNamedBufferSubData(dest_buffer.Handle(), GL_R32UI, static_cast<GLintptr>(offset),
                              static_cast<GLsizeiptr>(size), GL_RED, GL_UNSIGNED_INT, &value);
}

void BufferCacheRuntime::BindIndexBuffer(Buffer& buffer, u32 offset, u32 size) {
    if (has_unified_vertex_buffers) {
        buffer.MakeResident(GL_READ_ONLY);
        glBufferAddressRangeNV(GL_ELEMENT_ARRAY_ADDRESS_NV, 0, buffer.HostGpuAddr() + offset,
                               static_cast<GLsizeiptr>(Common::AlignUp(size, 4)));
    } else {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffer.Handle());
        index_buffer_offset = offset;
    }
}

void BufferCacheRuntime::BindVertexBuffer(u32 index, Buffer& buffer, u32 offset, u32 size,
                                          u32 stride) {
    if (index >= max_attributes) {
        return;
    }
    if (has_unified_vertex_buffers) {
        buffer.MakeResident(GL_READ_ONLY);
        glBindVertexBuffer(index, 0, 0, static_cast<GLsizei>(stride));
        glBufferAddressRangeNV(GL_VERTEX_ATTRIB_ARRAY_ADDRESS_NV, index,
                               buffer.HostGpuAddr() + offset, static_cast<GLsizeiptr>(size));
    } else {
        glBindVertexBuffer(index, buffer.Handle(), static_cast<GLintptr>(offset),
                           static_cast<GLsizei>(stride));
    }
}

void BufferCacheRuntime::BindVertexBuffers(VideoCommon::HostBindings& bindings) {
    for (u32 index = 0; index < bindings.buffers.size(); index++) {
        BindVertexBuffer(
            bindings.min_index + index, *reinterpret_cast<Buffer*>(bindings.buffers[index]),
            static_cast<u32>(bindings.offsets[index]), static_cast<u32>(bindings.sizes[index]),
            static_cast<u32>(bindings.strides[index]));
    }
}

void BufferCacheRuntime::BindUniformBuffer(size_t stage, u32 binding_index, Buffer& buffer,
                                           u32 offset, u32 size) {
    if (use_assembly_shaders) {
        GLuint handle;
        if (offset != 0) {
            handle = copy_uniforms[stage][binding_index].handle;
            glCopyNamedBufferSubData(buffer.Handle(), handle, offset, 0, size);
        } else {
            handle = buffer.Handle();
        }
        glBindBufferRangeNV(PABO_LUT[stage], binding_index, handle, 0,
                            static_cast<GLsizeiptr>(size));
    } else {
        const GLuint base_binding = graphics_base_uniform_bindings[stage];
        const GLuint binding = base_binding + binding_index;
        glBindBufferRange(GL_UNIFORM_BUFFER, binding, buffer.Handle(),
                          static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size));
    }
}

void BufferCacheRuntime::BindComputeUniformBuffer(u32 binding_index, Buffer& buffer, u32 offset,
                                                  u32 size) {
    if (use_assembly_shaders) {
        GLuint handle;
        if (offset != 0) {
            handle = copy_compute_uniforms[binding_index].handle;
            glCopyNamedBufferSubData(buffer.Handle(), handle, offset, 0, size);
        } else {
            handle = buffer.Handle();
        }
        glBindBufferRangeNV(GL_COMPUTE_PROGRAM_PARAMETER_BUFFER_NV, binding_index, handle, 0,
                            static_cast<GLsizeiptr>(size));
    } else {
        glBindBufferRange(GL_UNIFORM_BUFFER, binding_index, buffer.Handle(),
                          static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size));
    }
}

void BufferCacheRuntime::BindStorageBuffer(size_t stage, u32 binding_index, Buffer& buffer,
                                           u32 offset, u32 size, bool is_written) {
    if (use_storage_buffers) {
        const GLuint base_binding = graphics_base_storage_bindings[stage];
        const GLuint binding = base_binding + binding_index;
        glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding, buffer.Handle(),
                          static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size));
    } else {
        const BindlessSSBO ssbo{
            .address = buffer.HostGpuAddr() + offset,
            .length = static_cast<GLsizei>(size),
            .padding = 0,
        };
        buffer.MakeResident(is_written ? GL_READ_WRITE : GL_READ_ONLY);
        glProgramLocalParametersI4uivNV(
            PROGRAM_LUT[stage],
            Shader::Backend::GLASM::PROGRAM_LOCAL_PARAMETER_STORAGE_BUFFER_BASE + binding_index, 1,
            reinterpret_cast<const GLuint*>(&ssbo));
    }
}

void BufferCacheRuntime::BindComputeStorageBuffer(u32 binding_index, Buffer& buffer, u32 offset,
                                                  u32 size, bool is_written) {
    if (use_storage_buffers) {
        if (size != 0) {
            glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding_index, buffer.Handle(),
                              static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size));
        } else {
            glBindBufferRange(GL_SHADER_STORAGE_BUFFER, binding_index, 0, 0, 0);
        }
    } else {
        const BindlessSSBO ssbo{
            .address = buffer.HostGpuAddr() + offset,
            .length = static_cast<GLsizei>(size),
            .padding = 0,
        };
        buffer.MakeResident(is_written ? GL_READ_WRITE : GL_READ_ONLY);
        glProgramLocalParametersI4uivNV(
            GL_COMPUTE_PROGRAM_NV,
            Shader::Backend::GLASM::PROGRAM_LOCAL_PARAMETER_STORAGE_BUFFER_BASE + binding_index, 1,
            reinterpret_cast<const GLuint*>(&ssbo));
    }
}

void BufferCacheRuntime::BindTransformFeedbackBuffer(u32 index, Buffer& buffer, u32 offset,
                                                     u32 size) {
    glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, index, buffer.Handle(),
                      static_cast<GLintptr>(offset), static_cast<GLsizeiptr>(size));
}

void BufferCacheRuntime::BindTransformFeedbackBuffers(VideoCommon::HostBindings& bindings) {
    for (u32 index = 0; index < bindings.buffers.size(); index++) {
        glBindBufferRange(GL_TRANSFORM_FEEDBACK_BUFFER, index,
                          reinterpret_cast<Buffer*>(bindings.buffers[index])->Handle(),
                          static_cast<GLintptr>(bindings.offsets[index]),
                          static_cast<GLsizeiptr>(bindings.sizes[index]));
    }
}

void BufferCacheRuntime::BindTextureBuffer(Buffer& buffer, u32 offset, u32 size,
                                           PixelFormat format) {
    *texture_handles++ = buffer.View(offset, size, format);
}

void BufferCacheRuntime::BindImageBuffer(Buffer& buffer, u32 offset, u32 size, PixelFormat format) {
    *image_handles++ = buffer.View(offset, size, format);
}

} // namespace OpenGL
