#pragma once

#include <cstdint>
#include <string>
#include <DirectXMath.h>

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

        // Runtime-only calibration (not serialized)
        bool hasProxyToBone = false;
        int cachedBoneIndex = -1;
        DirectX::XMFLOAT4X4 proxyToBone {
            1,0,0,0,
            0,1,0,0,
            0,0,1,0,
            0,0,0,1
        };
    };
}
