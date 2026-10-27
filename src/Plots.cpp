#include "Plots.h"
#include "Localization.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Plots {

void line(const char* id, const char* title, const char* unit,
          const std::vector<double>& t, const std::vector<double>& y,
          double tNow, double yNow, double xScale, double yScale,
          float height, ImU32 color) {
    const std::size_t n = t.size();
    if (n == 0 || y.size() != n) {
        ImGui::TextDisabled("%s", Localization::get().text("plot_no_data"));
        return;
    }

    // ---- Alan ayirma ----
    const float width = ImGui::GetContentRegionAvail().x;
    const ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton(id, ImVec2(width, height));
    const bool hovered = ImGui::IsItemHovered();
    ImDrawList* dl = ImGui::GetWindowDrawList();

    const float padL = 64.f, padR = 10.f, padT = 18.f, padB = 20.f;
    const ImVec2 pmin(origin.x + padL, origin.y + padT);
    const ImVec2 pmax(origin.x + width - padR, origin.y + height - padB);
    if (pmax.x - pmin.x < 20.f) return;

    // ---- Veri araligi ----
    const double xMin = t.front() * xScale;
    double xMax = std::max(tNow, t.back()) * xScale;
    if (xMax <= xMin) xMax = xMin + 1e-6;

    double yMin = yNow * yScale, yMax = yMin;
    for (double v : y) { yMin = std::min(yMin, v * yScale); yMax = std::max(yMax, v * yScale); }
    double pad = (yMax - yMin) * 0.06;
    if (pad <= 0.0) pad = std::max(std::abs(yMax) * 0.05, 1e-12);
    yMin -= pad; yMax += pad;

    // Veri -> piksel (dogrusal donusum)
    auto px = [&](double x) { return pmin.x + static_cast<float>((x - xMin) / (xMax - xMin)) * (pmax.x - pmin.x); };
    auto py = [&](double v) { return pmax.y - static_cast<float>((v - yMin) / (yMax - yMin)) * (pmax.y - pmin.y); };

    // ---- Zemin, izgara, etiketler ----
    dl->AddRectFilled(pmin, pmax, IM_COL32(18, 18, 28, 255));
    char buf[48];
    for (int k = 0; k <= 4; ++k) {
        const double f = k / 4.0;
        const double v = yMin + f * (yMax - yMin);
        const float yy = py(v);
        dl->AddLine(ImVec2(pmin.x, yy), ImVec2(pmax.x, yy), IM_COL32(255, 255, 255, 28));
        std::snprintf(buf, sizeof buf, "%.3g", v);
        const ImVec2 ts = ImGui::CalcTextSize(buf);
        dl->AddText(ImVec2(pmin.x - ts.x - 5.f, yy - ts.y * 0.5f), IM_COL32(180, 180, 195, 255), buf);

        const double xv = xMin + f * (xMax - xMin);
        const float xx = px(xv);
        dl->AddLine(ImVec2(xx, pmin.y), ImVec2(xx, pmax.y), IM_COL32(255, 255, 255, 28));
        std::snprintf(buf, sizeof buf, "%.3g", xv);
        const ImVec2 tx = ImGui::CalcTextSize(buf);
        dl->AddText(ImVec2(xx - tx.x * 0.5f, pmax.y + 3.f), IM_COL32(180, 180, 195, 255), buf);
    }
    dl->AddRect(pmin, pmax, IM_COL32(120, 120, 140, 255));

    std::snprintf(buf, sizeof buf, "%s [%s]", title, unit);
    dl->AddText(ImVec2(pmin.x + 2.f, origin.y + 1.f), IM_COL32(235, 235, 240, 255), buf);
    const char* xl = Localization::get().text("plot_time_axis");
    dl->AddText(ImVec2(pmax.x - ImGui::CalcTextSize(xl).x, origin.y + 1.f), IM_COL32(150, 150, 165, 255), xl);

    // ---- Seri: piksel kovalarina gore min/max seyreltme ----
    // Ornek sayisi piksel sayisindan cok fazlaysa her piksel sutununda sadece
    // en dusuk ve en yuksek nokta cizilir; tepe noktalari kaybolmaz.
    const std::size_t total = n + 1;  // +1: canli son nokta
    auto point = [&](std::size_t i) -> ImVec2 {
        return i < n ? ImVec2(px(t[i] * xScale), py(y[i] * yScale))
                     : ImVec2(px(xMax), py(yNow * yScale));
    };
    static std::vector<ImVec2> pts;
    pts.clear();
    const int buckets = std::max(1, static_cast<int>(pmax.x - pmin.x));
    if (total <= static_cast<std::size_t>(buckets) * 2) {
        for (std::size_t i = 0; i < total; ++i) pts.push_back(point(i));
    } else {
        std::size_t i = 0;
        while (i < total) {
            const ImVec2 first = point(i);
            const int b = static_cast<int>(first.x - pmin.x);
            std::size_t lo = i, hi = i, j = i;
            float yLo = first.y, yHi = first.y;   // ekran y'si: kucuk = yukari
            for (; j < total; ++j) {
                const ImVec2 p = point(j);
                if (static_cast<int>(p.x - pmin.x) != b) break;
                if (p.y < yLo) { yLo = p.y; lo = j; }
                if (p.y > yHi) { yHi = p.y; hi = j; }
            }
            if (lo == hi) pts.push_back(point(lo));
            else { pts.push_back(point(std::min(lo, hi))); pts.push_back(point(std::max(lo, hi))); }
            i = j;
        }
        pts.push_back(point(total - 1));
    }

    dl->PushClipRect(pmin, pmax, true);
    if (pts.size() >= 2) dl->AddPolyline(pts.data(), static_cast<int>(pts.size()), color, ImDrawFlags_None, 1.6f);
    dl->PopClipRect();

    // ---- Hover: en yakin ornek ----
    if (hovered) {
        const ImVec2 m = ImGui::GetIO().MousePos;
        if (m.x >= pmin.x && m.x <= pmax.x) {
            const double tx = (xMin + (m.x - pmin.x) / (pmax.x - pmin.x) * (xMax - xMin)) / xScale;
            double ts, ys;
            if (tx >= t.back()) {
                ts = tNow; ys = yNow;
            } else {
                std::size_t k = static_cast<std::size_t>(std::lower_bound(t.begin(), t.end(), tx) - t.begin());
                if (k > 0 && tx - t[k - 1] < t[k] - tx) --k;
                ts = t[k]; ys = y[k];
            }
            const float hx = px(ts * xScale), hy = py(ys * yScale);
            dl->AddLine(ImVec2(hx, pmin.y), ImVec2(hx, pmax.y), IM_COL32(255, 255, 255, 90));
            dl->AddCircleFilled(ImVec2(hx, hy), 3.5f, color);
            ImGui::SetTooltip(Localization::get().text("plot_tooltip"),
                              ts * xScale, title, ys * yScale, unit);
        }
    }
}

} // namespace Plots
