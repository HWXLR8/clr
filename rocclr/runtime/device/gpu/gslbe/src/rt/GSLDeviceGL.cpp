#include "gsl_ctx.h"
#include "GSLDevice.h"
#include "component_types.h"
#include "cwddeci.h"
#include <GL/gl.h>
#include "GL/glATIInternal.h"
#ifdef ATI_OS_LINUX
#include <stdlib.h>
#include <dlfcn.h>
#include "GL/glx.h"
#include "GL/glxext.h"
#include "GL/glXATIPrivate.h"
#else
#include "GL/wglATIPrivate.h"
#endif
#include "memory/MemObject.h"

typedef struct cmFormatXlateRec{
    cmSurfFmt   raw_cmFormat;
    cmSurfFmt   cal_cmFormat;
    gslChannelOrder channelOrder;
} cmFormatXlateParams;

// relates full range of cm surface formats to those supported by CAL
static const   cmFormatXlateParams cmFormatXlateTable [] = {
    {CM_SURF_FMT_LUMINANCE8,            CM_SURF_FMT_R8I,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_LUMINANCE16,           CM_SURF_FMT_R16,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_LUMINANCE16F,          CM_SURF_FMT_R16F,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_LUMINANCE32F,          CM_SURF_FMT_R32F,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_INTENSITY8,            CM_SURF_FMT_INTENSITY8, GSL_CHANNEL_ORDER_REPLICATE_R},
    {CM_SURF_FMT_INTENSITY16,           CM_SURF_FMT_R16,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_INTENSITY16F,          CM_SURF_FMT_R16F,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_INTENSITY32F,          CM_SURF_FMT_R32F,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_ALPHA8,                CM_SURF_FMT_R8I,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_ALPHA16,               CM_SURF_FMT_R16,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_ALPHA16F,              CM_SURF_FMT_R16F,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_ALPHA32F,              CM_SURF_FMT_R32F,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_LUMINANCE8_ALPHA8,     CM_SURF_FMT_RG8I,       GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_LUMINANCE16_ALPHA16,   CM_SURF_FMT_RG16I,      GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_LUMINANCE16F_ALPHA16F, CM_SURF_FMT_RG16F,      GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_LUMINANCE32F_ALPHA32F, CM_SURF_FMT_RG16F,      GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_B2_G3_R3,              (cmSurfFmt)500,         GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_B5_G6_R5,              CM_SURF_FMT_B5_G6_R5,   GSL_CHANNEL_ORDER_RGB},
    {CM_SURF_FMT_BGRX4,                 (cmSurfFmt)500,         GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_BGR5_X1,               CM_SURF_FMT_BGR5_X1,    GSL_CHANNEL_ORDER_RGB},
    {CM_SURF_FMT_BGRX8,                 CM_SURF_FMT_RGBA8,      GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_BGR10_X2,              CM_SURF_FMT_BGR10_X2,   GSL_CHANNEL_ORDER_RGB},
    {CM_SURF_FMT_BGRX16,                CM_SURF_FMT_RGBA16,     GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_BGRX16F,               CM_SURF_FMT_RGBA16F,    GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_BGRX32F,               CM_SURF_FMT_RGBA32F,    GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_RGBX4,                 (cmSurfFmt)500,         GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGB5_X1,               CM_SURF_FMT_BGR5_X1,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBX8,                 CM_SURF_FMT_RGBA8,      GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGB10_X2,              CM_SURF_FMT_BGR10_X2,   GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBX16,                CM_SURF_FMT_RGBA16,     GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBX16F,               CM_SURF_FMT_RGBA16F,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBX32F,               CM_SURF_FMT_RGBA32F,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_BGRA4,                 (cmSurfFmt)500,         GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_BGR5_A1,               CM_SURF_FMT_BGR5_X1,    GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_BGRA8,                 CM_SURF_FMT_RGBA8,      GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_BGR10_A2,              CM_SURF_FMT_BGR10_X2,   GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_BGRA16,                CM_SURF_FMT_RGBA16,     GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_BGRA16F,               CM_SURF_FMT_RGBA16,     GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_BGRA32F,               CM_SURF_FMT_RGBA32F,    GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_RGBA4,                 (cmSurfFmt)500,         GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGB5_A1,               CM_SURF_FMT_BGR5_X1,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBA8,                 CM_SURF_FMT_RGBA8,      GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGB10_A2,              CM_SURF_FMT_BGR10_X2,   GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBA16,                CM_SURF_FMT_RGBA16,     GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBA16F,               CM_SURF_FMT_RGBA16F,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBA32I,               CM_SURF_FMT_RGBA32I,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBA32F,               CM_SURF_FMT_RGBA32F,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_DUDV8,                 CM_SURF_FMT_RG8I,       GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_DXT1,                  (cmSurfFmt)500,         GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_DXT2_3,                (cmSurfFmt)500,         GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_DXT4_5,                (cmSurfFmt)00,          GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_ATI1N,                 (cmSurfFmt)500,         GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_ATI2N,                 (cmSurfFmt)500,         GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_DEPTH16,               CM_SURF_FMT_DEPTH16,    GSL_CHANNEL_ORDER_REPLICATE_R},
    {CM_SURF_FMT_DEPTH16F,              CM_SURF_FMT_R16F,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_DEPTH24_X8,            (cmSurfFmt)500,         GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_DEPTH24F_X8,           (cmSurfFmt)500,         GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_DEPTH24_STEN8,         CM_SURF_FMT_DEPTH24_STEN8, GSL_CHANNEL_ORDER_REPLICATE_R},
    {CM_SURF_FMT_DEPTH24F_STEN8,        (cmSurfFmt)500,         GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_DEPTH32F_X24_STEN8,    CM_SURF_FMT_RG32I,      GSL_CHANNEL_ORDER_REPLICATE_R},
    {CM_SURF_FMT_DEPTH32F,              CM_SURF_FMT_R32F,       GSL_CHANNEL_ORDER_REPLICATE_R},
    {CM_SURF_FMT_sR11_sG11_sB10,        (cmSurfFmt)500,         GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sU16,                  CM_SURF_FMT_sU16,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sUV16,                 CM_SURF_FMT_sUV16,      GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_sUVWQ16,               CM_SURF_FMT_sUVWQ16,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RG16,                  CM_SURF_FMT_RG16,       GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_RG16F,                 CM_SURF_FMT_RG16F,      GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_RG32F,                 CM_SURF_FMT_RG32F,      GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_ABGR4,                 (cmSurfFmt)500,         GSL_CHANNEL_ORDER_ARGB},
    {CM_SURF_FMT_A1_BGR5,               CM_SURF_FMT_BGR5_X1,    GSL_CHANNEL_ORDER_ARGB},
    {CM_SURF_FMT_ABGR8,                 CM_SURF_FMT_RGBA8,      GSL_CHANNEL_ORDER_ARGB},
    {CM_SURF_FMT_A2_BGR10,              CM_SURF_FMT_BGR10_X2,   GSL_CHANNEL_ORDER_ARGB},
    {CM_SURF_FMT_ABGR16,                CM_SURF_FMT_RGBA16,     GSL_CHANNEL_ORDER_ARGB},
    {CM_SURF_FMT_ABGR16F,               CM_SURF_FMT_RGBA16F,    GSL_CHANNEL_ORDER_ARGB},
    {CM_SURF_FMT_ABGR32F,               CM_SURF_FMT_RGBA32F,    GSL_CHANNEL_ORDER_ARGB},
    {CM_SURF_FMT_DXT1A,                 (cmSurfFmt)500,         GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sRGB10_A2,             (cmSurfFmt)500,         GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_sR8,                   CM_SURF_FMT_sR8,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sRG8,                  CM_SURF_FMT_sRG8,       GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_sR32I,                 CM_SURF_FMT_sR32I,      GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sRG32I,                CM_SURF_FMT_sRG32I,     GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_sRGBA32I,              CM_SURF_FMT_sRGBA32I,   GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_R32I,                  CM_SURF_FMT_R32I,       GSL_CHANNEL_ORDER_REPLICATE_R},
    {CM_SURF_FMT_RG32I,                 CM_SURF_FMT_RG32I,      GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_RG8,                   CM_SURF_FMT_RG8,        GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_sRGBA8,                CM_SURF_FMT_sRGBA8,     GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_R11F_G11F_B10F,        (cmSurfFmt)500,         GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGB9_E5,               CM_SURF_FMT_RGBA8,      GSL_CHANNEL_ORDER_ARGB},
    {CM_SURF_FMT_LUMINANCE_LATC1,       (cmSurfFmt)500,         GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_SIGNED_LUMINANCE_LATC1,(cmSurfFmt)500,         GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_LUMINANCE_ALPHA_LATC2, (cmSurfFmt)500,         GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_SIGNED_LUMINANCE_ALPHA_LATC2, (cmSurfFmt)500,  GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RED_RGTC1,             (cmSurfFmt)500,         GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_SIGNED_RED_RGTC1,      (cmSurfFmt)500,         GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RED_GREEN_RGTC2,       (cmSurfFmt)500,         GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_SIGNED_RED_GREEN_RGTC2,(cmSurfFmt)500,         GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_R8,                    CM_SURF_FMT_INTENSITY8, GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_R16,                   CM_SURF_FMT_R16,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_R16F,                  CM_SURF_FMT_R16F,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_R32F,                  CM_SURF_FMT_R32F,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_R8I,                   CM_SURF_FMT_R8I,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sR8I,                  CM_SURF_FMT_sR8I,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_RG8I,                  CM_SURF_FMT_RG8I,       GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_sRG8I,                 CM_SURF_FMT_sRG8I,      GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_R16I,                  CM_SURF_FMT_R16I,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sR16I,                 CM_SURF_FMT_sR16I,      GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_RG16I,                 CM_SURF_FMT_RG16I,      GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_sRG16I,                CM_SURF_FMT_sRG16I,     GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_RGBA32UI,              CM_SURF_FMT_RGBA32UI,   GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBX32UI,              CM_SURF_FMT_RGBA32UI,   GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_ALPHA32UI,             CM_SURF_FMT_R32I,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_INTENSITY32UI,         CM_SURF_FMT_R32I,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_LUMINANCE32UI,         CM_SURF_FMT_R32I,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_LUMINANCE_ALPHA32UI,   CM_SURF_FMT_RG32I,      GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_RGBA16UI,              CM_SURF_FMT_RGBA16UI,   GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBX16UI,              CM_SURF_FMT_RGBA16UI,   GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_ALPHA16UI,             CM_SURF_FMT_R16I,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_INTENSITY16UI,         CM_SURF_FMT_R16I,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_LUMINANCE16UI,         CM_SURF_FMT_R16I,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_LUMINANCE_ALPHA16UI,   CM_SURF_FMT_R32I,       GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_RGBA8UI,               CM_SURF_FMT_RGBA8UI,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBX8UI,               CM_SURF_FMT_RGBA8UI,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_ALPHA8UI,              CM_SURF_FMT_R8I,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_INTENSITY8UI,          CM_SURF_FMT_R8I,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_LUMINANCE8UI,          CM_SURF_FMT_R8I,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_LUMINANCE_ALPHA8UI,    CM_SURF_FMT_RG8I,       GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_sRGBA32I_EXT,          CM_SURF_FMT_sRGBA32I,   GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_sRGBX32I,              CM_SURF_FMT_sRGBA32I,   GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_sALPHA32I,             CM_SURF_FMT_sR32I,      GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sINTENSITY32I,         CM_SURF_FMT_sR32I,      GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sLUMINANCE32I,         CM_SURF_FMT_sR32I,      GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sLUMINANCE_ALPHA32I,   CM_SURF_FMT_sRG32I,     GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_sRGBA16I,              CM_SURF_FMT_sRGBA16I,   GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_sRGBX16I,              CM_SURF_FMT_sRGBA16I,   GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_sALPHA16I,             CM_SURF_FMT_sR16I,      GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sINTENSITY16I,         CM_SURF_FMT_sR16I,      GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sLUMINANCE16I,         CM_SURF_FMT_sR16I,      GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sLUMINANCE_ALPHA16I,   CM_SURF_FMT_sRG16I,     GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_sRGBA8I,               CM_SURF_FMT_sRGBA8I,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_sRGBX8I,               CM_SURF_FMT_sRGBA8I,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_sALPHA8I,              CM_SURF_FMT_sR8I,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sINTENSITY8I,          CM_SURF_FMT_sR8I,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sLUMINANCE8I,          CM_SURF_FMT_sR8I,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_sLUMINANCE_ALPHA8I,    CM_SURF_FMT_sRG8I,      GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_sDXT6,                 (cmSurfFmt)500,         GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_DXT6,                  (cmSurfFmt)500,         GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_DXT7,                  (cmSurfFmt)500,         GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_LUMINANCE8_SNORM,      CM_SURF_FMT_sR8,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_LUMINANCE16_SNORM,     CM_SURF_FMT_sU16,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_INTENSITY8_SNORM,      CM_SURF_FMT_sR8,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_INTENSITY16_SNORM,     CM_SURF_FMT_sU16,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_ALPHA8_SNORM,          CM_SURF_FMT_sR8,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_ALPHA16_SNORM,         CM_SURF_FMT_sU16,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_LUMINANCE_ALPHA8_SNORM,CM_SURF_FMT_sRG8,       GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_LUMINANCE_ALPHA16_SNORM,CM_SURF_FMT_sUV16,     GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_R8_SNORM,               CM_SURF_FMT_sR8,       GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_R16_SNORM,              CM_SURF_FMT_sU16,      GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_RG8_SNORM,              CM_SURF_FMT_sRG8,      GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_RG16_SNORM,             CM_SURF_FMT_sUV16,     GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_RGBX8_SNORM,            CM_SURF_FMT_sRGBA8,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBX16_SNORM,           CM_SURF_FMT_sUVWQ16,   GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBA8_SNORM,            CM_SURF_FMT_sRGBA8,    GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBA16_SNORM,           CM_SURF_FMT_sUVWQ16,   GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGB8_ETC2,              (cmSurfFmt)500,        GSL_CHANNEL_ORDER_RGB},
    {CM_SURF_FMT_SRGB8_ETC2,             (cmSurfFmt)500,        GSL_CHANNEL_ORDER_RGB},
    {CM_SURF_FMT_RGB8_PT_ALPHA1_ETC2,    (cmSurfFmt)500,        GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_SRGB8_PT_ALPHA1_ETC2,   (cmSurfFmt)500,        GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_RGBA8_ETC2_EAC,         (cmSurfFmt)500,        GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_SRGB8_ALPHA8_ETC2_EAC,  (cmSurfFmt)500,        GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_R11_EAC,                (cmSurfFmt)500,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_SIGNED_R11_EAC,         (cmSurfFmt)500,        GSL_CHANNEL_ORDER_R},
    {CM_SURF_FMT_RG11_EAC,               (cmSurfFmt)500,        GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_SIGNED_RG11_EAC,        (cmSurfFmt)500,        GSL_CHANNEL_ORDER_RG},
    {CM_SURF_FMT_BGR10_A2UI,             (cmSurfFmt)501,        GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_A2_BGR10UI,             (cmSurfFmt)501,        GSL_CHANNEL_ORDER_ARGB},
    {CM_SURF_FMT_A2_RGB10UI,             (cmSurfFmt)501,        GSL_CHANNEL_ORDER_ABGR},
    {CM_SURF_FMT_B5_G6_R5UI,             (cmSurfFmt)500,        GSL_CHANNEL_ORDER_BGRA},
    {CM_SURF_FMT_R5_G6_B5UI,             (cmSurfFmt)500,        GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_DEPTH32F_X24_STEN8_UNCLAMPED, CM_SURF_FMT_RG32I, GSL_CHANNEL_ORDER_REPLICATE_R},
    {CM_SURF_FMT_DEPTH32F_UNCLAMPED,    CM_SURF_FMT_R32F,       GSL_CHANNEL_ORDER_REPLICATE_R},
    {CM_SURF_FMT_L8_X16_A8_SRGB,         (cmSurfFmt)500,        GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_L8_X24_SRGB,            (cmSurfFmt)500,        GSL_CHANNEL_ORDER_RGBA},
    {CM_SURF_FMT_STENCIL8,               CM_SURF_FMT_R8I,       GSL_CHANNEL_ORDER_R},
    };

