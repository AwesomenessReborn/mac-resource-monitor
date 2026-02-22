#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>

#include <chrono>
#include <deque>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

#include "metrics/cpu_metrics.h"

using namespace ftxui;

static constexpr size_t HISTORY_SIZE = 60;

static std::string fmt_pct(double v) {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(1) << v << "%";
    return ss.str();
}

// Builds a GraphFunction from a deque. ftxui passes width = cols*2, height = rows*2,
// so one return value per sub-column, scaled to [0, height].
static GraphFunction make_graph(const std::deque<double>& hist) {
    return [hist](int width, int height) -> std::vector<int> {
        std::vector<int> out(width, 0);
        int n = std::min(static_cast<int>(hist.size()), width);
        for (int i = 0; i < n; ++i) {
            int src = static_cast<int>(hist.size()) - n + i;
            int dst = width - n + i;
            out[dst] = static_cast<int>(hist[src] / 100.0 * height);
        }
        return out;
    };
}

int main() {
    CpuMetrics cpu;

    // Shared state (protected by mx).
    struct State {
        CPUUsage usage;
        std::deque<double> overall_hist;
        std::vector<std::deque<double>> core_hist;
    } state;
    std::mutex mx;

    auto screen = ScreenInteractive::Fullscreen();

    // Background thread: sample CPU every second, then wake the UI.
    std::thread poller([&] {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::lock_guard<std::mutex> lock(mx);

            state.usage = cpu.sample();
            auto& u = state.usage;

            // Overall history.
            state.overall_hist.push_back(u.overall);
            if (state.overall_hist.size() > HISTORY_SIZE)
                state.overall_hist.pop_front();

            // Per-core history — resize on first sample.
            if (state.core_hist.size() != u.per_core.size())
                state.core_hist.resize(u.per_core.size());
            for (size_t i = 0; i < u.per_core.size(); ++i) {
                state.core_hist[i].push_back(u.per_core[i]);
                if (state.core_hist[i].size() > HISTORY_SIZE)
                    state.core_hist[i].pop_front();
            }

            screen.PostEvent(Event::Custom);
        }
    });
    poller.detach();

    auto renderer = Renderer([&]() -> Element {
        // Snapshot under lock, then render without holding it.
        State local;
        {
            std::lock_guard<std::mutex> lock(mx);
            local = state;
        }

        const auto& labels = cpu.core_labels();
        size_t max_lbl = 0;
        for (const auto& l : labels) max_lbl = std::max(max_lbl, l.size());

        Elements rows;

        // ── Overall bar ──────────────────────────────────────────────────
        rows.push_back(
            hbox({
                text("Overall") | bold,
                text("  "),
                gauge(static_cast<float>(local.usage.overall / 100.0)) | flex,
                text("  "),
                text(fmt_pct(local.usage.overall)) | color(Color::Cyan) | bold,
                text("  "),
                graph(make_graph(local.overall_hist))
                    | size(WIDTH, EQUAL, 20)
                    | color(Color::Cyan),
            })
        );

        if (!local.usage.per_core.empty())
            rows.push_back(separator());

        // ── Per-core bars ─────────────────────────────────────────────────
        for (size_t i = 0; i < local.usage.per_core.size(); ++i) {
            double pct = local.usage.per_core[i];
            Color bar_color = pct > 80.0 ? Color::Red
                            : pct > 50.0 ? Color::Yellow
                                         : Color::Green;

            std::string label = (i < labels.size()) ? labels[i] : "Core " + std::to_string(i);
            while (label.size() < max_lbl) label += ' ';

            const auto& hist = (i < local.core_hist.size())
                                    ? local.core_hist[i]
                                    : std::deque<double>{};

            rows.push_back(
                hbox({
                    text(label),
                    text("  "),
                    gauge(static_cast<float>(pct / 100.0)) | flex | color(bar_color),
                    text("  "),
                    text(fmt_pct(pct)),
                    text("  "),
                    graph(make_graph(hist))
                        | size(WIDTH, EQUAL, 20)
                        | color(bar_color),
                })
            );
        }

        return window(
            text(" CPU Usage ") | bold | hcenter,
            vbox(std::move(rows))
        );
    });

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
