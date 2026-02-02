#include "Editor/Core/EditorCore.h"
#include "Editor/Core/EditorUIState.h"
#include "Runtime/Gameplay/Animation/BonePhysicsProxyComponent.h"
#include "Runtime/ECS/Components/IDComponent.h"
#include "Runtime/Importing/FbxModel.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace Alice
{
	void EditorCore::DrawInspectorBonePhysicsProxy(World& world, const EntityId& _selectedEntity)
	{
		auto* proxy = world.GetComponent<BonePhysicsProxyComponent>(_selectedEntity);
		if (!proxy)
			return;

		if (!ImGui::CollapsingHeader("Bone Physics Proxy", ImGuiTreeNodeFlags_DefaultOpen))
			return;

		bool changed = false;

		if (ImGui::Button("Remove##BonePhysicsProxyRemove"))
		{
			world.RemoveComponent<BonePhysicsProxyComponent>(_selectedEntity);
			g_SceneDirty = true;
			return;
		}

		changed |= ImGui::Checkbox("Enabled", &proxy->enabled);

		// Owner GUID
		std::uint64_t ownerGuid = proxy->ownerGuid;
		if (ImGui::InputScalar("Owner GUID", ImGuiDataType_U64, &ownerGuid))
		{
			proxy->ownerGuid = ownerGuid;
			proxy->owner = InvalidEntityId;
			changed = true;
		}

		if (proxy->ownerGuid == 0)
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Owner GUID is 0 -> proxy will not run");

		// Owner picker
		{
			std::string preview = proxy->ownerNameDebug.empty()
				? (proxy->ownerGuid != 0 ? std::to_string(proxy->ownerGuid) : "(none)")
				: proxy->ownerNameDebug;
			if (ImGui::BeginCombo("Owner (pick entity)", preview.c_str()))
			{
				for (auto&& [eid, idc] : world.GetComponents<IDComponent>())
				{
					std::string label = world.GetEntityName(eid);
					if (label.empty()) label = "Entity " + std::to_string(eid);
					label += " (";
					label += std::to_string(idc.guid);
					label += ")";
					const bool sel = (idc.guid == proxy->ownerGuid);
					if (ImGui::Selectable(label.c_str(), sel))
					{
						proxy->ownerGuid = idc.guid;
						proxy->ownerNameDebug = world.GetEntityName(eid);
						proxy->owner = InvalidEntityId;
						changed = true;
					}
					if (sel)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}

		// Resolve owner and build bone list
		EntityId resolvedOwner = (proxy->ownerGuid != 0) ? world.FindEntityByGuid(proxy->ownerGuid) : InvalidEntityId;
		std::vector<std::string> boneNames;
		if (resolvedOwner != InvalidEntityId && m_skinnedRegistry)
		{
			if (const auto* skinned = world.GetComponent<SkinnedMeshComponent>(resolvedOwner))
			{
				if (!skinned->meshAssetPath.empty())
				{
					std::shared_ptr<SkinnedMeshGPU> mesh = m_skinnedRegistry->Find(skinned->meshAssetPath);
					if (mesh && mesh->sourceModel)
						boneNames = mesh->sourceModel->GetBoneNames();
				}
			}
		}

		int currentBoneIndex = -1;
		if (!boneNames.empty())
		{
			for (size_t i = 0; i < boneNames.size(); ++i)
			{
				if (boneNames[i] == proxy->boneName)
				{
					currentBoneIndex = static_cast<int>(i);
					break;
				}
			}
			if (currentBoneIndex < 0)
			{
				proxy->boneName = boneNames[0];
				proxy->boneIndex = 0;
				currentBoneIndex = 0;
				changed = true;
			}
		}

		// Bone: dropdown selection
		if (!boneNames.empty())
		{
			const char* preview = (currentBoneIndex >= 0 && currentBoneIndex < static_cast<int>(boneNames.size()))
				? boneNames[static_cast<size_t>(currentBoneIndex)].c_str()
				: (proxy->boneName.empty() ? "(선택)" : proxy->boneName.c_str());
			if (ImGui::BeginCombo("Bone", preview))
			{
				for (size_t i = 0; i < boneNames.size(); ++i)
				{
					const bool sel = (currentBoneIndex == static_cast<int>(i));
					if (ImGui::Selectable(boneNames[i].c_str(), sel))
					{
						proxy->boneName = boneNames[i];
						proxy->boneIndex = static_cast<int>(i);
						changed = true;
					}
					if (sel)
						ImGui::SetItemDefaultFocus();
				}
				ImGui::EndCombo();
			}
		}
		else if (resolvedOwner != InvalidEntityId)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "Owner has no bones (SkinnedMeshComponent/FBX 확인)");
			ImGui::Text("Bone Name: %s", proxy->boneName.empty() ? "(none)" : proxy->boneName.c_str());
		}
		else
		{
			ImGui::TextDisabled("Owner를 먼저 선택하면 Bone 목록에서 고를 수 있습니다.");
			ImGui::Text("Bone Name: %s", proxy->boneName.empty() ? "(none)" : proxy->boneName.c_str());
		}

		ImGui::Text("Bone Index: %d", proxy->boneIndex);

		if (changed) g_SceneDirty = true;
	}
}
