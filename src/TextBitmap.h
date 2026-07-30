/*
 * TextBitmap.h — Simple bitmap font renderer for wasmcart
 * Subclass of Text, replaces TextFTGL/TextGLC
 */
#ifndef TextBitmap_h
#define TextBitmap_h

#include "Text.h"

class TextBitmap : public Text
{
public:
    TextBitmap();
    virtual ~TextBitmap();

    virtual void  Render(const char* str, const int len = -1);
    virtual float Advance(const char* str, const int len = -1);
    virtual float LineHeight(const char* str = " ", const int len = -1);

private:
    void initFont();
    unsigned int fontTex;
    int initialized;
    float charWidth;
    float charHeight;
};

#endif // TextBitmap_h
