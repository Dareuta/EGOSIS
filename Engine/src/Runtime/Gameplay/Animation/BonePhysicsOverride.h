#pragma once

#include <vector>
#include <DirectXMath.h>

#include "Runtime/ECS/World.h"
#include "Runtime/ECS/Components/TransformComponent.h"
#include "Runtime/ECS/Components/IDComponent.h"
#include "Runtime/Gameplay/Animation/BonePhysicsProxyComponent.h"
#include "Runtime/Importing/FbxModel.h"

namespace Alice
{
    namespace BonePhysicsOverride
    {
        inline int ResolveBoneIndex(const FbxModel& model, const BonePhysicsProxyComponent& proxy)
        {
            if (proxy.boneIndex >= 0)
                return proxy.boneIndex;

            if (proxy.boneName.empty())
                return -1;

            const auto& names = model.GetBoneNames();
            for (size_t i = 0; i < names.size(); ++i)
            {
                if (names[i] == proxy.boneName)
                    return static_cast<int>(i);
            }

            return -1;
        }

        inline void Apply(World& world,
                          EntityId owner,
                          const FbxModel& model,
                          std::vector<DirectX::XMFLOAT4X4>& palette)
        {
            if (owner == InvalidEntityId)
                return;

            if (palette.empty())
                return;

            const auto& proxies = world.GetComponents<BonePhysicsProxyComponent>();
            if (proxies.empty())
                return;

            const auto& boneNames = model.GetBoneNames();
            if (boneNames.empty())
                return;

            const auto& boneOffsets = model.GetBoneOffsets();
            const bool useOffsets = (model.GetCurrentAnimationType() != FbxModel::AnimationType::Rigid);

            DirectX::XMMATRIX ownerWorld = DirectX::XMMatrixIdentity();
            if (world.GetComponent<TransformComponent>(owner))
                ownerWorld = world.ComputeWorldMatrix(owner);

            DirectX::XMVECTOR detOwner;
            DirectX::XMMATRIX ownerWorldInv = DirectX::XMMatrixInverse(&detOwner, ownerWorld);

            const DirectX::XMMATRIX globalInv = DirectX::XMLoadFloat4x4(&model.GetGlobalInverse());

            for (const auto& [proxyId, proxy] : proxies)
            {
                if (!proxy.enabled)
                    continue;
                if (proxy.ownerGuid != 0)
                {
                    const auto* idc = world.GetComponent<IDComponent>(owner);
                    if (!idc || idc->guid != proxy.ownerGuid)
                        continue;
                }
                else if (proxy.owner != InvalidEntityId)
                {
                    if (proxy.owner != owner)
                        continue;
                }
                else
                {
                    continue;
                }

                const int boneIndex = ResolveBoneIndex(model, proxy);
                if (boneIndex < 0)
                    continue;
                if (boneIndex >= static_cast<int>(boneNames.size()))
                    continue;
                if (boneIndex >= static_cast<int>(palette.size()))
                    continue;

                const auto* proxyTr = world.GetComponent<TransformComponent>(proxyId);
                if (!proxyTr || !proxyTr->enabled)
                    continue;

                const DirectX::XMMATRIX proxyWorld = world.ComputeWorldMatrix(proxyId);
                const DirectX::XMMATRIX boneModel = proxyWorld * ownerWorldInv;

                DirectX::XMMATRIX skin = globalInv * boneModel;
                if (useOffsets && boneIndex < static_cast<int>(boneOffsets.size()))
                {
                    const DirectX::XMMATRIX offset = DirectX::XMLoadFloat4x4(&boneOffsets[boneIndex]);
                    skin = skin * offset;
                }

                DirectX::XMStoreFloat4x4(&palette[boneIndex], skin);
            }
        }
    }
}
