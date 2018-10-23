#include "gen.h"
#include "vs.h"
#include "vsc.h"
#include "xcode.h"

namespace iris
{
    std::unique_ptr<generator> get_generator(proj_gen_type type)
    {
        switch (type)
        {
        case proj_gen_type::visual_studio:
            return std::make_unique<vs_gen>();
        case proj_gen_type::visual_studio_code:
            return std::make_unique<vsc_gen>();
        case proj_gen_type::xcode:
            return std::make_unique<xcode_gen>();
        }
        return nullptr;
    }
}