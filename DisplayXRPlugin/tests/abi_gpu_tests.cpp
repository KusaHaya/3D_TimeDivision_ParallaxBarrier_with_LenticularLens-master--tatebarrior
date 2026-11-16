#include <winsock2.h>
#include <windows.h>
#include <d3d11.h>
#include <wrl/client.h>
#include "xrt/xrt_plugin.h"
#include "xrt/xrt_display_metrics.h"
#include "projectpb/Barrier.h"
#include "OscReceiver.h"
#include <chrono>
#include <cstring>
#include <iostream>
#include <map>
#include <stdexcept>
#include <thread>
#include <vector>
using Microsoft::WRL::ComPtr;
static std::map<std::string,std::string> settings;
static void Check(bool ok,const char* what) { if(!ok) throw std::runtime_error(what); }
static void HR(HRESULT hr,const char* what) { Check(SUCCEEDED(hr),what); }
static bool Setting(const char* id,const char* key,char* buf,uint32_t n) {
    Check(std::string(id)=="projectpb","setting component id");
    auto it=settings.find(key); if(it==settings.end()) return false;
    strncpy_s(buf,n,it->second.c_str(),_TRUNCATE); return true;
}
static uint8_t Channel(unsigned eye,unsigned x,unsigned y,unsigned c) {
    return static_cast<uint8_t>(20+eye*100+(x*3+y*7+c*11)%80);
}
static std::vector<unsigned char> Packet(const char* address,float x,float y,float z) {
    std::vector<unsigned char> p;
    auto text=[&](const char* s) { p.insert(p.end(),s,s+std::strlen(s)+1); while(p.size()%4)p.push_back(0); };
    text(address); text(",fff");
    for(float f:{x,y,z}) { uint32_t b; std::memcpy(&b,&f,4); for(int shift=24;shift>=0;shift-=8)p.push_back(static_cast<unsigned char>(b>>shift)); }
    return p;
}
static void TestUDP() {
    projectpb::CameraTransform transform; transform.tilt=0; transform.translation={0,0,0};
    // Isolated test port; never opens the user's configured 8000 input.
    projectpb::OscReceiver receiver(39147,transform); Check(receiver.Ready(),"bind isolated OSC input");
    projectpb::OscReceiver conflict(39147,transform); Check(!conflict.Ready(),"exclusive UDP ownership");
    SOCKET sender=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
    sockaddr_in target{}; target.sin_family=AF_INET; target.sin_addr.s_addr=htonl(INADDR_LOOPBACK); target.sin_port=htons(39147);
    auto send=[&](const std::vector<unsigned char>& p) { Check(sendto(sender,reinterpret_cast<const char*>(p.data()),static_cast<int>(p.size()),0,
        reinterpret_cast<sockaddr*>(&target),sizeof(target))==static_cast<int>(p.size()),"send OSC"); };
    auto left=Packet("/eye/left",-.03f,0,1),right=Packet("/eye/right",.03f,0,1);
    std::vector<unsigned char> broken={'#','b','u','n','d','l','e',0,0,0,0,0,0,0,0,1};
    for(const auto& p:{left,right}) { const auto n=static_cast<uint32_t>(p.size()); for(int s=24;s>=0;s-=8)broken.push_back(static_cast<unsigned char>(n>>s)); broken.insert(broken.end(),p.begin(),p.end()); }
    broken.push_back(1); send(broken);
    std::this_thread::sleep_for(std::chrono::milliseconds(60));
    Check(!receiver.Read(projectpb::MonotonicNs()).valid,"malformed bundle is transactional");
    send(right); send(left);
    projectpb::EyeSample sample;
    for(int n=0;n<100;++n) { sample=receiver.Read(projectpb::MonotonicNs()); if(sample.valid)break; std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
    closesocket(sender);
    Check(sample.valid && sample.left[0]<0 && sample.right[0]>0 && sample.receivedNs>0,"loopback OSC eye pair");
}
int main(int argc,char** argv) {
    HMODULE library=nullptr;
    try {
        Check(argc==2,"DLL path required");
        library=LoadLibraryA(argv[1]); Check(library!=nullptr,"load standalone plugin DLL");
        auto negotiate=reinterpret_cast<xrt_plugin_negotiate_fn_t>(GetProcAddress(library,"xrtPluginNegotiate")); Check(negotiate!=nullptr,"C export");
        xrt_plugin_iface* iface=reinterpret_cast<xrt_plugin_iface*>(uintptr_t(1)); uint32_t version=0;
        Check(negotiate(0,nullptr,&iface,&version)!=XRT_SUCCESS && !iface && version==XRT_PLUGIN_API_VERSION_CURRENT,"ABI mismatch decline");
        xrt_plugin_host_iface shortHost{}; shortHost.struct_size=8; shortHost.host_api_version=version;
        Check(negotiate(version,&shortHost,&iface,&version)==XRT_SUCCESS,"short host negotiation");
        xrt_plugin_instance* instance=nullptr;
        Check(iface->probe(&instance)==XRT_ERROR_PROBER_NOT_SUPPORTED && !instance,"opt in disabled by default");
        xrt_plugin_host_iface host{}; host.struct_size=sizeof(host); host.host_api_version=version; host.get_setting=Setting;
        Check(negotiate(version,&host,&iface,&version)==XRT_SUCCESS,"official negotiation");
        Check(std::string(iface->id)=="projectpb" && iface->create_dp_d3d11 && !iface->create_dp_vk,"D3D11-only capability");
        xrt_plugin_platform_status shortStatus;
        std::memset(&shortStatus,0xa5,sizeof(shortStatus)); shortStatus.struct_size=8;
        Check(iface->get_platform_state(&shortStatus),"short output struct");
        auto* statusBytes=reinterpret_cast<unsigned char*>(&shortStatus);
        for(size_t n=8;n<sizeof(shortStatus);++n) Check(statusBytes[n]==0xa5,"no write past caller struct_size");
        settings={{"ENABLE","1"},{"PHASE","9"}};
        ComPtr<ID3D11Device> device; ComPtr<ID3D11DeviceContext> context; D3D_FEATURE_LEVEL level{};
        HR(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_WARP,nullptr,0,nullptr,0,D3D11_SDK_VERSION,&device,&level,&context),"WARP device");
        xrt_display_processor_d3d11* dp=nullptr;
        Check(iface->create_dp_d3d11(device.Get(),context.Get(),nullptr,&dp)!=XRT_SUCCESS && !dp,"reject invalid phase");
        settings={{"ENABLE","1"},{"TRACKING","1"}};
        Check(iface->create_dp_d3d11(device.Get(),context.Get(),nullptr,&dp)!=XRT_SUCCESS && !dp,"uncalibrated tracker rejected");
        constexpr unsigned W=1920,H=1080,VW=64,VH=32,AW=160,AH=48;
        std::vector<unsigned char> input(AW*AH*4,255);
        for(unsigned eye=0;eye<2;++eye) for(unsigned y=0;y<VH;++y) for(unsigned x=0;x<VW;++x)
            for(unsigned c=0;c<3;++c) input[(y*AW+eye*VW+x)*4+c]=Channel(eye,x,y,c);
        D3D11_TEXTURE2D_DESC inDesc{}; inDesc.Width=AW;inDesc.Height=AH;inDesc.MipLevels=1;inDesc.ArraySize=1;
        inDesc.Format=DXGI_FORMAT_R8G8B8A8_UNORM;inDesc.SampleDesc.Count=1;inDesc.Usage=D3D11_USAGE_IMMUTABLE;inDesc.BindFlags=D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA data{input.data(),AW*4,0}; ComPtr<ID3D11Texture2D> atlas;
        HR(device->CreateTexture2D(&inDesc,&data,&atlas),"atlas with padded row and column");
        ComPtr<ID3D11ShaderResourceView> srv; HR(device->CreateShaderResourceView(atlas.Get(),nullptr,&srv),"atlas SRV");
        D3D11_TEXTURE2D_DESC outDesc=inDesc;outDesc.Width=W;outDesc.Height=H;outDesc.Usage=D3D11_USAGE_DEFAULT;outDesc.BindFlags=D3D11_BIND_RENDER_TARGET;
        ComPtr<ID3D11Texture2D> output;HR(device->CreateTexture2D(&outDesc,nullptr,&output),"output texture");
        ComPtr<ID3D11RenderTargetView> rtv;HR(device->CreateRenderTargetView(output.Get(),nullptr,&rtv),"output RTV");
        outDesc.Usage=D3D11_USAGE_STAGING;outDesc.BindFlags=0;outDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        ComPtr<ID3D11Texture2D> readback;HR(device->CreateTexture2D(&outDesc,nullptr,&readback),"readback");
        ID3D11RenderTargetView* bound=rtv.Get();context->OMSetRenderTargets(1,&bound,nullptr);
        const D3D11_VIEWPORT oldViewport{3,4,500,600,0,1};context->RSSetViewports(1,&oldViewport);
        context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);
        for(unsigned phase=0;phase<4;++phase) {
            settings={{"ENABLE","1"},{"PHASE",std::to_string(phase)}};
            Check(iface->create_dp_d3d11(device.Get(),context.Get(),nullptr,&dp)==XRT_SUCCESS && dp,"create official D3D11 DP");
            Check(dp->struct_size==sizeof(*dp) && !dp->request_display_mode && !dp->get_hardware_3d_state,"no invented LED acknowledgement");
            xrt_eye_positions eyes{};Check(dp->get_eye_positions_at(dp,projectpb::MonotonicNs(),&eyes) && eyes.count==2 && eyes.valid &&
                !eyes.is_tracking && eyes.timestamp_ns==0 && eyes.eyes[0].x<0 && eyes.eyes[1].x>0,"honest nominal eyes");
            dp->process_atlas(dp,context.Get(),srv.Get(),VW,VH,2,1,inDesc.Format,W,H,0,0,0,0);
            uint32_t state=99;Check(dp->get_backend_state(dp,&state)&&state==XRT_DP_BACKEND_STATE_OK,"weave backend healthy");
            D3D11_PRIMITIVE_TOPOLOGY topology{};context->IAGetPrimitiveTopology(&topology);
            D3D11_VIEWPORT viewport{};UINT count=1;context->RSGetViewports(&count,&viewport);
            Check(topology==D3D11_PRIMITIVE_TOPOLOGY_LINELIST && viewport.TopLeftX==3 && viewport.Width==500,"restore caller GPU state");
            context->CopyResource(readback.Get(),output.Get());D3D11_MAPPED_SUBRESOURCE mapped{};
            HR(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&mapped),"read pixels");
            uint64_t mismatches=0;
            for(unsigned y=0;y<H;++y)for(unsigned x=0;x<W;++x) {
                auto* pixel=static_cast<unsigned char*>(mapped.pData)+y*mapped.RowPitch+x*4;
                const unsigned sx=std::min(VW-1,static_cast<unsigned>((x+.5)*VW/W));
                const unsigned sy=std::min(VH-1,static_cast<unsigned>((y+.5)*VH/H));
                for(unsigned c=0;c<3;++c) {
                    unsigned eye=projectpb::SelectLeft(x,y,H,c,phase,{})?0:1;
                    if(pixel[c]!=Channel(eye,sx,sy,c))++mismatches;
                }
                if(pixel[3]!=255)++mismatches;
            }
            context->Unmap(readback.Get(),0);
            Check(mismatches==0,"GPU/legacy optical masks and atlas UV must match for every pixel");
            dp->set_present_origin(dp,1,0);
            dp->process_atlas(dp,context.Get(),srv.Get(),VW,VH,2,1,inDesc.Format,W,H,0,0,0,0);
            Check(dp->get_backend_state(dp,&state)&&state==XRT_DP_BACKEND_STATE_DEGRADED,"reject shifted panel origin");
            dp->destroy(dp);dp=nullptr;
        }
        // Exercise OSC -> shared plugin receiver -> official target-time eye query.
        settings={{"ENABLE","1"},{"TRACKING","1"},{"CALIBRATION_CONFIRMED","1"},
                  {"OPTICAL_TRACKING","1"},
                  {"OSC_PORT","39148"},{"CAM_TILT_RAD","0"},{"CAM_X_M","0"},{"CAM_Y_M","0"},{"CAM_Z_M","0"}};
        xrt_display_processor_d3d11* second=nullptr;
        Check(iface->create_dp_d3d11(device.Get(),context.Get(),nullptr,&dp)==XRT_SUCCESS,"tracking DP");
        Check(iface->create_dp_d3d11(device.Get(),context.Get(),nullptr,&second)==XRT_SUCCESS,"shared UDP owner across DPs");
        SOCKET sender=socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP);
        sockaddr_in destination{};destination.sin_family=AF_INET;destination.sin_addr.s_addr=htonl(INADDR_LOOPBACK);destination.sin_port=htons(39148);
        for(const auto& message:{Packet("/eye/right",.034f,.02f,.7f),Packet("/eye/left",-.034f,.02f,.7f)})
            Check(sendto(sender,reinterpret_cast<const char*>(message.data()),static_cast<int>(message.size()),0,
                reinterpret_cast<sockaddr*>(&destination),sizeof(destination))==static_cast<int>(message.size()),"send to plugin");
        closesocket(sender);
        xrt_eye_positions measured{};
        for(int n=0;n<100;++n) {
            dp->get_eye_positions_at(dp,projectpb::MonotonicNs(),&measured);
            if(measured.timestamp_ns>0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        Check(measured.timestamp_ns>0 && measured.eyes[0].x==-.034f && measured.eyes[1].x==.034f &&
              measured.eyes[0].y==-.02f && measured.eyes[0].z==.7f && !measured.is_tracking,"OSC to official DP eyes");
        xrt_eye_positions held{};second->get_eye_positions_at(second,measured.timestamp_ns+1000000000LL,&held);
        Check(held.timestamp_ns==measured.timestamp_ns && !held.is_tracking && held.valid,"hold does not forge measurement time");
        dp->process_atlas(dp,context.Get(),srv.Get(),VW,VH,2,1,inDesc.Format,W,H,0,0,0,0);
        context->CopyResource(readback.Get(),output.Get());D3D11_MAPPED_SUBRESOURCE dynamicPixels{};
        HR(context->Map(readback.Get(),0,D3D11_MAP_READ,0,&dynamicPixels),"tracked optical pixels");
        projectpb::Barrier tracked;
        Check(projectpb::TrackedBarrier(0,.7f,0,.65f,.7f,tracked),"tracked optical model");
        unsigned dynamicMismatch=0;
        for(unsigned y=0;y<H;y+=37) for(unsigned x=0;x<W;x+=19) for(unsigned c=0;c<3;++c) {
            const unsigned sx=std::min(VW-1,static_cast<unsigned>((x+.5)*VW/W));
            const unsigned sy=std::min(VH-1,static_cast<unsigned>((y+.5)*VH/H));
            const unsigned eye=projectpb::SelectLeft(x,y,H,c,0,tracked)?0:1;
            const auto actual=static_cast<unsigned char*>(dynamicPixels.pData)[y*dynamicPixels.RowPitch+x*4+c];
            if(actual!=Channel(eye,sx,sy,c))++dynamicMismatch;
        }
        context->Unmap(readback.Get(),0);
        Check(dynamicMismatch==0,"OSC eyes drive the legacy optical shader correction");
        dp->destroy(dp);dp=nullptr;
        Check(second->get_predicted_eye_positions(second,&held) && held.timestamp_ns==measured.timestamp_ns,"surviving DP retains receiver");
        second->destroy(second);
        TestUDP();
        FreeLibrary(library); library=nullptr;
        std::cout << "PASS: DLL ABI, defaults, invalid settings, 24,883,200 GPU RGB comparisons, state restore, UDP integration\n";
        return 0;
    } catch(const std::exception& e) {
        // Do not unload a DLL with a live test DP on failure; process teardown owns cleanup.
        std::cerr << "FAIL: " << e.what() << '\n'; return 1;
    }
}
