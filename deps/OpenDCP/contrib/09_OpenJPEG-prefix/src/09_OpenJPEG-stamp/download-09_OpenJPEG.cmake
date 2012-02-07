message(STATUS "downloading...
     src='http://opendcp.googlecode.com/files/openjpeg_v1_4_sources_r697.tar.gz'
     dst='/home/carl/src/OpenDCP/contrib/09_OpenJPEG-prefix/src/openjpeg_v1_4_sources_r697.tar.gz'
     timeout='none'")

file(DOWNLOAD
  "http://opendcp.googlecode.com/files/openjpeg_v1_4_sources_r697.tar.gz"
  "/home/carl/src/OpenDCP/contrib/09_OpenJPEG-prefix/src/openjpeg_v1_4_sources_r697.tar.gz"
  SHOW_PROGRESS
  # no EXPECTED_MD5
  # no TIMEOUT
  STATUS status
  LOG log)

list(GET status 0 status_code)
list(GET status 1 status_string)

if(NOT status_code EQUAL 0)
  message(FATAL_ERROR "error: downloading 'http://opendcp.googlecode.com/files/openjpeg_v1_4_sources_r697.tar.gz' failed
  status_code: ${status_code}
  status_string: ${status_string}
  log: ${log}
")
endif()

message(STATUS "downloading... done")