FINLINE void
dummyAssertIfCmSurfFmtChanges(void)
{
    //
    //  Assert if cmSurfFmt defined in ugl/src/include/cmndefs.h changes.
    //
    COMPILE_TIME_ASSERT(cmSurfFmt_FIRST == CM_SURF_FMT_LUMINANCE8);
    COMPILE_TIME_ASSERT(  0 == CM_SURF_FMT_LUMINANCE8);
    COMPILE_TIME_ASSERT(  1 == CM_SURF_FMT_LUMINANCE16);
    COMPILE_TIME_ASSERT(  2 == CM_SURF_FMT_LUMINANCE16F);
    COMPILE_TIME_ASSERT(  3 == CM_SURF_FMT_LUMINANCE32F);
    COMPILE_TIME_ASSERT(  4 == CM_SURF_FMT_INTENSITY8);
    COMPILE_TIME_ASSERT(  5 == CM_SURF_FMT_INTENSITY16);
    COMPILE_TIME_ASSERT(  6 == CM_SURF_FMT_INTENSITY16F);
    COMPILE_TIME_ASSERT(  7 == CM_SURF_FMT_INTENSITY32F);
    COMPILE_TIME_ASSERT(  8 == CM_SURF_FMT_ALPHA8);
    COMPILE_TIME_ASSERT(  9 == CM_SURF_FMT_ALPHA16);
    COMPILE_TIME_ASSERT( 10 == CM_SURF_FMT_ALPHA16F);
    COMPILE_TIME_ASSERT( 11 == CM_SURF_FMT_ALPHA32F);
    COMPILE_TIME_ASSERT( 12 == CM_SURF_FMT_LUMINANCE8_ALPHA8);
    COMPILE_TIME_ASSERT( 13 == CM_SURF_FMT_LUMINANCE16_ALPHA16);
    COMPILE_TIME_ASSERT( 14 == CM_SURF_FMT_LUMINANCE16F_ALPHA16F);
    COMPILE_TIME_ASSERT( 15 == CM_SURF_FMT_LUMINANCE32F_ALPHA32F);
    COMPILE_TIME_ASSERT( 16 == CM_SURF_FMT_B2_G3_R3);
    COMPILE_TIME_ASSERT( 17 == CM_SURF_FMT_B5_G6_R5);
    COMPILE_TIME_ASSERT( 18 == CM_SURF_FMT_BGRX4);
    COMPILE_TIME_ASSERT( 19 == CM_SURF_FMT_BGR5_X1);
    COMPILE_TIME_ASSERT( 20 == CM_SURF_FMT_BGRX8);
    COMPILE_TIME_ASSERT( 21 == CM_SURF_FMT_BGR10_X2);
    COMPILE_TIME_ASSERT( 22 == CM_SURF_FMT_BGRX16);
    COMPILE_TIME_ASSERT( 23 == CM_SURF_FMT_BGRX16F);
    COMPILE_TIME_ASSERT( 24 == CM_SURF_FMT_BGRX32F);
    COMPILE_TIME_ASSERT( 25 == CM_SURF_FMT_RGBX4);
    COMPILE_TIME_ASSERT( 26 == CM_SURF_FMT_RGB5_X1);
    COMPILE_TIME_ASSERT( 27 == CM_SURF_FMT_RGBX8);
    COMPILE_TIME_ASSERT( 28 == CM_SURF_FMT_RGB10_X2);
    COMPILE_TIME_ASSERT( 29 == CM_SURF_FMT_RGBX16);
    COMPILE_TIME_ASSERT( 30 == CM_SURF_FMT_RGBX16F);
    COMPILE_TIME_ASSERT( 31 == CM_SURF_FMT_RGBX32F);
    COMPILE_TIME_ASSERT( 32 == CM_SURF_FMT_BGRA4);
    COMPILE_TIME_ASSERT( 33 == CM_SURF_FMT_BGR5_A1);
    COMPILE_TIME_ASSERT( 34 == CM_SURF_FMT_BGRA8);
    COMPILE_TIME_ASSERT( 35 == CM_SURF_FMT_BGR10_A2);
    COMPILE_TIME_ASSERT( 36 == CM_SURF_FMT_BGRA16);
    COMPILE_TIME_ASSERT( 37 == CM_SURF_FMT_BGRA16F);
    COMPILE_TIME_ASSERT( 38 == CM_SURF_FMT_BGRA32F);
    COMPILE_TIME_ASSERT( 39 == CM_SURF_FMT_RGBA4);
    COMPILE_TIME_ASSERT( 40 == CM_SURF_FMT_RGB5_A1);
    COMPILE_TIME_ASSERT( 41 == CM_SURF_FMT_RGBA8);
    COMPILE_TIME_ASSERT( 42 == CM_SURF_FMT_RGB10_A2);
    COMPILE_TIME_ASSERT( 43 == CM_SURF_FMT_RGBA16);
    COMPILE_TIME_ASSERT( 44 == CM_SURF_FMT_RGBA16F);
    COMPILE_TIME_ASSERT( 45 == CM_SURF_FMT_RGBA32I);
    COMPILE_TIME_ASSERT( 46 == CM_SURF_FMT_RGBA32F);
    COMPILE_TIME_ASSERT( 47 == CM_SURF_FMT_DUDV8);
    COMPILE_TIME_ASSERT( 48 == CM_SURF_FMT_DXT1);
    COMPILE_TIME_ASSERT( 49 == CM_SURF_FMT_DXT2_3);
    COMPILE_TIME_ASSERT( 50 == CM_SURF_FMT_DXT4_5);
    COMPILE_TIME_ASSERT( 51 == CM_SURF_FMT_ATI1N);
    COMPILE_TIME_ASSERT( 52 == CM_SURF_FMT_ATI2N);
    COMPILE_TIME_ASSERT( 53 == CM_SURF_FMT_DEPTH16);
    COMPILE_TIME_ASSERT( 54 == CM_SURF_FMT_DEPTH16F);
    COMPILE_TIME_ASSERT( 55 == CM_SURF_FMT_DEPTH24_X8);
    COMPILE_TIME_ASSERT( 56 == CM_SURF_FMT_DEPTH24F_X8);
    COMPILE_TIME_ASSERT( 57 == CM_SURF_FMT_DEPTH24_STEN8);
    COMPILE_TIME_ASSERT( 58 == CM_SURF_FMT_DEPTH24F_STEN8);
    COMPILE_TIME_ASSERT( 59 == CM_SURF_FMT_DEPTH32F_X24_STEN8);
    COMPILE_TIME_ASSERT( 60 == CM_SURF_FMT_DEPTH32F);
    COMPILE_TIME_ASSERT( 61 == CM_SURF_FMT_sR11_sG11_sB10);
    COMPILE_TIME_ASSERT( 62 == CM_SURF_FMT_sU16);
    COMPILE_TIME_ASSERT( 63 == CM_SURF_FMT_sUV16);
    COMPILE_TIME_ASSERT( 64 == CM_SURF_FMT_sUVWQ16);
    COMPILE_TIME_ASSERT( 65 == CM_SURF_FMT_RG16);
    COMPILE_TIME_ASSERT( 66 == CM_SURF_FMT_RG16F);
    COMPILE_TIME_ASSERT( 67 == CM_SURF_FMT_RG32F);
    COMPILE_TIME_ASSERT( 68 == CM_SURF_FMT_ABGR4);
    COMPILE_TIME_ASSERT( 69 == CM_SURF_FMT_A1_BGR5);
    COMPILE_TIME_ASSERT( 70 == CM_SURF_FMT_ABGR8);
    COMPILE_TIME_ASSERT( 71 == CM_SURF_FMT_A2_BGR10);
    COMPILE_TIME_ASSERT( 72 == CM_SURF_FMT_ABGR16);
    COMPILE_TIME_ASSERT( 73 == CM_SURF_FMT_ABGR16F);
    COMPILE_TIME_ASSERT( 74 == CM_SURF_FMT_ABGR32F);
    COMPILE_TIME_ASSERT( 75 == CM_SURF_FMT_DXT1A);
    COMPILE_TIME_ASSERT( 76 == CM_SURF_FMT_sRGB10_A2);
    COMPILE_TIME_ASSERT( 77 == CM_SURF_FMT_sR8);
    COMPILE_TIME_ASSERT( 78 == CM_SURF_FMT_sRG8);
    COMPILE_TIME_ASSERT( 79 == CM_SURF_FMT_sR32I);
    COMPILE_TIME_ASSERT( 80 == CM_SURF_FMT_sRG32I);
    COMPILE_TIME_ASSERT( 81 == CM_SURF_FMT_sRGBA32I);
    COMPILE_TIME_ASSERT( 82 == CM_SURF_FMT_R32I);
    COMPILE_TIME_ASSERT( 83 == CM_SURF_FMT_RG32I);
    COMPILE_TIME_ASSERT( 84 == CM_SURF_FMT_RG8);
    COMPILE_TIME_ASSERT( 85 == CM_SURF_FMT_sRGBA8);
    COMPILE_TIME_ASSERT( 86 == CM_SURF_FMT_R11F_G11F_B10F);
    COMPILE_TIME_ASSERT( 87 == CM_SURF_FMT_RGB9_E5);
    COMPILE_TIME_ASSERT( 88 == CM_SURF_FMT_LUMINANCE_LATC1);
    COMPILE_TIME_ASSERT( 89 == CM_SURF_FMT_SIGNED_LUMINANCE_LATC1);
    COMPILE_TIME_ASSERT( 90 == CM_SURF_FMT_LUMINANCE_ALPHA_LATC2);
    COMPILE_TIME_ASSERT( 91 == CM_SURF_FMT_SIGNED_LUMINANCE_ALPHA_LATC2);
    COMPILE_TIME_ASSERT( 92 == CM_SURF_FMT_RED_RGTC1);
    COMPILE_TIME_ASSERT( 93 == CM_SURF_FMT_SIGNED_RED_RGTC1);
    COMPILE_TIME_ASSERT( 94 == CM_SURF_FMT_RED_GREEN_RGTC2);
    COMPILE_TIME_ASSERT( 95 == CM_SURF_FMT_SIGNED_RED_GREEN_RGTC2);
    COMPILE_TIME_ASSERT( 96 == CM_SURF_FMT_R8);
    COMPILE_TIME_ASSERT( 97 == CM_SURF_FMT_R16);
    COMPILE_TIME_ASSERT( 98 == CM_SURF_FMT_R16F);
    COMPILE_TIME_ASSERT( 99 == CM_SURF_FMT_R32F);
    COMPILE_TIME_ASSERT(100 == CM_SURF_FMT_R8I);
    COMPILE_TIME_ASSERT(101 == CM_SURF_FMT_sR8I);
    COMPILE_TIME_ASSERT(102 == CM_SURF_FMT_RG8I);
    COMPILE_TIME_ASSERT(103 == CM_SURF_FMT_sRG8I);
    COMPILE_TIME_ASSERT(104 == CM_SURF_FMT_R16I);
    COMPILE_TIME_ASSERT(105 == CM_SURF_FMT_sR16I);
    COMPILE_TIME_ASSERT(106 == CM_SURF_FMT_RG16I);
    COMPILE_TIME_ASSERT(107 == CM_SURF_FMT_sRG16I);
    COMPILE_TIME_ASSERT(108 == CM_SURF_FMT_RGBA32UI);
    COMPILE_TIME_ASSERT(109 == CM_SURF_FMT_RGBX32UI);
    COMPILE_TIME_ASSERT(110 == CM_SURF_FMT_ALPHA32UI);
    COMPILE_TIME_ASSERT(111 == CM_SURF_FMT_INTENSITY32UI);
    COMPILE_TIME_ASSERT(112 == CM_SURF_FMT_LUMINANCE32UI);
    COMPILE_TIME_ASSERT(113 == CM_SURF_FMT_LUMINANCE_ALPHA32UI);
    COMPILE_TIME_ASSERT(114 == CM_SURF_FMT_RGBA16UI);
    COMPILE_TIME_ASSERT(115 == CM_SURF_FMT_RGBX16UI);
    COMPILE_TIME_ASSERT(116 == CM_SURF_FMT_ALPHA16UI);
    COMPILE_TIME_ASSERT(117 == CM_SURF_FMT_INTENSITY16UI);
    COMPILE_TIME_ASSERT(118 == CM_SURF_FMT_LUMINANCE16UI);
    COMPILE_TIME_ASSERT(119 == CM_SURF_FMT_LUMINANCE_ALPHA16UI);
    COMPILE_TIME_ASSERT(120 == CM_SURF_FMT_RGBA8UI);
    COMPILE_TIME_ASSERT(121 == CM_SURF_FMT_RGBX8UI);
    COMPILE_TIME_ASSERT(122 == CM_SURF_FMT_ALPHA8UI);
    COMPILE_TIME_ASSERT(123 == CM_SURF_FMT_INTENSITY8UI);
    COMPILE_TIME_ASSERT(124 == CM_SURF_FMT_LUMINANCE8UI);
    COMPILE_TIME_ASSERT(125 == CM_SURF_FMT_LUMINANCE_ALPHA8UI);
    COMPILE_TIME_ASSERT(126 == CM_SURF_FMT_sRGBA32I_EXT);
    COMPILE_TIME_ASSERT(127 == CM_SURF_FMT_sRGBX32I);
    COMPILE_TIME_ASSERT(128 == CM_SURF_FMT_sALPHA32I);
    COMPILE_TIME_ASSERT(129 == CM_SURF_FMT_sINTENSITY32I);
    COMPILE_TIME_ASSERT(130 == CM_SURF_FMT_sLUMINANCE32I);
    COMPILE_TIME_ASSERT(131 == CM_SURF_FMT_sLUMINANCE_ALPHA32I);
    COMPILE_TIME_ASSERT(132 == CM_SURF_FMT_sRGBA16I);
    COMPILE_TIME_ASSERT(133 == CM_SURF_FMT_sRGBX16I);
    COMPILE_TIME_ASSERT(134 == CM_SURF_FMT_sALPHA16I);
    COMPILE_TIME_ASSERT(135 == CM_SURF_FMT_sINTENSITY16I);
    COMPILE_TIME_ASSERT(136 == CM_SURF_FMT_sLUMINANCE16I);
    COMPILE_TIME_ASSERT(137 == CM_SURF_FMT_sLUMINANCE_ALPHA16I);
    COMPILE_TIME_ASSERT(138 == CM_SURF_FMT_sRGBA8I);
    COMPILE_TIME_ASSERT(139 == CM_SURF_FMT_sRGBX8I);
    COMPILE_TIME_ASSERT(140 == CM_SURF_FMT_sALPHA8I);
    COMPILE_TIME_ASSERT(141 == CM_SURF_FMT_sINTENSITY8I);
    COMPILE_TIME_ASSERT(142 == CM_SURF_FMT_sLUMINANCE8I);
    COMPILE_TIME_ASSERT(143 == CM_SURF_FMT_sLUMINANCE_ALPHA8I);
    COMPILE_TIME_ASSERT(144 == CM_SURF_FMT_sDXT6);
    COMPILE_TIME_ASSERT(145 == CM_SURF_FMT_DXT6);
    COMPILE_TIME_ASSERT(146 == CM_SURF_FMT_DXT7);
    COMPILE_TIME_ASSERT(147 == CM_SURF_FMT_LUMINANCE8_SNORM);
    COMPILE_TIME_ASSERT(148 == CM_SURF_FMT_LUMINANCE16_SNORM);
    COMPILE_TIME_ASSERT(149 == CM_SURF_FMT_INTENSITY8_SNORM);
    COMPILE_TIME_ASSERT(150 == CM_SURF_FMT_INTENSITY16_SNORM);
    COMPILE_TIME_ASSERT(151 == CM_SURF_FMT_ALPHA8_SNORM);
    COMPILE_TIME_ASSERT(152 == CM_SURF_FMT_ALPHA16_SNORM);
    COMPILE_TIME_ASSERT(153 == CM_SURF_FMT_LUMINANCE_ALPHA8_SNORM);
    COMPILE_TIME_ASSERT(154 == CM_SURF_FMT_LUMINANCE_ALPHA16_SNORM);
    COMPILE_TIME_ASSERT(155 == CM_SURF_FMT_R8_SNORM);
    COMPILE_TIME_ASSERT(156 == CM_SURF_FMT_R16_SNORM);
    COMPILE_TIME_ASSERT(157 == CM_SURF_FMT_RG8_SNORM);
    COMPILE_TIME_ASSERT(158 == CM_SURF_FMT_RG16_SNORM);
    COMPILE_TIME_ASSERT(159 == CM_SURF_FMT_RGBX8_SNORM);
    COMPILE_TIME_ASSERT(160 == CM_SURF_FMT_RGBX16_SNORM);
    COMPILE_TIME_ASSERT(161 == CM_SURF_FMT_RGBA8_SNORM);
    COMPILE_TIME_ASSERT(162 == CM_SURF_FMT_RGBA16_SNORM);
    COMPILE_TIME_ASSERT(163 == CM_SURF_FMT_RGB10_A2UI);
    COMPILE_TIME_ASSERT(164 == CM_SURF_FMT_RGB32F);
    COMPILE_TIME_ASSERT(165 == CM_SURF_FMT_RGB32I);
    COMPILE_TIME_ASSERT(166 == CM_SURF_FMT_RGB32UI);
    COMPILE_TIME_ASSERT(167 == CM_SURF_FMT_RGBX8_SRGB);
    COMPILE_TIME_ASSERT(168 == CM_SURF_FMT_RGBA8_SRGB);
    COMPILE_TIME_ASSERT(169 == CM_SURF_FMT_DXT1_SRGB);
    COMPILE_TIME_ASSERT(170 == CM_SURF_FMT_DXT1A_SRGB);
    COMPILE_TIME_ASSERT(171 == CM_SURF_FMT_DXT2_3_SRGB);
    COMPILE_TIME_ASSERT(172 == CM_SURF_FMT_DXT4_5_SRGB);
    COMPILE_TIME_ASSERT(173 == CM_SURF_FMT_DXT7_SRGB);
    COMPILE_TIME_ASSERT(174 == CM_SURF_FMT_RGB8_ETC2);
    COMPILE_TIME_ASSERT(175 == CM_SURF_FMT_SRGB8_ETC2);
    COMPILE_TIME_ASSERT(176 == CM_SURF_FMT_RGB8_PT_ALPHA1_ETC2);
    COMPILE_TIME_ASSERT(177 == CM_SURF_FMT_SRGB8_PT_ALPHA1_ETC2);
    COMPILE_TIME_ASSERT(178 == CM_SURF_FMT_RGBA8_ETC2_EAC);
    COMPILE_TIME_ASSERT(179 == CM_SURF_FMT_SRGB8_ALPHA8_ETC2_EAC);
    COMPILE_TIME_ASSERT(180 == CM_SURF_FMT_R11_EAC);
    COMPILE_TIME_ASSERT(181 == CM_SURF_FMT_SIGNED_R11_EAC);
    COMPILE_TIME_ASSERT(182 == CM_SURF_FMT_RG11_EAC);
    COMPILE_TIME_ASSERT(183 == CM_SURF_FMT_SIGNED_RG11_EAC);
    COMPILE_TIME_ASSERT(184 == CM_SURF_FMT_BGR10_A2UI);
    COMPILE_TIME_ASSERT(185 == CM_SURF_FMT_A2_BGR10UI);
    COMPILE_TIME_ASSERT(186 == CM_SURF_FMT_A2_RGB10UI);
    COMPILE_TIME_ASSERT(187 == CM_SURF_FMT_B5_G6_R5UI);
    COMPILE_TIME_ASSERT(188 == CM_SURF_FMT_R5_G6_B5UI);
    COMPILE_TIME_ASSERT(189 == CM_SURF_FMT_DEPTH32F_X24_STEN8_UNCLAMPED);
    COMPILE_TIME_ASSERT(190 == CM_SURF_FMT_DEPTH32F_UNCLAMPED);
    COMPILE_TIME_ASSERT(191 == CM_SURF_FMT_L8_X16_A8_SRGB);
    COMPILE_TIME_ASSERT(192 == CM_SURF_FMT_L8_X24_SRGB);
    COMPILE_TIME_ASSERT(193 == CM_SURF_FMT_STENCIL8);

    COMPILE_TIME_ASSERT(cmSurfFmt_LAST  == CM_SURF_FMT_STENCIL8);
    COMPILE_TIME_ASSERT(cmSurfFmt_LAST < 501);
}

