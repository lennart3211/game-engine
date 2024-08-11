#ifndef ENTT_CONFIG_VERSION_H
#define ENTT_CONFIG_VERSION_H

#include "macro.h"

#define ENTT_VERSION_MAJOR 3
#define ENTT_VERSION_MINOR 14
#define ENTT_VERSION_PATCH 0

#define ENTT_VERSION \
    ENTT_XSTR(ENTT_VERSION_MAJOR) \
    "." ENTT_XSTR(ENTT_VERSION_MINOR) "." ENTT_XSTR(ENTT_VERSION_PATCH)

#endif
