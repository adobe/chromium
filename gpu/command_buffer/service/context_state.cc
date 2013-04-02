// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gpu/command_buffer/service/context_state.h"

#include "gpu/command_buffer/common/gles2_cmd_utils.h"
#include "gpu/command_buffer/service/buffer_manager.h"
#include "gpu/command_buffer/service/framebuffer_manager.h"
#include "gpu/command_buffer/service/program_manager.h"
#include "gpu/command_buffer/service/renderbuffer_manager.h"
#include "ui/gl/gl_bindings.h"
#include "ui/gl/gl_implementation.h"

namespace gpu {
namespace gles2 {

namespace {

void EnableDisable(GLenum pname, bool enable) {
  if (enable) {
    glEnable(pname);
  } else {
    glDisable(pname);
  }
}

}  // anonymous namespace.

TextureUnit::TextureUnit()
    : bind_target(GL_TEXTURE_2D) {
}

TextureUnit::~TextureUnit() {
}

ContextState::ContextState(FeatureInfo* feature_info)
    : pack_alignment(4),
      unpack_alignment(4),
      active_texture_unit(0),
      hint_generate_mipmap(GL_DONT_CARE),
      hint_fragment_shader_derivative(GL_DONT_CARE),
      pack_reverse_row_order(false),
      feature_info_(feature_info) {
  Initialize();
}

ContextState::~ContextState() {
}

void ContextState::RestoreTextureUnitBindings(GLuint unit, const ContextState* previous_state) const {
  DCHECK_LT(unit, texture_units.size());
  const TextureUnit& texture_unit = texture_units[unit];
  if (!previous_state || unit >= previous_state->texture_units.size()) {
    glActiveTexture(GL_TEXTURE0 + unit);
    GLuint service_id = texture_unit.bound_texture_2d ?
        texture_unit.bound_texture_2d->service_id() : 0;
    glBindTexture(GL_TEXTURE_2D, service_id);
    service_id = texture_unit.bound_texture_cube_map ?
        texture_unit.bound_texture_cube_map->service_id() : 0;
    glBindTexture(GL_TEXTURE_CUBE_MAP, service_id);

    if (feature_info_->feature_flags().oes_egl_image_external) {
      service_id = texture_unit.bound_texture_external_oes ?
          texture_unit.bound_texture_external_oes->service_id() : 0;
      glBindTexture(GL_TEXTURE_EXTERNAL_OES, service_id);
    }

    if (feature_info_->feature_flags().arb_texture_rectangle) {
      service_id = texture_unit.bound_texture_rectangle_arb ?
          texture_unit.bound_texture_rectangle_arb->service_id() : 0;
      glBindTexture(GL_TEXTURE_RECTANGLE_ARB, service_id);
    }
  } else {
    DCHECK_LT(unit, previous_state->texture_units.size());
    const TextureUnit& prev_texture_unit = previous_state->texture_units[unit];

    bool activated_texture_unit = false;
    
    GLuint service_id = texture_unit.bound_texture_2d ?
        texture_unit.bound_texture_2d->service_id() : 0;
    GLuint prev_service_id = prev_texture_unit.bound_texture_2d ?
        prev_texture_unit.bound_texture_2d->service_id() : 0;
    if (service_id != prev_service_id) {
      if (!activated_texture_unit) {
        activated_texture_unit = true;
        glActiveTexture(GL_TEXTURE0 + unit);
      }
      glBindTexture(GL_TEXTURE_2D, service_id);
    }

    service_id = texture_unit.bound_texture_cube_map ?
        texture_unit.bound_texture_cube_map->service_id() : 0;
    prev_service_id = prev_texture_unit.bound_texture_cube_map ?
        prev_texture_unit.bound_texture_cube_map->service_id() : 0;
    if (service_id != prev_service_id) {
      if (!activated_texture_unit) {
        activated_texture_unit = true;
        glActiveTexture(GL_TEXTURE0 + unit);
      }
      glBindTexture(GL_TEXTURE_CUBE_MAP, service_id);
    }

    if (feature_info_->feature_flags().oes_egl_image_external) {
      service_id = texture_unit.bound_texture_external_oes ?
          texture_unit.bound_texture_external_oes->service_id() : 0;
      prev_service_id = prev_texture_unit.bound_texture_external_oes ?
          prev_texture_unit.bound_texture_external_oes->service_id() : 0;
      if (service_id != prev_service_id) {
        if (!activated_texture_unit) {
          activated_texture_unit = true;
          glActiveTexture(GL_TEXTURE0 + unit);
        }
        glBindTexture(GL_TEXTURE_EXTERNAL_OES, service_id);
      }
    }

    if (feature_info_->feature_flags().arb_texture_rectangle) {
      service_id = texture_unit.bound_texture_rectangle_arb ?
          texture_unit.bound_texture_rectangle_arb->service_id() : 0;
      prev_service_id = prev_texture_unit.bound_texture_rectangle_arb ?
          prev_texture_unit.bound_texture_rectangle_arb->service_id() : 0;
      if (service_id != prev_service_id) {
        if (!activated_texture_unit) {
          activated_texture_unit = true;
          glActiveTexture(GL_TEXTURE0 + unit);
        }
        glBindTexture(GL_TEXTURE_RECTANGLE_ARB, service_id);
      }
    }
  }
}

void ContextState::RestoreBufferBindings(const ContextState* previous_state) const {
  if (vertex_attrib_manager) {
    Buffer* element_array_buffer =
        vertex_attrib_manager->element_array_buffer();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,
        element_array_buffer ? element_array_buffer->service_id() : 0);
  }
  glBindBuffer(
      GL_ARRAY_BUFFER,
      bound_array_buffer ? bound_array_buffer->service_id() : 0);
}

void ContextState::RestoreRenderbufferBindings(const ContextState* previous_state) const {
  // Restore Bindings
  glBindRenderbufferEXT(
      GL_RENDERBUFFER,
      bound_renderbuffer ? bound_renderbuffer->service_id() : 0);
}

void ContextState::RestoreProgramBindings(const ContextState* previous_state) const {
  glUseProgram(current_program ? current_program->service_id() : 0);
}

void ContextState::RestoreActiveTexture(const ContextState* previous_state) const {
  if (!previous_state || previous_state->active_texture_unit != active_texture_unit)
    glActiveTexture(GL_TEXTURE0 + active_texture_unit);
}

void ContextState::RestoreAttribute(GLuint attrib, const ContextState* previous_state) const {
  const VertexAttrib* info =
      vertex_attrib_manager->GetVertexAttrib(attrib);
  const VertexAttrib* prev_info = 0;
  if (previous_state && attrib < previous_state->vertex_attrib_manager->num_attribs())
      prev_info = previous_state->vertex_attrib_manager->GetVertexAttrib(attrib);
  const void* ptr = reinterpret_cast<const void*>(info->offset());
  Buffer* buffer_info = info->buffer();
  glBindBuffer(
      GL_ARRAY_BUFFER, buffer_info ? buffer_info->service_id() : 0);
  glVertexAttribPointer(
      attrib, info->size(), info->type(), info->normalized(),
      info->gl_stride(), ptr);
  if (info->divisor())
    glVertexAttribDivisorANGLE(attrib, info->divisor());
  // Never touch vertex attribute 0's state (in particular, never
  // disable it) when running on desktop GL because it will never be
  // re-enabled.
  if (attrib != 0 ||
      gfx::GetGLImplementation() == gfx::kGLImplementationEGLGLES2) {
    if (!prev_info || prev_info->enabled() != info->enabled()) {
      if (info->enabled()) {
        glEnableVertexAttribArray(attrib);
      } else {
        glDisableVertexAttribArray(attrib);
      }
    }
  }
  glVertexAttrib4fv(attrib, attrib_values[attrib].v);
}

void ContextState::RestoreCapabilities(const ContextState* previous_state) const {
  if (previous_state->enable_flags.blend != enable_flags.blend)
    EnableDisable(GL_BLEND, enable_flags.blend);
  
  if (previous_state->enable_flags.cull_face != enable_flags.cull_face)
    EnableDisable(GL_CULL_FACE, enable_flags.cull_face);

  if (previous_state->enable_flags.depth_test != enable_flags.depth_test)
    EnableDisable(GL_DEPTH_TEST, enable_flags.depth_test);
  
  if (previous_state->enable_flags.dither != enable_flags.dither)
    EnableDisable(GL_DITHER, enable_flags.dither);
  
  if (previous_state->enable_flags.polygon_offset_fill != enable_flags.polygon_offset_fill)
    EnableDisable(GL_POLYGON_OFFSET_FILL, enable_flags.polygon_offset_fill);
  
  if (previous_state->enable_flags.sample_alpha_to_coverage != enable_flags.sample_alpha_to_coverage)
    EnableDisable(
      GL_SAMPLE_ALPHA_TO_COVERAGE, enable_flags.sample_alpha_to_coverage);
  
  if (previous_state->enable_flags.sample_coverage != enable_flags.sample_coverage)
    EnableDisable(GL_SAMPLE_COVERAGE, enable_flags.sample_coverage);
  
  if (previous_state->enable_flags.scissor_test != enable_flags.scissor_test)
    EnableDisable(GL_SCISSOR_TEST, enable_flags.scissor_test);
  
  if (previous_state->enable_flags.stencil_test != enable_flags.stencil_test)
    EnableDisable(GL_STENCIL_TEST, enable_flags.stencil_test);
}

void ContextState::RestoreInternalState(const ContextState* previous_state) const {
  if (previous_state->blend_color_red != blend_color_red
    || previous_state->blend_color_green != blend_color_green
    || previous_state->blend_color_blue != blend_color_blue
    || previous_state->blend_color_alpha != blend_color_alpha)
    glBlendColor(
      blend_color_red, blend_color_green, blend_color_blue, blend_color_alpha);

  if (previous_state->blend_equation_rgb != blend_equation_rgb
    || previous_state->blend_equation_alpha != blend_equation_alpha)
    glBlendEquationSeparate(blend_equation_rgb, blend_equation_alpha);

  if (previous_state->blend_source_rgb != blend_source_rgb
    || previous_state->blend_dest_rgb != blend_dest_rgb
    || previous_state->blend_source_alpha != blend_source_alpha
    || previous_state->blend_dest_alpha != blend_dest_alpha)
    glBlendFuncSeparate(
        blend_source_rgb, blend_dest_rgb, blend_source_alpha, blend_dest_alpha);
  
  if (previous_state->color_clear_red != color_clear_red
    || previous_state->color_clear_green != color_clear_green
    || previous_state->color_clear_blue != color_clear_blue
    || previous_state->color_clear_alpha != color_clear_alpha)
    glClearColor(
        color_clear_red, color_clear_green, color_clear_blue, color_clear_alpha);
  
  if (previous_state->depth_clear != depth_clear)
    glClearDepth(depth_clear);

  if (previous_state->stencil_clear != stencil_clear)
    glClearStencil(stencil_clear);
  
  if (previous_state->color_mask_red != color_mask_red
    || previous_state->color_mask_green != color_mask_green
    || previous_state->color_mask_blue != color_mask_blue
    || previous_state->color_mask_alpha != color_mask_alpha)
    glColorMask(
      color_mask_red, color_mask_green, color_mask_blue, color_mask_alpha);
  
  if (previous_state->cull_mode != cull_mode)
    glCullFace(cull_mode);

  if (previous_state->depth_func != depth_func)
    glDepthFunc(depth_func);
  
  if (previous_state->depth_mask != depth_mask)
    glDepthMask(depth_mask);
  
  if (previous_state->z_near != z_near
    || previous_state->z_far != z_far)
    glDepthRange(z_near, z_far);
  
  if (previous_state->front_face != front_face)
    glFrontFace(front_face);
  
  if (previous_state->line_width != line_width)
    glLineWidth(line_width);
  
  if (previous_state->polygon_offset_factor != polygon_offset_factor
    || previous_state->polygon_offset_units != polygon_offset_units)
    glPolygonOffset(polygon_offset_factor, polygon_offset_units);
  
  if (previous_state->sample_coverage_value != sample_coverage_value
    || previous_state->sample_coverage_invert != sample_coverage_invert)
    glSampleCoverage(sample_coverage_value, sample_coverage_invert);
  
  if (previous_state->scissor_x != scissor_x
    || previous_state->scissor_y != scissor_y
    || previous_state->scissor_width != scissor_width
    || previous_state->scissor_height != scissor_height)
    glScissor(scissor_x, scissor_y, scissor_width, scissor_height);
  
  if (previous_state->stencil_front_func != stencil_front_func
    || previous_state->stencil_front_ref != stencil_front_ref
    || previous_state->stencil_front_mask != stencil_front_mask)
    glStencilFuncSeparate(
      GL_FRONT, stencil_front_func, stencil_front_ref, stencil_front_mask);
  
  if (previous_state->stencil_back_func != stencil_back_func
    || previous_state->stencil_back_ref != stencil_back_ref
    || previous_state->stencil_back_mask != stencil_back_mask)
    glStencilFuncSeparate(
      GL_BACK, stencil_back_func, stencil_back_ref, stencil_back_mask);
  
  if (previous_state->stencil_front_writemask != stencil_front_writemask)
    glStencilMaskSeparate(GL_FRONT, stencil_front_writemask);
  
  if (previous_state->stencil_back_writemask != stencil_back_writemask)
    glStencilMaskSeparate(GL_BACK, stencil_back_writemask);
  
  if (previous_state->stencil_front_fail_op != stencil_front_fail_op
    || previous_state->stencil_front_z_fail_op != stencil_front_z_fail_op
    || previous_state->stencil_front_z_pass_op != stencil_front_z_pass_op)
    glStencilOpSeparate(
      GL_FRONT, stencil_front_fail_op, stencil_front_z_fail_op,
      stencil_front_z_pass_op);
  
  if (previous_state->stencil_back_fail_op != stencil_back_fail_op
    || previous_state->stencil_back_z_fail_op != stencil_back_z_fail_op
    || previous_state->stencil_back_z_pass_op != stencil_back_z_pass_op)
    glStencilOpSeparate(
      GL_BACK, stencil_back_fail_op, stencil_back_z_fail_op,
      stencil_back_z_pass_op);
  
  if (previous_state->viewport_x != viewport_x
    || previous_state->viewport_y != viewport_y
    || previous_state->viewport_width != viewport_width
    || previous_state->viewport_height != viewport_height)
  glViewport(viewport_x, viewport_y, viewport_width, viewport_height);
}

void ContextState::RestoreGlobalState(const ContextState* previous_state) const {
  if (!previous_state) {
    InitCapabilities();
    InitState();

    glPixelStorei(GL_PACK_ALIGNMENT, pack_alignment);
    glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);

    glHint(GL_GENERATE_MIPMAP_HINT, hint_generate_mipmap);
  } else {
    RestoreCapabilities(previous_state);
    RestoreInternalState(previous_state);
    if (previous_state->pack_alignment != pack_alignment)
      glPixelStorei(GL_PACK_ALIGNMENT, pack_alignment);

    if (previous_state->unpack_alignment != unpack_alignment)
      glPixelStorei(GL_UNPACK_ALIGNMENT, unpack_alignment);

    if (previous_state->hint_generate_mipmap != hint_generate_mipmap)
      glHint(GL_GENERATE_MIPMAP_HINT, hint_generate_mipmap);
  }

