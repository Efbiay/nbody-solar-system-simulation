#pragma once
#include <vector>
#include <imgui.h>

namespace Plots {

// Basit, bagimliliksiz cizgi grafigi (ImDrawList ile): eksen, izgara, etiket, hover ipucu.
//  t, y   : gecmis ornekler (SI birimlerinde, t saniye)
//  tNow,yNow : "canli" son nokta (grafik her zaman simulasyon zamanina kadar uzanir)
//  xScale, yScale : SI -> gosterim birimi carpani (ornegin 1/yil, 1/AU)
void line(const char* id, const char* title, const char* unit,
          const std::vector<double>& t, const std::vector<double>& y,
          double tNow, double yNow, double xScale, double yScale,
          float height, ImU32 color);

} // namespace Plots
