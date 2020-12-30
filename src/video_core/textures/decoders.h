// Copyright 2018 yuzu Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <span>

#include "common/common_types.h"
#include "video_core/textures/texture.h"

namespace Tegra::Texture {

constexpr u32 GOB_SIZE_X = 64;
constexpr u32 GOB_SIZE_Y = 8;
constexpr u32 GOB_SIZE_Z = 1;
constexpr u32 GOB_SIZE = GOB_SIZE_X * GOB_SIZE_Y * GOB_SIZE_Z;

constexpr u32 GOB_SIZE_X_SHIFT = 6;
constexpr u32 GOB_SIZE_Y_SHIFT = 3;
constexpr u32 GOB_SIZE_Z_SHIFT = 0;
constexpr u32 GOB_SIZE_SHIFT = GOB_SIZE_X_SHIFT + GOB_SIZE_Y_SHIFT + GOB_SIZE_Z_SHIFT;

using SwizzleTable = std::array<std::array<u32, GOB_SIZE_X>, GOB_SIZE_Y>;

/// Returns a z-order swizzle table
SwizzleTable MakeSwizzleTable();

/// Unswizzles a block linear texture into linear memory.
void UnswizzleTexture(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel,
                      u32 width, u32 height, u32 depth, u32 block_height, u32 block_depth,
                      u32 stride_alignment = 1);

/// Swizzles linear memory into a block linear texture.
void SwizzleTexture(std::span<u8> output, std::span<const u8> input, u32 bytes_per_pixel, u32 width,
                    u32 height, u32 depth, u32 block_height, u32 block_depth,
                    u32 stride_alignment = 1);

/// This function calculates the correct size of a texture depending if it's tiled or not.
std::size_t CalculateSize(bool tiled, u32 bytes_per_pixel, u32 width, u32 height, u32 depth,
                          u32 block_height, u32 block_depth);

/// Copies an untiled subrectangle into a tiled surface.
void SwizzleSubrect(u32 subrect_width, u32 subrect_height, u32 source_pitch, u32 swizzled_width,
                    u32 bytes_per_pixel, u8* swizzled_data, const u8* unswizzled_data,
                    u32 block_height_bit, u32 offset_x, u32 offset_y);

/// Copies a tiled subrectangle into a linear surface.
void UnswizzleSubrect(u32 line_length_in, u32 line_count, u32 pitch, u32 width, u32 bytes_per_pixel,
                      u32 block_height, u32 origin_x, u32 origin_y, u8* output, const u8* input);

/// @brief Swizzles a 2D array of pixels into a 3D texture
/// @param line_length_in  Number of pixels per line
/// @param line_count      Number of lines
/// @param pitch           Number of bytes per line
/// @param width           Width of the swizzled texture
/// @param height          Height of the swizzled texture
/// @param bytes_per_pixel Number of bytes used per pixel
/// @param block_height    Block height shift
/// @param block_depth     Block depth shift
/// @param origin_x        Column offset in pixels of the swizzled texture
/// @param origin_y        Row offset in pixels of the swizzled texture
/// @param output          Pointer to the pixels of the swizzled texture
/// @param input           Pointer to the 2D array of pixels used as input
/// @pre input and output points to an array large enough to hold the number of bytes used
void SwizzleSliceToVoxel(u32 line_length_in, u32 line_count, u32 pitch, u32 width, u32 height,
                         u32 bytes_per_pixel, u32 block_height, u32 block_depth, u32 origin_x,
                         u32 origin_y, u8* output, const u8* input);

void SwizzleKepler(u32 width, u32 height, u32 dst_x, u32 dst_y, u32 block_height,
                   std::size_t copy_size, const u8* source_data, u8* swizzle_data);

/// Obtains the offset of the gob for positions 'dst_x' & 'dst_y'
u64 GetGOBOffset(u32 width, u32 height, u32 dst_x, u32 dst_y, u32 block_height,
                 u32 bytes_per_pixel);

} // namespace Tegra::Texture