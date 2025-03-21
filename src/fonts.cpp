#include "fonts.h"

#include <imgui_internal.h>
#include <imgui_stdlib.h>
#include <msdfgen.h>
#include <msdfgen-ext.h>
#include <spdlog/spdlog.h>
#include <fmt/format.h>

ImFont* font_times      = nullptr;
ImFont* font_mono       = nullptr;
ImFont* font_procedural = nullptr;

ImFont* LoadProceduralFont(float scale)
{
    using namespace msdfgen;
    ImGuiIO& io = ImGui::GetIO();
    static ImFontLoader custom_loader;
    custom_loader.Name = "Procedural";
    custom_loader.FontSrcContainsGlyph = [](ImFontAtlas* atlas, ImFontConfig* src, ImWchar codepoint)
    {
        IM_UNUSED(atlas);
        IM_UNUSED(src);
        // if (codepoint >= 'A' && codepoint <= 'Z')
        //     return false;
        return true;
    };
    custom_loader.FontBakedLoadGlyph = [](ImFontAtlas*  atlas,
                                          ImFontConfig* src,
                                          ImFontBaked*  baked,
                                          void*,
                                          ImWchar codepoint) -> ImFontGlyph* {
        
        ImFontGlyph* out_glyph = nullptr;

        if (codepoint == ' ' || codepoint == '\t') 
        {
            return  out_glyph;
        }

        if (FreetypeHandle *ft = initializeFreetype()) {
            if (FontHandle *font = loadFont(ft, "C:\\Windows\\Fonts\\times.ttf")) {
                static constexpr FontCoordinateScaling FONT_SCALING = FONT_SCALING_EM_NORMALIZED;

                FontMetrics font_metrics;
                getFontMetrics(font_metrics, font, FONT_SCALING);
                font_metrics.ascenderY *= baked->Size;
                font_metrics.descenderY *= baked->Size;
                font_metrics.lineHeight *= baked->Size;
                font_metrics.underlineY *= baked->Size;
                font_metrics.underlineThickness *= baked->Size;

                baked->Ascent = IM_ROUND(font_metrics.ascenderY);  // should round?
                baked->Descent = IM_ROUND(font_metrics.descenderY);  // should round?

                Shape shape;
                double advance;
                if (loadGlyph(shape, font, codepoint, FONT_SCALING, &advance)) {
                    advance *= baked->Size;
                    // shape.inverseYAxis = true;
                    shape.normalize();

                    //Shape::Bounds bounds = shape.getBounds(0.f);
                    Shape::Bounds bounds = shape.getBounds(0.1f);

                    int rect_w = static_cast<int>((bounds.r - bounds.l) * baked->Size);
                    int rect_h = static_cast<int>((bounds.t - bounds.b) * baked->Size); //upside down

                    //                      max. angle
                    edgeColoringSimple(shape, 3.0);

// #define SYMBOLS_ONLY
                    
#ifdef SYMBOLS_ONLY
                    const int B_CHANNELS = 1; //1(SDF), 3(MSDF) or 4(MTSDF)
                    DistanceMapping dist_mapping(Range(0)); // set to 0 to remove softness. Original value is 0.125
#else
                    const int B_CHANNELS = 4; //1(SDF), 3(MSDF) or 4(MTSDF)
                    DistanceMapping dist_mapping(Range(0.125)); // set to 0 to remove softness. Original value is 0.125
#endif
                    
                    
                    
                    /// A transformation from shape coordinates to pixel coordinates.
                    msdfgen::Vector2  projection_scale(baked->Size, baked->Size);   //Scale 
                    msdfgen::Vector2  projection_offset(-bounds.l, -bounds.b);  //Translation in em's
                    Projection        projection(projection_scale, projection_offset);
                    SDFTransformation t(projection, dist_mapping);

                    Bitmap<float, B_CHANNELS> bitmap(rect_w, rect_h); //output width, height
#ifdef SYMBOLS_ONLY
                    generateSDF(bitmap, shape, t);
#else
                    // generateSDF(bitmap, shape, t);
                    // generateMSDF(bitmap, shape, t);
                    generateMTSDF(bitmap, shape, t);
#endif

                    savePng(bitmap, fmt::format("out_{}.png", codepoint).c_str());

                    ImFontAtlasRectId pack_id
                        = ImFontAtlasPackAddRect(atlas, bitmap.width(), bitmap.height());
                    ImTextureRect* r = ImFontAtlasPackGetRect(atlas, pack_id);

                    ImFontGlyph  glyph_in = {};
                    ImFontGlyph* glyph    = &glyph_in;
                    glyph->Codepoint      = codepoint;
                    glyph->AdvanceX       = IM_ROUND(static_cast<float>(advance));

                    glyph->X0             = IM_ROUND(bounds.l * baked->Size);
                    glyph->X1             = IM_ROUND(bounds.r * baked->Size);
                    glyph->Y1             = IM_ROUND(font_metrics.ascenderY - (bounds.b * baked->Size));
                    glyph->Y0             = IM_ROUND(font_metrics.ascenderY - (bounds.t * baked->Size));
                    
                    glyph->Visible        = true;
                    glyph->Colored        = true;
                    glyph->PackId         = pack_id;

                    // spdlog::debug("asc: {}", baked->Ascent);
                    // spdlog::debug("desc: {}", baked->Descent);
                    const char ascii_only_codepoint = (char)glyph->Codepoint;
                    spdlog::debug("SDF '{}'({:#x}) adv: {}, x0: {}, y0: {}, x1: {}, y1: {}",
                                  std::string(&ascii_only_codepoint, 1),
                                  (int)glyph->Codepoint,
                                  glyph->AdvanceX,
                                  glyph->X0,
                                  glyph->Y0,
                                  glyph->X1,
                                  glyph->Y1);

                    out_glyph = ImFontAtlasBakedAddFontGlyph(atlas, baked, src, &glyph_in);

                    ImTextureData* tex = atlas->TexData;
                    

                    ImTextureData im_tex_data;
                    im_tex_data.Create(ImTextureFormat::ImTextureFormat_RGBA32,
                                       bitmap.width(),
                                       bitmap.height());

                    for (int y = 0; y < bitmap.height(); ++y) {
                        for (int x = 0; x < bitmap.width(); ++x) {
                            auto           target_y = bitmap.height() - 1 - y;
                            unsigned char* pixel    = im_tex_data.GetPixelsAt(x, target_y);
                            if (B_CHANNELS == 1) {
                                pixel[0] = msdfgen::pixelFloatToByte(bitmap(x, y)[0]);
                                pixel[1] = msdfgen::pixelFloatToByte(bitmap(x, y)[0]);
                                pixel[2] = msdfgen::pixelFloatToByte(bitmap(x, y)[0]);
                            } else {
                                pixel[0] = msdfgen::pixelFloatToByte(bitmap(x, y)[0]);
                                pixel[1] = msdfgen::pixelFloatToByte(bitmap(x, y)[1]);
                                pixel[2] = msdfgen::pixelFloatToByte(bitmap(x, y)[2]);
                            }
                            pixel[3] = (B_CHANNELS == 4)
                                           ? msdfgen::pixelFloatToByte(bitmap(x, y)[3])
                                           : 0xff;
                        }
                    }

                    ImFontAtlasTextureBlockCopy(&im_tex_data,
                                                0,
                                                0,
                                                tex,
                                                r->x,
                                                r->y,
                                                r->w,
                                                r->h);
                    ImFontAtlasTextureBlockQueueUpload(atlas, tex, r->x, r->y, r->w, r->h);
                }
                destroyFont(font);
            }
            deinitializeFreetype(ft);
        }
        return out_glyph;
    };

    ImFontConfig empty_font;
    strcpy_s(empty_font.Name, "procedural");
    empty_font.SizePixels = 20.0f * scale;  //
    empty_font.FontLoader = &custom_loader;
    return io.Fonts->AddFont(&empty_font);
}