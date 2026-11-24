#define _CRT_SECURE_NO_WARNINGS
#define NOMINMAX

#include <algorithm>
#include <array>
#include <cmath>
#include <stdio.h>
#include "ExternalEyeTracker.h"
#include "AppState.h"
#include "HeadTracking.h"

ExternalEyeTracker trackingClient;

void UpdateTracking()
{
    // Manual alignment must not depend on incoming camera data.
    if (ht == 0 && cali_flag == 0) return;

    std::array<float, 6> datas;
    if (!trackingClient.Poll(datas)) {
        if (ht) {
            printf("External eye tracking has no sample. Press h after receiving data.\n");
            ht = 0;
            cali_flag = 0;
        }
        // Keep the last displayed alignment if tracking data is lost.
        return;
    }
    tmpX = (datas[0] + datas[3]) / 2 + PosX;
    tmpY = COS * (datas[1] + datas[4]) / 2 + SIN * (datas[2] + datas[5]) / 2 + PosY;
    tmpZ = COS * (datas[2] + datas[5]) / 2 - SIN * (datas[1] + datas[4]) / 2 + PosZ;
    // Guard the existing barrier equation's singularity before calibration.
    if (std::abs(tmpZ - 0.3245f) < 0.02f) {
        ht = 0;
        cali_flag = 0;
        // Stop tracking without resetting the last displayed alignment.
        return;
    }
    if (cali_flag == 1) {
        caliX = tmpX;
        caliY = tmpY;
        caliZ = tmpZ;
        trackingBaseHaba = haba;
        trackingBaseMiddleLine = MiddleLine;
        ht = 1;
        cali_flag = 0;
        printf("External OSC head-tracking: ON\n");
    }
    for (int eye = 0; eye < 2; ++eye) {
        const int index = (1 - eye) * 3;
        eyeposx[eye] = 1000 * (datas[index] + PosX);
        eyeposy[eye] = 1000 * (COS * datas[index + 1] + SIN * datas[index + 2] + PosY);
        eyeposz[eye] = 1000 * (COS * datas[index + 2] - SIN * datas[index + 1] + PosZ);
    }
    if (ht == 1) {
        haba = std::max(2, std::min(100000,
            trackingBaseHaba + (int)((caliZ - tmpZ) * 1000 / 2.0)));
        // 同じ奥行きでは左右対称。移動方向によって符号だけが変わる。
        delta = horizontalTrackingGain * 0.895 / (tmpZ - 0.3245) * (tmpX - caliX) * 1000 / DotSubPixel;
        headTrackShift = delta;
        const int move = delta < 0.0 ? (int)(delta - 0.5) : (int)(delta + 0.5);
        MiddleLine = trackingBaseMiddleLine - move;
    }
}

void StartHeadTracking()
{
    // 外部OSCを受信する。RealSenseのカメラはEyeTracker-RSだけが開く。
    trackingClient.Start();
}

void StopHeadTracking()
{
    trackingClient.Stop();
}
