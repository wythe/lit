#pragma once

#include <boost/log/trivial.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

BOOST_LOG_GLOBAL_LOGGER(logger, boost::log::sources::severity_logger_mt<boost::log::trivial::severity_level>)

#define log_trace   BOOST_LOG_TRIVIAL(trace)
#define log_debug   BOOST_LOG_TRIVIAL(debug)
#define log_info    BOOST_LOG_TRIVIAL(info)
#define log_warning BOOST_LOG_TRIVIAL(warning)
#define log_error   BOOST_LOG_TRIVIAL(error)
#define log_fatal   BOOST_LOG_TRIVIAL(fatal)
