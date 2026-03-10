* Change the 'os/file.cppm' implementation to use operating system functions instead of 'std::fstream'
  * This is required to support pipes (next step)

* Implement pipe support via the function 'os::pipes' (windows and linux)
  * Will support the 'Reader' and 'Writer' concept

* Replace std::system with an 'os::exec' function.
  * Use pipes to read std io from process.
