#pragma once
// GLUTの表示コールバック。V-Syncごとに1フレーム描画し、
// 提示した位相をArduinoへ送って kk を進める。
#include <memory>
#include "DrawVideo.h"

extern std::unique_ptr<vmlab::DrawVideo> VideoMode;

// OpenGLとシェーダーを準備する。失敗したら false。
bool init(void);
void disp(void);
