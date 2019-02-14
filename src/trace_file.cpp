#include <cstring>
#include <iostream>
#include <trace_file.h>

static auto ios_open_mode (TraceFileMode mode)
{
    switch (mode)
    {
    case TraceFileMode::READ:
        return std::ios::in;

    case TraceFileMode::WRITE:
        return std::ios::out;

    default:
        throw std::invalid_argument ("Unsupported open mode.");
    }
}

TraceFile::TraceFile (const std::string &file, TraceFileMode mode)
{
    auto ios_mode = ios_open_mode(mode);
    file_.open(file, ios_mode | std::ios::binary);
}

TraceFile::~TraceFile ()
{
    file_.close();
}

void TraceFile::write (const EventBuffer &event_buffer)
{
    TraceMetaData md(event_buffer);
    write_meta_data(md);

    auto array_range = event_buffer.data.array_one();
    write_raw_data((char *)array_range.first,
                   array_range.second * sizeof(AccessEvent));

    array_range = event_buffer.data.array_two();
    write_raw_data((char *)array_range.first,
                    array_range.second * sizeof(AccessEvent));
}

AccessSequence TraceFile::read ()
{
    // TODO implement
    return AccessSequence ();
}

void TraceFile::write_meta_data(const TraceMetaData & md)
{
    file_ << tag_;
    file_.write((char *) &md, sizeof(TraceMetaData));
}

void TraceFile::write_raw_data(const char * data, size_t nbytes)
{
    if (nbytes > 0)
{
        file_.write(data, nbytes);
    }
}

void TraceFile::read_meta_data(TraceMetaData * md)
{
    constexpr std::size_t tag_len = tag_.size();
    char tag_buffer[tag_len];
    file_.read(tag_buffer, sizeof(char) * tag_len);
    if(tag_ != tag_buffer)
    {
        throw std::runtime_error("Trace does not contain the correct tag at the beginning.");
    }
    file_.read((char *) md, sizeof(TraceMetaData));
}


void * TraceFile::read_raw_data(const TraceMetaData & md)
{
    return nullptr;
}