#ifdef ATI_OS_LINUX
typedef void* (*PFNGlxGetProcAddress)(const GLubyte* procName);
static PFNGlxGetProcAddress    pfnGlxGetProcAddress=NULL;
static PFNGLXBEGINCLINTEROPAMD glXBeginCLInteropAMD = NULL;
static PFNGLXENDCLINTEROPAMD glXEndCLInteropAMD = NULL;
static PFNGLXRESOURCEATTACHAMD glXResourceAttachAMD = NULL;
static PFNGLXRESOURCEDETACHAMD glxResourceAcquireAMD = NULL;
static PFNGLXRESOURCEDETACHAMD glxResourceReleaseAMD = NULL;
static PFNGLXRESOURCEDETACHAMD glXResourceDetachAMD = NULL;
static PFNGLXGETCONTEXTMVPUINFOAMD glXGetContextMVPUInfoAMD = NULL;
#else
static PFNWGLBEGINCLINTEROPAMD wglBeginCLInteropAMD = NULL;
static PFNWGLENDCLINTEROPAMD wglEndCLInteropAMD = NULL;
static PFNWGLRESOURCEATTACHAMD wglResourceAttachAMD = NULL;
static PFNWGLRESOURCEDETACHAMD wglResourceAcquireAMD = NULL;
static PFNWGLRESOURCEDETACHAMD wglResourceReleaseAMD = NULL;
static PFNWGLRESOURCEDETACHAMD wglResourceDetachAMD = NULL;
static PFNWGLGETCONTEXTGPUINFOAMD wglGetContextGPUInfoAMD = NULL;
#endif

