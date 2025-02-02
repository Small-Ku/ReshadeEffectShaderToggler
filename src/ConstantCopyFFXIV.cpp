#include <cstring>
#include <intrin.h>
#include <d3d11.h>
#include "ConstantCopyFFXIV.h"

using namespace Shim::Constants;
using namespace reshade::api;
using namespace std;

sig_ffxiv_cbload* ConstantCopyFFXIV::org_ffxiv_cbload = nullptr;
vector<tuple<const void*, uint64_t, size_t>> ConstantCopyFFXIV::_hostResourceBuffer;

ConstantCopyFFXIV::ConstantCopyFFXIV()
{
}

ConstantCopyFFXIV::~ConstantCopyFFXIV()
{
}

bool ConstantCopyFFXIV::Init()
{
    return Shim::GameHookT<sig_ffxiv_cbload>::Hook(&org_ffxiv_cbload, detour_ffxiv_cbload, ffxiv_cbload);
}

bool ConstantCopyFFXIV::UnInit()
{
    return MH_Uninitialize() == MH_OK;
}

void ConstantCopyFFXIV::GetHostConstantBuffer(vector<uint8_t>& dest, size_t size, uint64_t resourceHandle)
{
    for (uint32_t i = 0; i < _hostResourceBuffer.size(); i++)
    {
        auto& item = _hostResourceBuffer[i];
        if (get<1>(item) == resourceHandle)
        {
            size_t minSize = std::min(size, get<2>(item));
            memcpy(dest.data(), get<0>(item), minSize);
        }
    }
}

inline void ConstantCopyFFXIV::set_host_resource_data_location(void* origin, size_t len, int64_t resource_handle, uint64_t index)
{
    if (_hostResourceBuffer.size() < index + 1)
        _hostResourceBuffer.resize(index + 1);

    auto& entry = _hostResourceBuffer[index];
    if (get<1>(entry) != resource_handle)
        get<1>(entry) = resource_handle;

    if (get<2>(entry) != len)
        get<2>(entry) = len;

    get<0>(entry) = origin;
}

static inline int64_t exit_cbload(param_2_struct* param_2, param_3_struct& param_3, ID3D11Resource* puVar11, uint64_t uVar10)
{
    param_2->something_else[param_3.something2] = (uint8_t)uVar10;
    param_2->res[param_3.something2] = puVar11;
    param_2->something0 = param_2->something0 | (uint16_t)(1 << param_3.something2);
    return param_3.something2;
}

int64_t __fastcall ConstantCopyFFXIV::detour_ffxiv_cbload(ResourceData* param_1, param_2_struct* param_2, param_3_struct param_3, HostBufferData* param_4)
{
    int64_t lVar2;
    ID3D11DeviceContext* plVar3 = param_1->deviceContext;
    int64_t* plVar4;
    uint32_t uVar6;
    int32_t iVar7;
    uint32_t uVar9;
    uint64_t uVar10;
    ID3D11Resource* puVar11;
    uint32_t uVar12;
    const size_t valueSize = 16;
    const uint64_t copySize = static_cast<uint64_t>(param_4->size) * valueSize;
    D3D11_MAPPED_SUBRESOURCE rBuffer;

    uVar12 = param_3.something1;
    puVar11 = (ID3D11Resource*)param_4->data;
    uVar6 = param_4->size;
    if (param_4->size < uVar12) {
        uVar6 = uVar12;
    }
    uVar6 = uVar6 - 1;

    uVar10 = (uint64_t)(param_3.something0 & 3);
    if (*param_4->data == param_1->reserved1) {
        param_2->something_else[param_3.something2] = 4;
    }
    else {
        iVar7 = 31 - _lzcnt_u32(uVar6);
        plVar4 = param_4->data;
        if (iVar7 + 1U < 6) {
            lVar2 = uVar10 + (uint64_t)(iVar7 + 1U) * 4;
            uVar10 = 0;

            for (uint32_t i = 0; i < 4; i++)
            {
                if (param_1->resources[lVar2].data[i] == plVar4) {
                    uVar12 = param_1->resources[lVar2].count0;
                    param_1->resources[lVar2].count2++;
                    if (uVar12 != i) {
                        param_1->resources[lVar2].count0 = uVar12 * 8 | i;
                    }
                    puVar11 = param_1->resources[lVar2].res[i];
                    return exit_cbload(param_2, param_3, puVar11, i);
                }
            }

            param_1->resources[lVar2].count3++;
            uVar6 = 0;

            for (uint32_t i = 0; i < 10; i ++)
            {
                uVar9 = 1 << ((uint8_t)(param_1->resources[lVar2].count0 >> (uint8_t)(i * 3)) & 3) | uVar6;
                if (uVar9 == 15) break;
                uVar6 = uVar9;
            }

            uVar12 = param_1->resources[lVar2].count1 + 1 & 3;
            uVar6 = ~((uVar6 << 4 | uVar6) >> (int8_t)uVar12);

            iVar7 = _tzcnt_u32(uVar6);

            uVar9 = iVar7 + uVar12 & 3;
            uVar10 = (uint64_t)uVar9;
            param_1->resources[lVar2].data[uVar10] = plVar4;
            param_1->resources[lVar2].count1 = uVar12;
            uVar6 = param_1->resources[lVar2].count0;
            if ((uVar6 & 7) != uVar9) {
                param_1->resources[lVar2].count0 = uVar6 * 8 | uVar9;
            }
            puVar11 = param_1->resources[lVar2].res[uVar10];

            set_host_resource_data_location(plVar4, copySize, (int64_t)puVar11, lVar2 * 4 + uVar10);

            plVar3->Map(puVar11, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &rBuffer);
            memcpy(rBuffer.pData, plVar4, copySize);
            plVar3->Unmap(puVar11, 0);

            param_2->something_else[param_3.something2] = (uint8_t)uVar10;
        }
        else {
            puVar11 = param_1->tail_resources[uVar10].res;

            if (param_1->tail_resources[uVar10].data != plVar4) {
                set_host_resource_data_location(plVar4, copySize, (int64_t)puVar11, 24 * 4 + uVar10);

                plVar3->Map(puVar11, 0, D3D11_MAP::D3D11_MAP_WRITE_DISCARD, 0, &rBuffer);
                memcpy(rBuffer.pData, plVar4, copySize);
                plVar3->Unmap(puVar11, 0);
                param_1->tail_resources[uVar10].data = plVar4;
            }

            param_2->something_else[param_3.something2] = 4;
        }
    }

    param_2->res[param_3.something2] = puVar11;
    param_2->something0 = param_2->something0 | (uint16_t)(1 << param_3.something2);
    return param_3.something2;
}