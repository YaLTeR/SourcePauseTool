Download flatc.exe from https://github.com/google/flatbuffers/releases.
Run the following command in this directory to generate C++ code from a schema:

flatc.exe --cpp --cpp-std c++17 YOUR_SCHEMA.fbs

Read up https://flatbuffers.dev/languages/cpp/ for usage.
You can quickly convert to JSON with:

flatc.exe --json --raw-binary <YOUR_SCHEMA> --root-type <YOUR_ROOT_TYPE> -- <YOUR_BINARY_FILE>
