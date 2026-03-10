* Change the 'os/file.cppm' implementation to use operating system functions instead of 'std::fstream'
  * Put windows and linux implementations in seperate files, fx. the windows version should go in 'os/windows/file.cppm'
  * This is required to support pipes (next step)

* Implement read/write pipe support via a function 'os::pipes' (windows and linux)
  * Will support the 'Reader' and 'Writer' concept
  * Should also support the 'LineReader' concept, and the yet non-existing 'LineWriter'.

* Replace std::system with an 'os::exec' function.
  * Use pipes to read std io from process.
