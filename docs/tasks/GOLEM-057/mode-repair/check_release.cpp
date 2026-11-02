// Host ownership/order checks. check_host.py supplies the actual frozen functions.
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <memory>
#include <set>
#include <string>
#include <vector>

struct Event {
    std::string kind;
    int owner, mode, depth, live, numeric_mode;
};
struct Step { uint8_t mode, depth; int result; };
static std::vector<Event> events;
static std::vector<Step> steps;
static std::set<int> live_owners;
static size_t next_step;
static int next_owner, active_result;
static unsigned checks;
static bool recording, ttxtMode, mouseVisible;
static uint8_t videoMode, _VGAColourDepth;
static int canvasW = 640, canvasH = 480;

static void require(bool condition, const char *message) {
    ++checks;
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
static void note(const std::string &kind, int owner = 0, int mode = -1) {
    if (recording) events.push_back({kind, owner, mode, _VGAColourDepth,
                                   int(live_owners.size()), videoMode});
}
struct Controller {
    int id;
    bool ended = false;
    explicit Controller(int value) : id(value) { live_owners.insert(id); }
    void begin() { note("begin", id); }
    void end() {
        if (recording) require(!ended && live_owners.count(id), "end must precede one live deletion");
        ended = true;
        note("end", id);
    }
    ~Controller() {
        if (recording) require(ended, "controller deleted before end");
        note("delete", id);
        live_owners.erase(id);
    }
};
static std::unique_ptr<Controller> _VGAController;
struct Canvas {
    int owner;
    explicit Canvas(Controller *controller) : owner(controller ? controller->id : 0) {
        if (recording) require(controller != nullptr, "Canvas constructed without controller");
        note("canvas.new", owner);
    }
    ~Canvas() { note("canvas.delete", owner); }
};
static std::unique_ptr<Canvas> canvas;

static std::unique_ptr<Controller> getVGAController(uint8_t colours) {
    note("factory", 0, colours);
    if (active_result == 1) return nullptr;
    return std::make_unique<Controller>(next_owner++);
}
static bool updateVGAController(uint8_t colours);
static int8_t changeMode(uint8_t mode) {
    require(next_step < steps.size(), "unexpected changeMode retry");
    const Step step = steps[next_step++];
    note("attempt", 0, mode);
    require(mode == step.mode, "previous/final numeric mode changed");
    if (step.result == -1) return -1; // Invalid/teletext refusal leaves display installed.
    active_result = step.result;
    if (!updateVGAController(step.depth)) {
        require(step.result == 1, "unexpected factory refusal");
        return 1;
    }
    require(step.result != 1, "factory-error fixture accidentally used same-depth fast path");
    // Model changeResolution's existing Canvas replacement and height result.
    canvas.reset();
    canvas = std::make_unique<Canvas>(_VGAController.get());
    if (step.result == 0) videoMode = mode;
    return int8_t(step.result);
}
static void usable(const char *operation) {
    require(_VGAController && canvas, "display work during null-controller interval");
    require(canvas->owner == _VGAController->id, "display work with stale Canvas owner");
    note(operation, _VGAController->id);
}
struct Context { void cls() { usable("cls"); } };
static Context current_context;
static Context *context = &current_context;
static void debug_log(const char *, ...) {}
static void waitPlotCompletion(bool) { usable("wait"); }
static void bufferRemoveCallback(int, int) { usable("remove.callback"); }
static void resetAllContexts() { usable("reset.contexts"); }
static bool isDoubleBuffered() { usable("query.double"); return true; }
static void switchBuffer() { usable("switch"); }
static void showMouseCursor() { usable("show.mouse"); }
static void resetMousePositioner(int, int, Controller *controller) {
    require(controller == _VGAController.get(), "mouse rebound to wrong controller");
    usable("bind.mouse");
}
static void updateMouseVars(void *) { usable("mouse.vars"); }
static void bufferCallCallbacks(int) { usable("mode.callback"); }
static void sendModeInformation() { usable("mode.info"); }
static constexpr int CALLBACK_VSYNC = 1, CALLBACK_MODE_CHANGE = 2;
struct VDUStreamProcessor { void vdu_mode(uint8_t mode); };

#include "functions_under_test.hpp"

static void setup(const std::vector<Step> &script) {
    recording = false;
    canvas.reset();
    if (_VGAController) { _VGAController->ended = true; _VGAController.reset(); }
    require(live_owners.empty(), "fixture retained controller");
    steps = script;
    next_step = 0;
    next_owner = 2;
    active_result = 0;
    videoMode = 11;
    _VGAColourDepth = 2;
    mouseVisible = true;
    ttxtMode = true;
    _VGAController = std::make_unique<Controller>(1);
    canvas = std::make_unique<Canvas>(_VGAController.get());
    events.clear();
    recording = true;
}
static std::vector<size_t> releases() {
    std::vector<size_t> found;
    for (size_t i = 0; i + 2 < events.size(); ++i) {
        if (events[i].kind == "canvas.delete" && events[i+1].kind == "end" &&
            events[i+2].kind == "delete" && events[i].owner == events[i+1].owner &&
            events[i].owner == events[i+2].owner) found.push_back(i);
    }
    return found;
}
static size_t count(const std::string &kind) {
    size_t result = 0;
    for (const auto &event : events) result += event.kind == kind;
    return result;
}
static std::vector<Event> attempts() {
    std::vector<Event> result;
    for (const auto &event : events) if (event.kind == "attempt") result.push_back(event);
    return result;
}
static void run_case(const char *name, const std::vector<Step> &script,
                     unsigned expected_releases, bool same_depth = false) {
    setup(script);
    VDUStreamProcessor processor;
    processor.vdu_mode(script.front().mode);
    require(next_step == script.size(), "planned retry not performed");
    require(videoMode == script.back().mode, "successful final numeric mode lost");
    require(!ttxtMode, "existing mode reset omitted");
    require(live_owners.size() == 1, "controller ownership leaked");
    require(count("mode.info") == 1 && count("mode.callback") == 1,
            "mode epilogue must run exactly once");
    const auto disposed = releases();
    require(disposed.size() == (REPAIR_ENABLED ? expected_releases : 0),
            "failure-only Canvas/end/delete disposal count differs");
    if (REPAIR_ENABLED) {
        for (size_t i : disposed) {
            require(i + 4 < events.size(), "release lacks reconstruction");
            const Event &attempt = events[i+3], &factory = events[i+4];
            require(attempt.kind == "attempt" && factory.kind == "factory",
                    "work inserted between disposal and reconstruction");
            require(attempt.live == 0 && factory.live == 0,
                    "fallback factory overlaps failed controller lifetime");
            require(attempt.depth == 255 && factory.depth == 255,
                    "cached depth not invalidated before fallback");
        }
    }
    const auto calls = attempts();
    require(calls.front().numeric_mode == 11, "request overwrote previous numeric mode");
    if (calls.size() > 1) require(calls[1].numeric_mode == 11,
                                 "previous mode unavailable for first fallback");
    if (calls.size() > 2) require(calls[2].numeric_mode == 1,
                                 "existing final fallback assignment changed");
    if (same_depth) {
        require(count("factory") == (REPAIR_ENABLED ? 1u : 0u),
                "same-depth fallback failed to reconstruct after disposal");
        require(_VGAController->id == (REPAIR_ENABLED ? 2 : 1),
                "same-depth controller lifetime differs from expected");
    }
    // Trace enables an exact original/control and non-height-error comparison.
    std::cout << name << ':';
    for (const auto &event : events)
        std::cout << event.kind << '/' << event.owner << '/' << event.mode << '/'
                  << event.depth << '/' << event.live << '/' << event.numeric_mode << ';';
    std::cout << '\n';
}
int main() {
    run_case("success", {{20,64,0}}, 0);
    run_case("invalid_mode", {{250,64,-1},{11,2,0}}, 0);
    run_case("teletext_refusal", {{7,16,-1},{11,2,0}}, 0);
    run_case("factory_error", {{20,64,1},{11,2,0}}, 0);
    run_case("nonheight_second_error", {{250,64,-1},{11,2,-1},{1,4,0}}, 0);
    run_case("height_error", {{20,64,2},{11,2,0}}, 1);
    run_case("same_depth_height_error", {{12,2,2},{11,2,0}}, 1, true);
    run_case("two_height_errors", {{20,64,2},{11,2,2},{1,4,0}}, 2);
    run_case("second_height_error", {{250,64,-1},{11,2,2},{1,4,0}}, 1);
#if REPAIR_ENABLED
    setup({});
    releaseFailedResolution();
    require(!canvas && !_VGAController && _VGAColourDepth == 255,
            "direct disposal retained owned state");
    require(releases().size() == 1, "direct helper order differs");
    events.clear();
    releaseFailedResolution();
    require(events.empty() && _VGAColourDepth == 255,
            "empty-owner helper must remain safe");
#endif
    recording = false;
    canvas.reset();
    if (_VGAController) { _VGAController->ended = true; _VGAController.reset(); }
    require(live_owners.empty(), "final fixture teardown retained ownership");
    std::cout << "checks=" << checks << '\n';
}
