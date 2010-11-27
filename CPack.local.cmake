# ====================================
# cpack settings
# ====================================
set (CPACK_PACKAGE_VERSION_MAJOR  ${PACKAGE_VERSION_MAJOR})
set (CPACK_PACKAGE_VERSION_MINOR  ${PACKAGE_VERSION_MINOR})
set (CPACK_PACKAGE_VERSION_PATCH  ${PACKAGE_VERSION_PATCH})
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "Kcov is a code coverage tester based on Bcov")
set (CPACK_PACKAGE_DESCRIPTION_FILE  "${CMAKE_CURRENT_SOURCE_DIR}/README")
set (CPACK_RESOURCE_FILE_README  "${CMAKE_CURRENT_SOURCE_DIR}/README")
set (CPACK_PACKAGE_VENDOR  "Simon Kagstrom <simon.kagstrom@gmail.com>")
set (CPACK_STRIP_FILES  TRUE)
set (CPACK_SOURCE_IGNORE_FILES
  /Debug/  /Release/  /build/
  /.git/  /.gitignore
  .*~  .*.log
)

include (CPack)
