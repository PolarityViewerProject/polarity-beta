# -*- cmake -*-
include(Prebuilt)

set(Boost_FIND_QUIETLY ON)
set(Boost_FIND_REQUIRED ON)

if (USESYSTEMLIBS)
  include(FindBoost)

  set(BOOST_CONTEXT_LIBRARY boost_context-mt)
  set(BOOST_COROUTINE_LIBRARY boost_coroutine-mt)
  set(BOOST_FILESYSTEM_LIBRARY boost_filesystem-mt)
  set(BOOST_PROGRAM_OPTIONS_LIBRARY boost_program_options-mt)
  set(BOOST_REGEX_LIBRARY boost_regex-mt)
  set(BOOST_SIGNALS_LIBRARY boost_signals-mt)
  set(BOOST_SYSTEM_LIBRARY boost_system-mt)
  set(BOOST_THREAD_LIBRARY boost_thread-mt)
  set(BOOST_WAVE_LIBRARY boost_wave-mt)
else (USESYSTEMLIBS)
  use_prebuilt_binary(boost)
  set(Boost_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include)
  set(BOOST_VERSION "1.60")
  add_definitions(-DBOOST_ALL_NO_LIB)

  if (WINDOWS)
    set(BOOST_CHRONO_LIBRARY
        optimized libboost_chrono-mt
        debug libboost_chrono-mt-gd)
    set(BOOST_CONTEXT_LIBRARY
        optimized libboost_context-mt
        debug libboost_context-mt-gd)
    set(BOOST_COROUTINE_LIBRARY
        optimized libboost_coroutine-mt
        debug libboost_coroutine-mt-gd)
    set(BOOST_FILESYSTEM_LIBRARY
        optimized libboost_filesystem-mt
        debug libboost_filesystem-mt-gd)
    set(BOOST_PROGRAM_OPTIONS_LIBRARY
        optimized libboost_program_options-mt
        debug libboost_program_options-mt-gd)
    set(BOOST_REGEX_LIBRARY
        optimized libboost_regex-mt
        debug libboost_regex-mt-gd)
    set(BOOST_SIGNALS_LIBRARY
        optimized libboost_signals-mt
        debug libboost_signals-mt-gd)
    set(BOOST_SYSTEM_LIBRARY
        optimized libboost_system-mt
        debug libboost_system-mt-gd)
    set(BOOST_THREAD_LIBRARY
        optimized libboost_thread-mt
        debug libboost_thread-mt-gd)
    set(BOOST_WAVE_LIBRARY 
        optimized libboost_wave-mt
        debug libboost_wave-mt-gd)
  elseif (LINUX)
    set(BOOST_CHRONO_LIBRARY
        optimized boost_chrono-mt
        debug boost_chrono-mt-d)
    set(BOOST_CONTEXT_LIBRARY
        optimized boost_context-mt
        debug boost_context-mt-d)
    set(BOOST_COROUTINE_LIBRARY
        optimized boost_coroutine-mt
        debug boost_coroutine-mt-d)
    set(BOOST_FILESYSTEM_LIBRARY
        optimized boost_filesystem-mt
        debug boost_filesystem-mt-d)
    set(BOOST_PROGRAM_OPTIONS_LIBRARY
        optimized boost_program_options-mt
        debug boost_program_options-mt-d)
    set(BOOST_REGEX_LIBRARY
        optimized boost_regex-mt
        debug boost_regex-mt-d)
    set(BOOST_SIGNALS_LIBRARY
        optimized boost_signals-mt
        debug boost_signals-mt-d)
    set(BOOST_SYSTEM_LIBRARY
        optimized boost_system-mt
        debug boost_system-mt-d)
    set(BOOST_THREAD_LIBRARY
        optimized boost_thread-mt
        debug boost_thread-mt-d)
    set(BOOST_WAVE_LIBRARY 
        optimized boost_wave-mt
        debug boost_wave-mt-d)
  elseif (DARWIN)
    set(BOOST_CHRONO_LIBRARY
        optimized boost_chrono-mt
        debug boost_chrono-mt-d)
    set(BOOST_CONTEXT_LIBRARY
        optimized boost_context-mt
        debug boost_context-mt-d)
    set(BOOST_COROUTINE_LIBRARY
        optimized boost_coroutine-mt
        debug boost_coroutine-mt-d)
    set(BOOST_FILESYSTEM_LIBRARY
        optimized boost_filesystem-mt
        debug boost_filesystem-mt-d)
    set(BOOST_PROGRAM_OPTIONS_LIBRARY
        optimized boost_program_options-mt
        debug boost_program_options-mt-d)
    set(BOOST_REGEX_LIBRARY
        optimized boost_regex-mt
        debug boost_regex-mt-d)
    set(BOOST_SIGNALS_LIBRARY
        optimized boost_signals-mt
        debug boost_signals-mt-d)
    set(BOOST_SYSTEM_LIBRARY
        optimized boost_system-mt
        debug boost_system-mt-d)
    set(BOOST_THREAD_LIBRARY
        optimized boost_thread-mt
        debug boost_thread-mt-d)
    set(BOOST_WAVE_LIBRARY 
        optimized boost_wave-mt
        debug boost_wave-mt-d)
  endif (WINDOWS)
endif (USESYSTEMLIBS)

if (LINUX)
    set(BOOST_SYSTEM_LIBRARY ${BOOST_SYSTEM_LIBRARY} rt)
    set(BOOST_THREAD_LIBRARY ${BOOST_THREAD_LIBRARY} rt)
endif (LINUX)

