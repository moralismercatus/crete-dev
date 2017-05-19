Instructions for preparing a new version of Boost for inclusion into the CRETE repo.

1. Download the desired version of Boost.
2. Extract.
3. Remove documentation to reduce size. From inside the boost directory: ```find . -type d -name "doc" -exec rm -rf {} \;```
4. Compress and repackage appropriately.
5. Modify CMake files in lib/boost and front-end/guest/lib/boost as needed.
