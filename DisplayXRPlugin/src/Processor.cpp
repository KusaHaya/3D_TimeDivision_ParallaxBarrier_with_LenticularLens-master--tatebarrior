#include "PluginInternal.h"
#include "ShaderSource.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <atomic>

using Microsoft::WRL::ComPtr;
namespace projectpb {
namespace {
struct Params {
    float haba, middle, shift, slant;
    float threshold; uint32_t phase, height, mono;
    float atlasSize[2], viewSize[2];
};
static_assert(sizeof(Params) == 48);
struct Processor {
    xrt_display_processor_d3d11 base{};
    Settings settings;
    Panel panel;
    HWND window = nullptr;
    std::shared_ptr<OscReceiver> receiver;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11VertexShader> vs;
    ComPtr<ID3D11PixelShader> ps;
    ComPtr<ID3D11Buffer> cb;
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11RasterizerState> rasterizer;
    ComPtr<ID3D11BlendState> blend;
    ComPtr<ID3D11DepthStencilState> depth;
    std::atomic<uint32_t> state{XRT_DP_BACKEND_STATE_OK};
    std::atomic<int32_t> originX{0}, originY{0};
    xrt_atlas_encoding encoding = XRT_ATLAS_ENCODING_ENCODED;
    Barrier heldBarrier;
};
// Restore the state touched by the weaver, including shader class instances.
struct Guard {
    ID3D11DeviceContext* c;
    ComPtr<ID3D11VertexShader> vs; ComPtr<ID3D11PixelShader> ps;
    ComPtr<ID3D11GeometryShader> gs; ComPtr<ID3D11HullShader> hs; ComPtr<ID3D11DomainShader> ds;
    ID3D11ClassInstance *vc[256]{}, *pc[256]{}, *gc[256]{}, *hc[256]{}, *dc[256]{};
    UINT nv=256,np=256,ng=256,nh=256,nd=256;
    ComPtr<ID3D11InputLayout> layout;
    D3D11_PRIMITIVE_TOPOLOGY topology{};
    ComPtr<ID3D11Buffer> cb;
    ComPtr<ID3D11ShaderResourceView> srv;
    ComPtr<ID3D11SamplerState> sampler;
    ComPtr<ID3D11RasterizerState> rs;
    ComPtr<ID3D11BlendState> blend;
    FLOAT factor[4]{}; UINT mask=0;
    ComPtr<ID3D11DepthStencilState> depth; UINT reference=0;
    D3D11_VIEWPORT viewports[16]{}; UINT viewportCount=16;
    ComPtr<ID3D11Predicate> predicate; BOOL predicateValue=FALSE;
    explicit Guard(ID3D11DeviceContext* ctx) : c(ctx) {
        c->VSGetShader(&vs, vc, &nv); c->PSGetShader(&ps, pc, &np); c->GSGetShader(&gs, gc, &ng);
        c->HSGetShader(&hs, hc, &nh); c->DSGetShader(&ds, dc, &nd);
        c->IAGetInputLayout(&layout); c->IAGetPrimitiveTopology(&topology);
        c->PSGetConstantBuffers(0,1,&cb); c->PSGetShaderResources(0,1,&srv); c->PSGetSamplers(0,1,&sampler);
        c->RSGetState(&rs); c->RSGetViewports(&viewportCount, viewports);
        c->OMGetBlendState(&blend, factor, &mask); c->OMGetDepthStencilState(&depth, &reference);
        c->GetPredication(&predicate, &predicateValue);
    }
    ~Guard() {
        c->VSSetShader(vs.Get(), vc, nv); c->PSSetShader(ps.Get(), pc, np); c->GSSetShader(gs.Get(), gc, ng);
        c->HSSetShader(hs.Get(), hc, nh); c->DSSetShader(ds.Get(), dc, nd);
        for(UINT i=0;i<nv;++i) vc[i]->Release(); for(UINT i=0;i<np;++i) pc[i]->Release();
        for(UINT i=0;i<ng;++i) gc[i]->Release(); for(UINT i=0;i<nh;++i) hc[i]->Release();
        for(UINT i=0;i<nd;++i) dc[i]->Release();
        c->IASetInputLayout(layout.Get()); c->IASetPrimitiveTopology(topology);
        ID3D11Buffer* b=cb.Get(); c->PSSetConstantBuffers(0,1,&b);
        ID3D11ShaderResourceView* s=srv.Get(); c->PSSetShaderResources(0,1,&s);
        ID3D11SamplerState* sp=sampler.Get(); c->PSSetSamplers(0,1,&sp);
        c->RSSetState(rs.Get()); c->RSSetViewports(viewportCount, viewports);
        c->OMSetBlendState(blend.Get(),factor,mask); c->OMSetDepthStencilState(depth.Get(),reference);
        c->SetPredication(predicate.Get(), predicateValue);
    }
};
void Black(ID3D11DeviceContext* c) {
    ComPtr<ID3D11RenderTargetView> rtv; c->OMGetRenderTargets(1,&rtv,nullptr);
    const float black[4]={0,0,0,1}; if (rtv) c->ClearRenderTargetView(rtv.Get(),black);
}
void Process(xrt_display_processor_d3d11* base, void* context, void* atlas,
             uint32_t vw, uint32_t vh, uint32_t cols, uint32_t rows, uint32_t,
             uint32_t width, uint32_t height, int32_t ox, int32_t oy, uint32_t cw, uint32_t ch) {
    auto& p=*reinterpret_cast<Processor*>(base);
    auto* c=static_cast<ID3D11DeviceContext*>(context);
    auto* srv=static_cast<ID3D11ShaderResourceView*>(atlas);
    if (!c) { p.state=XRT_DP_BACKEND_STATE_DEGRADED; return; }
    auto fail=[&] { Black(c); p.state=XRT_DP_BACKEND_STATE_DEGRADED; };
    if (!srv || width!=1920 || height!=1080 || ox || oy || (cw && cw!=width) || (ch && ch!=height) ||
        p.originX.load() || p.originY.load() || rows!=1 || (cols!=1 && cols!=2) || !vw || !vh ||
        p.encoding!=XRT_ATLAS_ENCODING_ENCODED) { fail(); return; }
    if (p.window) {
        RECT r{}; POINT origin{};
        if (!GetClientRect(p.window,&r) || !ClientToScreen(p.window,&origin) ||
            r.right!=1920 || r.bottom!=1080 || origin.x!=p.panel.left || origin.y!=p.panel.top) { fail(); return; }
    }
    ComPtr<ID3D11Device> contextDevice; c->GetDevice(&contextDevice);
    if (contextDevice.Get()!=p.device.Get()) { fail(); return; }
    D3D11_SHADER_RESOURCE_VIEW_DESC sd{}; srv->GetDesc(&sd);
    if (sd.ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2D || sd.Texture2D.MostDetailedMip!=0 ||
        (sd.Format!=DXGI_FORMAT_R8G8B8A8_UNORM && sd.Format!=DXGI_FORMAT_B8G8R8A8_UNORM)) { fail(); return; }
    ComPtr<ID3D11Resource> resource; srv->GetResource(&resource);
    ComPtr<ID3D11Texture2D> tex; if (FAILED(resource.As(&tex))) { fail(); return; }
    D3D11_TEXTURE2D_DESC td{}; tex->GetDesc(&td);
    if (td.Width < static_cast<uint64_t>(vw)*cols || td.Height < vh || td.SampleDesc.Count!=1) { fail(); return; }
    ComPtr<ID3D11RenderTargetView> rtv; c->OMGetRenderTargets(1,&rtv,nullptr);
    if (!rtv) { p.state=XRT_DP_BACKEND_STATE_DEGRADED; return; }
    D3D11_RENDER_TARGET_VIEW_DESC rd{}; rtv->GetDesc(&rd);
    if ((rd.Format!=DXGI_FORMAT_R8G8B8A8_UNORM && rd.Format!=DXGI_FORMAT_B8G8R8A8_UNORM) ||
        rd.ViewDimension!=D3D11_RTV_DIMENSION_TEXTURE2D || rd.Texture2D.MipSlice!=0) { fail(); return; }
    ComPtr<ID3D11Resource> outputResource; rtv->GetResource(&outputResource);
    ComPtr<ID3D11Texture2D> output; if (FAILED(outputResource.As(&output))) { fail(); return; }
    D3D11_TEXTURE2D_DESC outputDesc{}; output->GetDesc(&outputDesc);
    if (outputDesc.Width!=width || outputDesc.Height!=height || outputDesc.SampleDesc.Count!=1) { fail(); return; }
    bool degraded = false;
    if (p.settings.tracking) {
        const auto sample=p.receiver->Read(MonotonicNs());
        degraded=!p.receiver->Ready() || !sample.fresh;
        if (p.settings.opticalTracking && sample.valid) {
            Barrier next=p.settings.barrier;
            if (TrackedBarrier((sample.left[0]+sample.right[0])*.5f,(sample.left[2]+sample.right[2])*.5f,
                               p.settings.refX,p.settings.refZ,p.settings.horizontalGain,next)) p.heldBarrier=next;
            else degraded=true; // Keep last optical alignment at the singularity.
        }
    }
    Guard guard(c);
    const auto& barrier=p.heldBarrier;
    Params params{barrier.haba,barrier.middle,barrier.shift,barrier.slant,
                  p.settings.barrier.threshold,p.settings.phase,height,cols==1 ? 1u:0u,
                  {static_cast<float>(td.Width),static_cast<float>(td.Height)},
                  {static_cast<float>(vw),static_cast<float>(vh)}};
    c->UpdateSubresource(p.cb.Get(),0,nullptr,&params,0,0);
    D3D11_VIEWPORT viewport{0,0,static_cast<float>(width),static_cast<float>(height),0,1};
    c->SetPredication(nullptr,FALSE);
    c->RSSetViewports(1,&viewport); c->RSSetState(p.rasterizer.Get());
    c->OMSetBlendState(p.blend.Get(),nullptr,0xffffffff); c->OMSetDepthStencilState(p.depth.Get(),0);
    c->IASetInputLayout(nullptr); c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    c->VSSetShader(p.vs.Get(),nullptr,0); c->PSSetShader(p.ps.Get(),nullptr,0);
    c->GSSetShader(nullptr,nullptr,0); c->HSSetShader(nullptr,nullptr,0); c->DSSetShader(nullptr,nullptr,0);
    ID3D11Buffer* cb=p.cb.Get(); c->PSSetConstantBuffers(0,1,&cb);
    ID3D11SamplerState* sampler=p.sampler.Get(); c->PSSetSamplers(0,1,&sampler);
    c->PSSetShaderResources(0,1,&srv); c->Draw(3,0);
    p.state=degraded ? XRT_DP_BACKEND_STATE_DEGRADED : XRT_DP_BACKEND_STATE_OK;
}
bool EyesAt(xrt_display_processor_d3d11* b,int64_t target,xrt_eye_positions* out) {
    auto& p=*reinterpret_cast<Processor*>(b); return FillEyes(p.settings,p.receiver,target,out);
}
bool Eyes(xrt_display_processor_d3d11* b,xrt_eye_positions* out) { return EyesAt(b,MonotonicNs(),out); }
bool Metrics(xrt_display_processor_d3d11* b,xrt_window_metrics* out) {
    if (!out) return false;
    auto& p=*reinterpret_cast<Processor*>(b); *out={};
    out->display_width_m=.528f; out->display_height_m=.297f;
    out->display_pixel_width=out->window_pixel_width=1920;
    out->display_pixel_height=out->window_pixel_height=1080;
    out->display_screen_left=out->window_screen_left=p.panel.left;
    out->display_screen_top=out->window_screen_top=p.panel.top;
    out->window_width_m=.528f; out->window_height_m=.297f;
    out->window_orientation.w=1; out->valid=p.panel.found; return out->valid;
}
}
xrt_result_t CreateProcessor(void* device,void*,void* window,xrt_display_processor_d3d11** out) {
    if (!out) return XRT_ERROR_DEVICE_CREATION_FAILED;
    *out=nullptr;
    try {
        auto p=std::make_unique<Processor>(); p->settings=ReadSettings(); p->panel=FindPanel(p->settings);
        p->heldBarrier=p->settings.barrier;
        if (!device || !p->settings.valid || !p->settings.enabled || (window && !p->panel.found))
            return XRT_ERROR_DEVICE_CREATION_FAILED;
        p->window=static_cast<HWND>(window); p->device=static_cast<ID3D11Device*>(device);
        p->receiver=AcquireTracking(p->settings);
        if (p->settings.tracking && !p->receiver) return XRT_ERROR_DEVICE_CREATION_FAILED;
        ComPtr<ID3DBlob> vs,ps,errors;
        if (FAILED(D3DCompile(kProjectPBShader,sizeof(kProjectPBShader)-1,nullptr,nullptr,nullptr,"VS","vs_5_0",
                              D3DCOMPILE_ENABLE_STRICTNESS,0,&vs,&errors)) ||
            FAILED(D3DCompile(kProjectPBShader,sizeof(kProjectPBShader)-1,nullptr,nullptr,nullptr,"PS","ps_5_0",
                              D3DCOMPILE_ENABLE_STRICTNESS,0,&ps,&errors))) return XRT_ERROR_DEVICE_CREATION_FAILED;
        if (FAILED(p->device->CreateVertexShader(vs->GetBufferPointer(),vs->GetBufferSize(),nullptr,&p->vs)) ||
            FAILED(p->device->CreatePixelShader(ps->GetBufferPointer(),ps->GetBufferSize(),nullptr,&p->ps)))
            return XRT_ERROR_DEVICE_CREATION_FAILED;
        D3D11_BUFFER_DESC bd{}; bd.ByteWidth=sizeof(Params); bd.Usage=D3D11_USAGE_DEFAULT; bd.BindFlags=D3D11_BIND_CONSTANT_BUFFER;
        D3D11_SAMPLER_DESC sd{}; sd.Filter=D3D11_FILTER_MIN_MAG_MIP_POINT;
        sd.AddressU=sd.AddressV=sd.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP; sd.MaxLOD=D3D11_FLOAT32_MAX;
        D3D11_RASTERIZER_DESC rs{}; rs.FillMode=D3D11_FILL_SOLID; rs.CullMode=D3D11_CULL_NONE; rs.DepthClipEnable=TRUE;
        D3D11_BLEND_DESC blend{}; blend.RenderTarget[0].RenderTargetWriteMask=D3D11_COLOR_WRITE_ENABLE_ALL;
        D3D11_DEPTH_STENCIL_DESC depth{}; depth.DepthFunc=D3D11_COMPARISON_ALWAYS;
        if (FAILED(p->device->CreateBuffer(&bd,nullptr,&p->cb)) || FAILED(p->device->CreateSamplerState(&sd,&p->sampler)) ||
            FAILED(p->device->CreateRasterizerState(&rs,&p->rasterizer)) ||
            FAILED(p->device->CreateBlendState(&blend,&p->blend)) ||
            FAILED(p->device->CreateDepthStencilState(&depth,&p->depth))) return XRT_ERROR_DEVICE_CREATION_FAILED;
        auto& b=p->base; b.struct_size=sizeof(b); b.process_atlas=Process;
        b.destroy=[](xrt_display_processor_d3d11* base) { delete reinterpret_cast<Processor*>(base); };
        b.get_predicted_eye_positions=Eyes; b.get_eye_positions_at=EyesAt; b.get_window_metrics=Metrics;
        b.get_display_dimensions=[](xrt_display_processor_d3d11*,float* w,float* h) { *w=.528f; *h=.297f; return true; };
        b.get_display_pixel_info=[](xrt_display_processor_d3d11* base,uint32_t* w,uint32_t* h,int32_t* x,int32_t* y) {
            auto& p=*reinterpret_cast<Processor*>(base); *w=1920; *h=1080; *x=p.panel.left; *y=p.panel.top; return p.panel.found;
        };
        b.get_backend_state=[](xrt_display_processor_d3d11* base,uint32_t* state) {
            *state=reinterpret_cast<Processor*>(base)->state.load(); return true;
        };
        b.set_atlas_encoding=[](xrt_display_processor_d3d11* base,xrt_atlas_encoding e) { reinterpret_cast<Processor*>(base)->encoding=e; };
        b.set_present_origin=[](xrt_display_processor_d3d11* base,int32_t x,int32_t y) {
            auto& p=*reinterpret_cast<Processor*>(base); p.originX=x; p.originY=y;
        };
        // No request_display_mode/get_hardware_3d_state: this build cannot
        // acknowledge LED hardware state. No serial port is opened anywhere.
        *out=&p.release()->base; return XRT_SUCCESS;
    } catch (...) { return XRT_ERROR_DEVICE_CREATION_FAILED; }
}
} // namespace projectpb