bool
CALGSLDevice::initGLInteropPrivateExt(CALvoid* GLplatformContext, CALvoid* GLdeviceContext) const
{
#ifdef ATI_OS_LINUX
    GLXContext ctx = (GLXContext)GLplatformContext;
    void * pModule = dlopen("libGL.so.1",RTLD_NOW);

    if(NULL == pModule){
        return false;
    }
    pfnGlxGetProcAddress = (PFNGlxGetProcAddress) dlsym(pModule,"glXGetProcAddress");

    if (NULL == pfnGlxGetProcAddress){
        return false;
    }

    if (!glXBeginCLInteropAMD || !glXEndCLInteropAMD || !glXResourceAttachAMD ||
        !glXResourceDetachAMD || !glXGetContextMVPUInfoAMD)
    {
        glXBeginCLInteropAMD = (PFNGLXBEGINCLINTEROPAMD) pfnGlxGetProcAddress ((const GLubyte *)"glXBeginCLInteroperabilityAMD");
        glXEndCLInteropAMD = (PFNGLXENDCLINTEROPAMD) pfnGlxGetProcAddress ((const GLubyte *)"glXEndCLInteroperabilityAMD");
        glXResourceAttachAMD = (PFNGLXRESOURCEATTACHAMD) pfnGlxGetProcAddress ((const GLubyte *)"glXResourceAttachAMD");
        glxResourceAcquireAMD = (PFNGLXRESOURCEDETACHAMD) pfnGlxGetProcAddress ((const GLubyte *)"glXResourceAcquireAMD");
        glxResourceReleaseAMD = (PFNGLXRESOURCEDETACHAMD) pfnGlxGetProcAddress ((const GLubyte *)"glXResourceReleaseAMD");
        glXResourceDetachAMD = (PFNGLXRESOURCEDETACHAMD) pfnGlxGetProcAddress ((const GLubyte *)"glXResourceDetachAMD");
        glXGetContextMVPUInfoAMD = (PFNGLXGETCONTEXTMVPUINFOAMD) pfnGlxGetProcAddress ((const GLubyte *)"glXGetContextMVPUInfoAMD");
    }

    if (!glXBeginCLInteropAMD || !glXEndCLInteropAMD || !glXResourceAttachAMD ||
        !glXResourceDetachAMD || !glXGetContextMVPUInfoAMD)
    {
        return false;
    }
#else
    if (!wglBeginCLInteropAMD || !wglEndCLInteropAMD || !wglResourceAttachAMD ||
        !wglResourceDetachAMD || !wglGetContextGPUInfoAMD)
    {
        HGLRC fakeRC = NULL;

        if (!wglGetCurrentContext())
        {
            fakeRC = wglCreateContext((HDC)GLdeviceContext);
            wglMakeCurrent((HDC)GLdeviceContext, fakeRC);
        }

        wglBeginCLInteropAMD = (PFNWGLBEGINCLINTEROPAMD) wglGetProcAddress ("wglBeginCLInteroperabilityAMD");
        wglEndCLInteropAMD = (PFNWGLENDCLINTEROPAMD) wglGetProcAddress ("wglEndCLInteroperabilityAMD");
        wglResourceAttachAMD = (PFNWGLRESOURCEATTACHAMD) wglGetProcAddress ("wglResourceAttachAMD");
        wglResourceAcquireAMD = (PFNWGLRESOURCEDETACHAMD) wglGetProcAddress ("wglResourceAcquireAMD");
        wglResourceReleaseAMD = (PFNWGLRESOURCEDETACHAMD) wglGetProcAddress ("wglResourceReleaseAMD");
        wglResourceDetachAMD = (PFNWGLRESOURCEDETACHAMD) wglGetProcAddress ("wglResourceDetachAMD");
        wglGetContextGPUInfoAMD = (PFNWGLGETCONTEXTGPUINFOAMD) wglGetProcAddress ("wglGetContextGPUInfoAMD");

        if (fakeRC)
        {
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(fakeRC);
        }
    }
    if (!wglBeginCLInteropAMD || !wglEndCLInteropAMD || !wglResourceAttachAMD ||
        !wglResourceDetachAMD || !wglGetContextGPUInfoAMD)
    {
        return false;
    }
#endif
    return true;
}

