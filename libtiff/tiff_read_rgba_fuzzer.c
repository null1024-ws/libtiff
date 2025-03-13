#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tiffio.h>
/* safe multiply returns either the multiplied value or 0 if it overflowed */
#define __TIFFSafeMultiply(t, v, m) ((((t)(m) != (t)0) && (((t)(((v)*(m))/(m))) == (t)(v))) ? (t)((v)*(m)) : (t)0)
#define MAX_SIZE 500000000
void handle_error(const char *unused, const char *unused2, va_list unused3) {
    (void)unused;
    (void)unused2;
    (void)unused3;
    return;
}
int LLVMFuzzerTestOneInput(const uint8_t *Data, size_t Size) {
#ifndef STANDALONE
    TIFFSetErrorHandler(handle_error);
    TIFFSetWarningHandler(handle_error);
#endif
#if defined(__has_feature)
#  if __has_feature(memory_sanitizer)
    /* libjpeg-turbo has issues with MSAN and SIMD code */
    /* See https://bugs.chromium.org/p/oss-fuzz/issues/detail?id=7547 */
    /* and https://github.com/libjpeg-turbo/libjpeg-turbo/pull/365 */
    setenv("JSIMD_FORCENONE" ,"1", 1);
#  endif
#endif
    TIFF* tif = TIFFClientOpen("MemTIFF", "r", (thandle_t)Data,
        NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (!tif) {
        return 0;
    }
    uint32_t w, h;
    uint32_t* raster;
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
    TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
    /* don't continue if file size is ludicrous */
    if (TIFFTileSize64(tif) > MAX_SIZE) {
        TIFFClose(tif);
        return 0;
    }
    uint64_t bufsize = TIFFTileSize64(tif);
    /* don't continue if the buffer size greater than the max allowed by the fuzzer */
    if (bufsize > MAX_SIZE || bufsize == 0) {
        TIFFClose(tif);
        return 0;
    }
    /* another hack to work around an OOM in tif_fax3.c */
    uint32_t tilewidth = 0, tilewidth2;
    uint32_t imagewidth = 0;
    TIFFGetField(tif, TIFFTAG_TILEWIDTH, &tilewidth);
    TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &imagewidth);
    tilewidth2 = __TIFFSafeMultiply(uint32_t, tilewidth, 2);
    imagewidth = __TIFFSafeMultiply(uint32_t, imagewidth, 2);
    if (tilewidth2 * 2 > MAX_SIZE || imagewidth * 2 > MAX_SIZE || (tilewidth != 0 && tilewidth2 == 0) || imagewidth == 0) {
        TIFFClose(tif);
        return 0;
    }
    uint32_t size = __TIFFSafeMultiply(uint32_t, w, h);
    if (size > MAX_SIZE || size == 0) {
        TIFFClose(tif);
        return 0;
    }
    raster = (uint32_t*)_TIFFmalloc(size * sizeof(uint32_t));
    if (raster != NULL) {
        TIFFReadRGBAImage(tif, w, h, raster, 0);
        _TIFFfree(raster);
    }
    TIFFClose(tif);
    return 0;
}