  // TODO: If OES_standard_derivatives is available
  // restore GL_FRAGMENT_SHADER_DERIVATIVE_HINT_OES
}

void ContextState::RestoreState(const ContextState* previous_state) const {
  TRACE_EVENT0("gpu", "ContextState::RestoreState");
  // Restore Texture state.
  for (size_t ii = 0; ii < texture_units.size(); ++ii) {
    RestoreTextureUnitBindings(ii, previous_state);
  }
  RestoreActiveTexture(previous_state);

  // Restore Attrib State
  // TODO: This if should not be needed. RestoreState is getting called
  // before GLES2Decoder::Initialize which is a bug.
  if (vertex_attrib_manager) {
    // TODO(gman): Move this restoration to VertexAttribManager.
    for (size_t attrib = 0; attrib < vertex_attrib_manager->num_attribs();
         ++attrib) {
      RestoreAttribute(attrib, previous_state);
    }
  }

  RestoreBufferBindings(previous_state);
  RestoreRenderbufferBindings(previous_state);
  RestoreProgramBindings(previous_state);
  RestoreGlobalState(previous_state);
}

// Include the auto-generated part of this file. We split this because it means
// we can easily edit the non-auto generated parts right here in this file
// instead of having to edit some template or the code generator.
#include "gpu/command_buffer/service/context_state_impl_autogen.h"

}  // namespace gles2
}  // namespace gpu


