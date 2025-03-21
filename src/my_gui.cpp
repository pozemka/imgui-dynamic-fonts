#include "my_gui.h"

#include "shader.h"
#include "fonts.h"

#include <GL/gl3w.h>
#include <fmt/chrono.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <msdfgen.h>
#include <spdlog/spdlog.h>

#include <msdfgen-ext.h>

// #include <GLFW/glfw3.h> // Will drag system OpenGL headers

using namespace std::string_literals;

namespace {
    float local_font_size = 50.f;
    bool use_proc_font = true;
    bool use_shader = true;
}



void drawSystemWindow()
{
#ifdef IMGUI_HAS_VIEWPORT
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
#else
    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
#endif
    ImGui::Begin("System");
    ImGui::Checkbox("Procedural font", &use_proc_font);
    ImGui::Checkbox("Use Shader", &use_shader);
    
    ImGui::DragFloat("io.FontGlobalScale", &ImGui::GetIO().FontGlobalScale, 0.05f, 0.5f, 5.0f);
    ImGui::DragFloat("font size", &local_font_size, 1.0f, 10.0f, 100.0f);

    std::string test_text = "AVIWoq°×";
    
    ImGui::Button(fmt::format("{}##1",test_text).c_str());
    if (use_proc_font) {
        ImGui::PushFont(font_procedural);
    }
    ImGui::PushFontSize(local_font_size);
    ImGui::Button(fmt::format("{}##2",test_text).c_str());
    const float line_h = ImGui::GetTextLineHeightWithSpacing();
    ImVec2 size(ImGui::GetContentRegionAvail().x, line_h * 3.f);
    ImGui::Dummy(size);
    ImVec2      p0        = ImGui::GetItemRectMin();
    ImVec2      p1        = ImGui::GetItemRectMax();

    
    auto* draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRect(p0, p1);

    if (use_shader) {
    draw_list->AddCallback(
        [](const ImDrawList * /*cmd_list*/, const ImDrawCmd *pcmd) {
            ImDrawData *draw_data = ImGui::GetDrawData();
            float       L         = draw_data->DisplayPos.x;
            float       R         = draw_data->DisplayPos.x + draw_data->DisplaySize.x;
            float       T         = draw_data->DisplayPos.y;
            float       B         = draw_data->DisplayPos.y + draw_data->DisplaySize.y;

            const float ortho_projection[4][4] = {
                {2.0f / (R - L), 0.0f, 0.0f, 0.0f},
                {0.0f, 2.0f / (T - B), 0.0f, 0.0f},
                {0.0f, 0.0f, -1.0f, 0.0f},
                {(R + L) / (L - R), (T + B) / (B - T), 0.0f, 1.0f},
            };

            glUseProgram(g_ShaderProgram);
            glUniformMatrix4fv(glGetUniformLocation(g_ShaderProgram, "ProjMtx"),
                               1,
                               GL_FALSE,
                               &ortho_projection[0][0]);

            // Next code os borrowed from the imgui_impl_opengl3.cpp file. 
            // From the loop inside ImGui_ImplOpenGL3_RenderDrawData function

            // Will project scissor/clipping rectangles into framebuffer space
            ImVec2 clip_off = draw_data->DisplayPos; // (0,0) unless using multi-viewports
            ImVec2 clip_scale
                = draw_data->FramebufferScale; // (1,1) unless using retina display
                                               // which are often (2,2)

            // Project scissor/clipping rectangles into framebuffer space
            ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                            (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
            ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                            (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
            if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y) {
                return;
            }

            int fb_width  = (int)(draw_data->DisplaySize.x * draw_data->FramebufferScale.x);
            int fb_height = (int)(draw_data->DisplaySize.y * draw_data->FramebufferScale.y);
            if (fb_width <= 0 || fb_height <= 0)
                return;

            // Apply scissor/clipping rectangle (Y is inverted in OpenGL)
            glScissor((int)clip_min.x,
                      (int)((float)fb_height - clip_max.y),
                      (int)(clip_max.x - clip_min.x),
                      (int)(clip_max.y - clip_min.y));

            // Bind texture, Draw
            glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd->GetTexID());
#    ifdef IMGUI_IMPL_OPENGL_MAY_HAVE_VTX_OFFSET
            if (bd->GlVersion >= 320)
                GL_CALL(glDrawElementsBaseVertex(
                    GL_TRIANGLES,
                    (GLsizei)pcmd->ElemCount,
                    sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                    (void *)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)),
                    (GLint)pcmd->VtxOffset));
            else
#    endif
                glDrawElements(GL_TRIANGLES,
                               (GLsizei)pcmd->ElemCount,
                               sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                               (void *)(intptr_t)(pcmd->IdxOffset * sizeof(ImDrawIdx)));
        },
        nullptr);
    }
    
    const ImVec2 l0 = p0 + ImVec2 {20, line_h * 1};
    const ImVec2 l1 = l0 + ImVec2{1000,0};
    draw_list->AddLine(l0, l1, IM_COL32_WHITE);
    draw_list->AddLine(l0 + ImVec2 {0, ImGui::GetTextLineHeight()},
                       l1 + ImVec2 {0, ImGui::GetTextLineHeight()},
                       IM_COL32_WHITE);

    draw_list->AddText(p0 + ImVec2 {20, line_h * 1}, IM_COL32_WHITE, test_text.c_str());
    
    draw_list->PopClipRect();

    ImGui::Button(fmt::format("{}##3",test_text).c_str());

    draw_list->AddCallback(ImDrawCallback_ResetRenderState, nullptr);
    
    ImGui::PopFontSize();
    if (use_proc_font) {
        ImGui::PopFont();
    }
    ImGui::Button(fmt::format("{}##4",test_text).c_str());

    // Debug comparison with built in Freetype font loader
    static bool once = false;
    if (!once){
        auto* baked_times = font_times->GetFontBaked(50.f);

        ImFontAtlas* font_atlas = ImGui::GetIO().Fonts;
        const ImFontLoader* font_loader = font_atlas->FontLoader;
        const char debug_codepoing = 'o';
        std::string debug_codepoint_str(&debug_codepoing, 1);
        ImFontGlyph*        gyph_ft     = font_loader->FontBakedLoadGlyph(font_atlas,
                                                               font_times->Sources,
                                                               baked_times,
                                                               baked_times->FontLoaderDatas,
                                                               debug_codepoing);

        // spdlog::debug("asc: {}", baked_times->Ascent);
        spdlog::debug("Freetype '{}'({:#x}) adv: {}, x0: {}, y0: {}, x1: {}, y1: {}",
                    debug_codepoint_str,
                      (int)gyph_ft->Codepoint,
                      gyph_ft->AdvanceX,
                      gyph_ft->X0,
                      gyph_ft->Y0,
                      gyph_ft->X1,
                      gyph_ft->Y1);
        once = true;
    }
    ImGui::End();
}
