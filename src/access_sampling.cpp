#include <access_sampling.h>
#include <sstream>
#include <trace_file.h>
#include <utils.h>

access_sampling::access_sampling () : perf_sampling_ (), pfm_wrapper_ ()
{
    std::cout << "Loading Metric Plugin\n";
}

std::vector<MetricProperty> access_sampling::get_metric_properties (const std::string &metric_name)
{
    std::cout << "get metric properties called with: " << metric_name << '\n';

    std::vector<MetricProperty> metric_properties;

    auto [event, dummy] = parse_metric (metric_name);

    if (pfm_wrapper_.is_metric_supported (event))
    {
        metric_properties.push_back (MetricProperty (metric_name, "", "Address").absolute_point ().value_uint ());
    }

    return metric_properties;
}

int32_t access_sampling::add_metric (const std::string &metric)
{
    auto [event, period] = parse_metric (metric);
    PerfEventAttribute perf_event_attr;
    pfm_wrapper_.get_perf_event (event, period, &perf_event_attr);

    perf_sampling_.event_open (&perf_event_attr);

    buffer_mutex_.lock ();
    auto tid = std::this_thread::get_id ();
    auto buffer_iter = thread_event_buffers_.find (tid);
    if (buffer_iter == thread_event_buffers_.end ())
    {
        thread_event_buffers_[tid] = perf_sampling_.get_event_buffer ();
    }

    int32_t id = all_events_.size ();
    all_events_.push_back (std::make_tuple (tid, event));
    buffer_mutex_.unlock ();

    return id;
}

void access_sampling::start ()
{
    perf_sampling_.enable ();
}

inline void start_write_worker (const std::thread::id &tid, const EventBuffer &event_buffer)
{
    std::stringstream ss;
    ss << "trace." << convert_thread_id (tid) << ".bin";
    TraceFile f (ss.str (), TraceFileMode::WRITE);
    f.write (event_buffer);
}

void access_sampling::stop ()
{


    perf_sampling_.disable ();
    std::vector<std::thread> workers;
    auto trace_path = create_trace_directory ();

    for (auto [tid, buffer] : thread_event_buffers_)
    {
        workers.push_back (std::thread (start_write_worker, std::ref (tid), std::ref (*buffer)));
    }
    for (auto &w : workers)
    {
        if (w.joinable ())
        {
            w.join ();
        }
    }
}

SCOREP_METRIC_PLUGIN_CLASS (access_sampling, "access_sampling")