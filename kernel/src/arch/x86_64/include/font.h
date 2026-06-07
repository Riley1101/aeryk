#ifndef FONT_H
#define FONT_H

struct limine_module_response;

struct PSF1_HEADER
{
    unsigned char magic[2];
    unsigned char mode;
    unsigned char charsize;
};

struct PSF1_FONT
{
    struct PSF1_HEADER *psf1_header;
    void *glyph_buffer;
};

void load_psf1(const char *name, struct PSF1_FONT *font, struct limine_module_response *modules);

#endif // FONT_H