bool
CALGSLDevice::glCanInterop(CALvoid* GLplatformContext, CALvoid* GLdeviceContext)
{
    bool canInteroperate = false;

#ifdef ATI_OS_WIN
    LUID glAdapterLuid = {0, 0};
    UINT glChainBitMask = 0;

    LUID calAdapterLuid = {0, 0};
    UINT calChainBitMask = 0;

    HGLRC hRC = (HGLRC)GLplatformContext;

    //get GL context's LUID and chainBitMask from UGL
    if (wglGetContextGPUInfoAMD(hRC, &glAdapterLuid, &glChainBitMask))
    {
        //now check against the CAL device' LUID and chainBitMask.
        if (m_adp->getMVPUinfo(&calAdapterLuid, &calChainBitMask))
        {
            canInteroperate = ((glAdapterLuid.HighPart == calAdapterLuid.HighPart) &&
                               (glAdapterLuid.LowPart == calAdapterLuid.LowPart) &&
                               (glChainBitMask == calChainBitMask));
        }
    }
#elif defined (ATI_OS_LINUX)
    GLuint glDeviceId = 0 ;
    GLuint glChainMask = 0 ;
    GLXContext ctx = (GLXContext)GLplatformContext;
    if (glXGetContextMVPUInfoAMD(ctx,&glDeviceId,&glChainMask)){
        GLuint deviceId = 0 ;
        GLuint chainMask = 0 ;

        if (m_adp->getMVPUinfo(&deviceId, &chainMask))
        {
        // we allow intoperability only with GL context
        // reside on a single GPU
            if (deviceId == glDeviceId && chainMask == glChainMask){
                    canInteroperate = true;
            }
        }
    }
#endif
    return canInteroperate;
}

