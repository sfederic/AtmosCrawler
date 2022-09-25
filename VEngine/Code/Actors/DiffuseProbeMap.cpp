#include "vpch.h"
#include "DiffuseProbeMap.h"
#include "Components/InstanceMeshComponent.h"
#include "Debug.h"

DiffuseProbeMap::DiffuseProbeMap()
{
	//Set mesh count as 1
	instanceMeshComponent = InstanceMeshComponent::system.Add(
		"InstanceMesh",
		this,
		InstanceMeshComponent(1, "cube.fbx", "test.png", ShaderItems::Instance));
	rootComponent = instanceMeshComponent;
}

void DiffuseProbeMap::Create()
{
	instanceMeshComponent->SetInstanceCount(GetProbeCount());
	SetInstanceMeshData();
}

Properties DiffuseProbeMap::GetProps()
{
	auto props = __super::GetProps();
	props.title = "DiffuseProbeMap";
	props.AddProp(sizeX);
	props.AddProp(sizeY);
	props.AddProp(sizeZ);
	return props;
}

void DiffuseProbeMap::SetInstanceMeshData()
{
	std::vector<InstanceData> instanceData;

	int probeIndex = 0;

	XMFLOAT3 pos = GetPosition();

	for (int x = pos.x; x < (sizeX + pos.x); x++)
	{
		for (int y = pos.y; y < (sizeY + pos.y); y++)
		{
			for (int z = pos.z; z < (sizeZ + pos.z); z++)
			{
				InstanceData data = {};
				data.world = XMMatrixTranslation((float)x, (float)y, (float)z);
				
				data.world.r[0].m128_f32[0] = 0.15f;
				data.world.r[1].m128_f32[1] = 0.15f;
				data.world.r[2].m128_f32[2] = 0.15f;

				instanceData.push_back(data);

				ProbeData pd = {};
				pd.index = probeIndex;
				XMStoreFloat3(&pd.position,data.world.r[3]);
				probeData.push_back(pd);

				probeIndex++;
			}
		}
	}

	instanceMeshComponent->SetInstanceData(instanceData);
}

void DiffuseProbeMap::SetProbeColour(XMFLOAT3 colour, uint32_t instanceMeshIndex)
{
}

uint32_t DiffuseProbeMap::GetProbeCount()
{
	return sizeX * sizeY * sizeZ;
}

ProbeData DiffuseProbeMap::FindClosestProbe(XMVECTOR pos)
{
	std::map<float, ProbeData> distanceMap;

	for (auto& probe : probeData)
	{
		distanceMap[XMVector3Length(XMLoadFloat3(&probe.position) - pos).m128_f32[0]] = probe;
	}

	if (!distanceMap.empty())
	{
		return distanceMap.begin()->second;
	}
	
	//return an empty Probe if none are found.
	return ProbeData();
}
