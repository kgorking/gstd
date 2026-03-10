TODO:
* Implement pipe support (windows and linux)
  * Will support the 'Reader' and 'Writer' concept
* Replace std::system with an 'os::exec' function.
  * Use pipes to read std io from process.
* Upgrade directory scanner. Return small struct containing info on the entry.
* Expand string support.