bool
CALGSLDevice::glAssociate(CALvoid* GLplatformContext, CALvoid* GLdeviceContext)
{
    //initialize pointers to the gl extension that supports interoperability
    if (!initGLInteropPrivateExt(GLplatformContext, GLdeviceContext) ||
        !glCanInterop(GLplatformContext, GLdeviceContext))
    {
        return false;
    }

#ifdef ATI_OS_LINUX
    GLXContext ctx = (GLXContext)GLplatformContext;
    return (glXBeginCLInteropAMD(ctx, 0)) ? true : false;
#else
    HGLRC hRC = (HGLRC)GLplatformContext;
    return (wglBeginCLInteropAMD(hRC, 0)) ? true : false;
#endif
}

bool
CALGSLDevice::glDissociate(CALvoid* GLplatformContext, CALvoid* GLdeviceContext)
{
#ifdef ATI_OS_LINUX
    GLXContext ctx = (GLXContext)GLplatformContext;
    return (glXEndCLInteropAMD(ctx, 0)) ? true : false;
#else
    HGLRC hRC = (HGLRC)GLplatformContext;
    return (wglEndCLInteropAMD(hRC, 0)) ? true : false;
#endif
}

bool
CALGSLDevice::resGLAssociate(GLResAssociate & resData) const
{
    //! @note: GSL device isn't thread safe
    amd::ScopedLock k(gslDeviceOps());

    GLResource hRes = {0};
    bool status = false;
    cmSurfFmt cal_cmFormat;
    uint32 depth;

    gslMemObjectAttribs attribs(
        GSL_MOA_TEXTURE_2D,      // type
        GSL_MOA_MEMORY_ALIAS,    // location
        GSL_MOA_TILING_TILED,    // tiling
        GSL_MOA_DISPLAYABLE_NO,  // displayable
        ATIGL_FALSE,             // mipmap
        1,                       // samples
        0,                       // cpu_address
        GSL_MOA_SIGNED_NO,       // signed_format
        GSL_MOA_FORMAT_DERIVED,  // numFormat
        DRIVER_MODULE_GLL,       // module
        GSL_ALLOCATION_INSTANCED // alloc_type
    );

    hRes.type = resData.type;

    GLResourceData* hData = new GLResourceData;
    if (NULL == hData)
    {
        return false;
    }
    memset(hData, 0, sizeof(GLResourceData));

    hRes.name = resData.name;
    hRes.flags = resData.flags;
    hData->version = GL_RESOURCE_DATA_VERSION;

#ifdef ATI_OS_LINUX
    GLXContext ctx = (GLXContext)resData.GLContext;
    if (glXResourceAttachAMD(ctx, &hRes, hData))
    {
        attribs.dynamicSharedBufferID = hData->sharedBufferID;
        status = true;
    }
#else
    HGLRC hRC = (HGLRC)resData.GLContext;
    if (wglResourceAttachAMD(hRC, &hRes, hData))
    {
        status =  true;
    }
#endif

    if (!status)
    {
        return false;
    }

    // for now, to be safe, allow only textures to have a depth other than 1
    if (hRes.type == GL_RESOURCE_ATTACH_TEXTURE_AMD)
    {
        depth = hData->rawDimensions.depth;
    }
    else
    {
        depth = 1;
    }

    attribs.type = static_cast<gslMemObjectAttribType>(hData->objectAttribType);

    osAssert(depth <= GLRDATA_MAX_LAYERS);
    osAssert(depth >= 1);
    attribs.alias_swizzles = (uint32*)malloc(depth * 2 * sizeof(uint32));
    osAssert(attribs.alias_swizzles);
    memcpy (attribs.alias_swizzles, hData->swizzles, sizeof(uint32) * depth);
    if (hData->levels > 1)
    {
        attribs.mipmap = ATIGL_TRUE;
        attribs.levels = static_cast<GLuint>(hData->levels);
        memcpy (&attribs.alias_swizzles[depth], hData->swizzlesMip, sizeof(uint32) * depth);
    }

    attribs.cpu_address = (void*)hData->handle;
    attribs.alias_subtile = hData->tilingMode;
    attribs.mcaddress = hData->cardAddr;
    // VBOs are hardcoded to have a UINT8 type format
    if (hRes.type == GL_RESOURCE_ATTACH_VERTEXBUFFER_AMD)
    {
        hData->format = CM_SURF_FMT_LUMINANCE8;
    }
    // CAL supports only a limited number of cm_surf formats, so we
    // have to translate incoming cm_surf formats
    uint32 index = hData->format - (uint32)CM_SURF_FMT_LUMINANCE8;
    if (index >= sizeof(cmFormatXlateTable)/sizeof(cmFormatXlateParams))
    {
        free(attribs.alias_swizzles);
        delete hData;
        return false;
    }
    osAssert(static_cast<cmSurfFmt>(hData->format) == cmFormatXlateTable[index].raw_cmFormat);
    cal_cmFormat = cmFormatXlateTable[index].cal_cmFormat;
    if (cal_cmFormat == 500)
    {
        free(attribs.alias_swizzles);
        delete hData;
        return false;  // format is not supported by CAL
    }
    attribs.channelOrder = cmFormatXlateTable[index].channelOrder;
    attribs.alias_perSurfTileInfo = hData->perSurfTileInfo;
    attribs.alias_GLInterop = ATIGL_TRUE;
    attribs.numFormat = GSL_MOA_FORMAT_DERIVED;

    gslMemObject    mem;

    if (hData->offset != 0)
    {
        osAssert((hData->rawDimensions.height == 1) && (depth == 1));
        mem = m_cs->createMemObject2D(CM_SURF_FMT_LUMINANCE8, hData->surfaceSize, 1, &attribs);
    }
    else
    {
        mem = m_cs->createMemObject3D(cal_cmFormat, hData->paddedDimensions.width,
            hData->rawDimensions.height, depth, &attribs);
    }
    if (hRes.type == GL_RESOURCE_ATTACH_VERTEXBUFFER_AMD)
    {
        attribs.tiling = mem->getAttribs().tiling;
        resData.mem_base = mem;
        mem  = m_cs->createOffsetMemObject2D(resData.mem_base, (static_cast<uintp>(hData->offset)),
                                                                cal_cmFormat,
                                                                hData->paddedDimensions.width,
                                                                1, &attribs);
    }
    else if ((hData->offset != 0) && (hData->rawDimensions.height == 1) && (depth == 1))
    {
        resData.mem_base = mem;
        attribs.tiling = mem->getAttribs().tiling;
        mem = m_cs->createOffsetMemObject3D(resData.mem_base, (static_cast<uintp>(hData->offset)),
            cal_cmFormat, hData->paddedDimensions.width,
            hData->rawDimensions.height, depth, &attribs);
    }
    free (attribs.alias_swizzles);
    resData.mbResHandle = (CALvoid*)hData->mbResHandle;
    resData.memObject = mem;
    delete hData;
    return mem != 0;
}

