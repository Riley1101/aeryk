#include <font.h>
#include <limine.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

static bool checkStringEndsWith(const char *str, const char *end)
{
    const char *_str = str;
    const char *_end = end;

    while (*str != 0)
        str++;
    str--;

    while (*end != 0)
        end++;
    end--;

    while (true)
    {
        if (*str != *end)
            return false;

        str--;
        end--;

        if (end == _end || (str == _str && end == _end))
            return true;

        if (str == _str)
            return false;
    }

    return true;
}

static struct limine_file *getFile(const char *name, struct limine_module_response *module_response)
{
    for (size_t i = 0; i < module_response->module_count; i++)
    {
        struct limine_file *f = module_response->modules[i];
        if (checkStringEndsWith(f->path, name))
            return f;
    }

    return NULL;
}

void loadPSF1(const char *name, struct PSF1_FONT *font, struct limine_module_response *modules)
{
    struct limine_file *file = getFile(name, modules);

    if (file == NULL)
        return;

    font->psf1_header = (struct PSF1_HEADER *)file->address;
    font->glyph_buffer = (void *)((uint64_t)file->address + sizeof(struct PSF1_HEADER));
}
