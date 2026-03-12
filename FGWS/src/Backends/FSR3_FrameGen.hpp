#pragma once
#include "../include/FrameGen.hpp"

namespace framegen {
    std::unique_ptr<IFrameGenerator> CreateFSR3FrameGenerator();
}
