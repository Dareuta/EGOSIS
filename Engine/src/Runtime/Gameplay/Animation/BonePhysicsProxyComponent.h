#pragma once

#include <cstdint>
#include <string>

#include "Runtime/ECS/Entity.h"

namespace Alice
{
    // Leaf-bone physics proxy: palette override uses this entity's Transform.
    struct BonePhysicsProxyComponent
    {
        bool enabled = true;

        // Authoring-friendly owner reference (scene serialization)
        std::uint64_t ownerGuid = 0;
        std::string ownerNameDebug;

        // Runtime override (not serialized)
        EntityId owner = InvalidEntityId;

        std::string boneName;
        int boneIndex = -1;
    };
}