bool
CALGSLDevice::resGLAcquire(CALvoid* GLplatformContext,
    CALvoid* mbResHandle,
    CALuint type) const
{
    //! @note: GSL device isn't thread safe
    amd::ScopedLock k(gslDeviceOps());

    GLResource hRes = {0};
    osAssert(mbResHandle);
    hRes.mbResHandle = (GLuintp)mbResHandle;
    hRes.type = type;

#ifdef ATI_OS_LINUX
    GLXContext ctx = (GLXContext) GLplatformContext;
    return (glxResourceAcquireAMD(ctx, &hRes)) ? true : false;
#else
    HGLRC hRC = wglGetCurrentContext();
    //! @todo A temporary workaround for MT issue in conformance fence_sync
    if (0 == hRC) {
        return true;
    }
    return (wglResourceAcquireAMD(hRC, &hRes)) ? true : false;
#endif
}

bool
CALGSLDevice::resGLRelease(CALvoid* GLplatformContext,
    CALvoid* mbResHandle,
    CALuint type) const
{
    //! @note: GSL device isn't thread safe
    amd::ScopedLock k(gslDeviceOps());

    GLResource hRes = {0};
    osAssert(mbResHandle);
    hRes.mbResHandle = (GLuintp)mbResHandle;
    hRes.type = type;

#ifdef ATI_OS_LINUX
    //TODO : make sure the application GL context is current. if not no
    // point calling into the GL RT.
    GLXContext ctx = (GLXContext) GLplatformContext;
    return (glxResourceReleaseAMD(ctx, &hRes)) ? true : false;
#else
    // Make the call into the GL driver only if the application GL context is current
    HGLRC hRC = wglGetCurrentContext();
    //! @todo A temporary workaround for MT issue in conformance fence_sync
    if (0 == hRC) {
        return true;
    }
    return (wglResourceReleaseAMD(hRC, &hRes)) ? true : false;
#endif
}

bool
CALGSLDevice::resGLFree (
    CALvoid* GLplatformContext,
    CALvoid* GLdeviceContext,
    gslMemObject mem,
    gslMemObject mem_base,
    CALvoid* mbResHandle,
    CALuint type) const
{
    //! @note: GSL device isn't thread safe
    amd::ScopedLock k(gslDeviceOps());

    GLResource hRes = {0};

    osAssert(mbResHandle);
    hRes.mbResHandle = (GLuintp)mbResHandle;
    hRes.type = type;

    if (mem_base)
    {
        m_cs->destroyMemObject(mem_base);
    }
    m_cs->destroyMemObject(mem);

#ifdef ATI_OS_LINUX
    GLXContext ctx = (GLXContext)GLplatformContext;
    return (glXResourceDetachAMD(ctx, &hRes)) ? true : false;
#else
    HGLRC hRC = (HGLRC)GLplatformContext;
    return (wglResourceDetachAMD(hRC, &hRes)) ? true : false;
#endif
};
