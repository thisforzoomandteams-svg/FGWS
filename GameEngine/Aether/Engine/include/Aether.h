#pragma once

// Convenience include for public engine headers.

#include "Core/Application.h"
#include "Core/Window.h"

// Compatibility: new code should prefer `ge2::` APIs. For now `ge2::` is an alias
// of the existing `Aether::` namespace so we can migrate incrementally.
#include "ge2/Namespace.h"
