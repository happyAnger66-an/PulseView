// Minimal Perfetto Track Event example (in-process tracing).
// Writes pulseview_example.pftrace in the current working directory.

#include "trace_categories.h"

#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace {

void InitPerfetto() {
  perfetto::TracingInitArgs args;
  args.backends = perfetto::kInProcessBackend;
  perfetto::Tracing::Initialize(args);
  perfetto::TrackEvent::Register();
}

std::unique_ptr<perfetto::TracingSession> StartTracing() {
  perfetto::TraceConfig cfg;
  cfg.add_buffers()->set_size_kb(1024);

  auto* ds_cfg = cfg.add_data_sources()->mutable_config();
  ds_cfg->set_name("track_event");

  perfetto::protos::gen::TrackEventConfig te_cfg;
  te_cfg.add_disabled_categories("*");
  te_cfg.add_enabled_categories("demo");
  ds_cfg->set_track_event_config_raw(te_cfg.SerializeAsString());

  auto session = perfetto::Tracing::NewTrace();
  session->Setup(cfg);
  session->StartBlocking();
  return session;
}

void WriteTrace(const std::string& path,
                std::unique_ptr<perfetto::TracingSession> session) {
  perfetto::TrackEvent::Flush();
  session->StopBlocking();
  std::vector<char> data = session->ReadTraceBlocking();

  std::ofstream out(path, std::ios::binary | std::ios::out);
  if (!out) {
    std::cerr << "failed to open output file: " << path << '\n';
    std::exit(1);
  }
  out.write(data.data(), static_cast<std::streamsize>(data.size()));
  out.close();

  std::cout << "Wrote " << data.size() << " bytes to " << path << '\n';
  std::cout << "Open in https://ui.perfetto.dev or convert with traceconv\n";
}

void SimulateWork(const char* name, int iterations_ms) {
  TRACE_EVENT("demo", "SimulateWork", "name", name, "iterations_ms",
              iterations_ms);
  std::this_thread::sleep_for(std::chrono::milliseconds(iterations_ms));
}

}  // namespace

int main(int argc, char** argv) {
  const std::string out_path =
      (argc > 1) ? argv[1] : "pulseview_example.pftrace";

  InitPerfetto();
  auto session = StartTracing();

  {
    TRACE_EVENT("demo", "Main");
    TRACE_COUNTER("demo", "heartbeat", 1);

    SimulateWork("phase_a", 100);

    {
      TRACE_EVENT_BEGIN("demo", "nested_scope");
      SimulateWork("phase_b", 50);
      TRACE_COUNTER("demo", "heartbeat", 2);
      TRACE_EVENT_END("demo");
    }

    TRACE_EVENT_INSTANT("demo", "done");
  }

  WriteTrace(out_path, std::move(session));
  return 0;
}
