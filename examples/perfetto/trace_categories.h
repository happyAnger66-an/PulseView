#ifndef PULSEVIEW_PERFETTO_EXAMPLE_TRACE_CATEGORIES_H_
#define PULSEVIEW_PERFETTO_EXAMPLE_TRACE_CATEGORIES_H_

#include <perfetto.h>

PERFETTO_DEFINE_CATEGORIES(
    perfetto::Category("demo").SetDescription("PulseView perfetto example"));

#endif  // PULSEVIEW_PERFETTO_EXAMPLE_TRACE_CATEGORIES_H_
