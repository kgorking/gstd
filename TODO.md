* Change the 'os/file.cppm' implementation to use operating system functions instead of 'std::fstream'
  * Put windows and linux implementations in separate files, fx. the windows version should go in 'os/windows/file.cppm'
  * This is required to support pipes (next step)

* Implement read/write pipe support via a function 'os::pipes' (windows and linux)
  * Will support the 'Reader' and 'Writer' concept
  * Should also support the 'LineReader' concept, and the yet non-existing 'LineWriter'.

* Add a 'os::exec' function to run a separate process, like std::system, but with access to the stdin/stdout pipes.
  * Use pipes to read std io from process.
