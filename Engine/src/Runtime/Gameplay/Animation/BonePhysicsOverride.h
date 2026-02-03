#pragma once

#include <vector>
#include <cmath>
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

            auto proxies = world.GetComponents<BonePhysicsProxyComponent>();
            if (proxies.empty())
                return;

            const auto& boneNames = model.GetBoneNames();
            if (boneNames.empty())
                return;

            const auto& boneOffsets = model.GetBoneOffsets();
            const bool useOffsets = (model.GetCurrentAnimationType() != FbxModel::AnimationType::Rigid);

            bool paletteIsColumnMajor = false;
            for (const auto& mat : palette)
            {
                const float rowT = std::fabs(mat._41) + std::fabs(mat._42) + std::fabs(mat._43);
                const float colT = std::fabs(mat._14) + std::fabs(mat._24) + std::fabs(mat._34);
                constexpr float kEps = 1e-4f;
                if (rowT > kEps || colT > kEps)
                {
                    paletteIsColumnMajor = (rowT <= kEps && colT > kEps);
                    break;
                }
            }

            DirectX::XMMATRIX ownerWorld = DirectX::XMMatrixIdentity();
            if (world.GetComponent<TransformComponent>(owner))
                ownerWorld = world.ComputeWorldMatrix(owner);

            DirectX::XMVECTOR detOwner;
            DirectX::XMMATRIX ownerWorldInv = DirectX::XMMatrixInverse(&detOwner, ownerWorld);

            for (auto&& [proxyId, proxy] : proxies)
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
                const DirectX::XMMATRIX proxyModel = proxyWorld * ownerWorldInv;

                DirectX::XMMATRIX offset = DirectX::XMMatrixIdentity();
                bool hasOffset = (useOffsets && boneIndex < static_cast<int>(boneOffsets.size()));
                if (hasOffset)
                {
                    offset = DirectX::XMLoadFloat4x4(&boneOffsets[boneIndex]);
                }

                DirectX::XMMATRIX skinCurrent = DirectX::XMLoadFloat4x4(&palette[boneIndex]);
                if (paletteIsColumnMajor)
                    skinCurrent = DirectX::XMMatrixTranspose(skinCurrent);

                DirectX::XMMATRIX boneModelCurrent = skinCurrent;
                if (hasOffset)
                {
                    DirectX::XMVECTOR detBind;
                    DirectX::XMMATRIX bind = DirectX::XMMatrixInverse(&detBind, offset);
                    boneModelCurrent = skinCurrent * bind;
                }

                if (!proxy.hasProxyToBone || proxy.cachedBoneIndex != boneIndex)
                {
                    DirectX::XMVECTOR detProxy;
                    DirectX::XMMATRIX proxyToBone = DirectX::XMMatrixInverse(&detProxy, proxyModel) * boneModelCurrent;
                    DirectX::XMStoreFloat4x4(&proxy.proxyToBone, proxyToBone);
                    proxy.hasProxyToBone = true;
                    proxy.cachedBoneIndex = boneIndex;
                }

                DirectX::XMMATRIX proxyToBone = DirectX::XMLoadFloat4x4(&proxy.proxyToBone);
                DirectX::XMMATRIX boneModel = proxyModel * proxyToBone;

                DirectX::XMMATRIX skin = boneModel;
                if (hasOffset)
                {
                    skin = skin * offset;
                }

                if (paletteIsColumnMajor)
                    skin = DirectX::XMMatrixTranspose(skin);

                DirectX::XMStoreFloat4x4(&palette[boneIndex], skin);
            }
        }
    }
}
