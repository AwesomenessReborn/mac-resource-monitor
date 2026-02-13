#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <chrono>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "metrics/cpu_metrics.h"

using namespace ftxui;

static std::string fmt_pct(double v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << v << "%";
    return ss.str();
}

int main() {
    CpuMetrics cpu;
    CPUUsage usage;
    std::mutex mx;

    auto screen = ScreenInteractive::Fullscreen();

    // Background thread: sample CPU every second, then wake the UI.
    std::thread poller([&] {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            {
                std::lock_guard<std::mutex> lock(mx);
                usage = cpu.sample();
            }
            screen.PostEvent(Event::Custom);
        }
    });
    poller.detach();

    auto renderer = Renderer([&]() -> Element {
        CPUUsage local;
        {
            std::lock_guard<std::mutex> lock(mx);
            local = usage;
        }

        Elements rows;

        // Overall bar
        rows.push_back(
            hbox({
                text("Overall") | bold,
                text("  "),
                gauge(static_cast<float>(local.overall / 100.0)) | flex,
                text("  "),
                text(fmt_pct(local.overall)) | color(Color::Cyan) | bold,
            })
        );

        if (!local.per_core.empty()) {
            rows.push_back(separator());
        }

        // Per-core bars
        for (size_t i = 0; i < local.per_core.size(); ++i) {
            double pct = local.per_core[i];
            Color bar_color = pct > 80.0 ? Color::Red
                            : pct > 50.0 ? Color::Yellow
                                         : Color::Green;
            std::string label = "Core " + std::to_string(i);
            if (i < 10) label += " ";  // align single-digit core numbers

            rows.push_back(
                hbox({
                    text(label),
                    text("  "),
                    gauge(static_cast<float>(pct / 100.0)) | flex | color(bar_color),
                    text("  "),
                    text(fmt_pct(pct)),
                })
            );
        }

        return window(
            text(" CPU Usage ") | bold | hcenter,
            vbox(std::move(rows))
        );
    });

    // Press 'q' to quit.
    auto root = CatchEvent(renderer, [&](Event event) {
        if (event == Event::Character('q')) {
            screen.ExitLoopClosure()();
            return true;
        }
        return false;
    });

    screen.Loop(root);
    return 0;
}
