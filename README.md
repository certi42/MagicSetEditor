# Magic Set Editor for Mac

## About

Magic Set Editor is a tool that can be used to create custom Magic the Gathering cards, originally located on [SourceForge](http://magicseteditor.sourceforge.net).

The goal of this project is to modify the existing source code to run on MacOS in addition to Windows and Linux systems, which are already supported.

## Building

First, install the dependencies. You can build these from source, but installing with Homebrew is probably easier.
```bash
brew install wxwidgets boost hunspell
```
Build the project with `cmake`. To generate Xcode project files, use:
```bash
cmake -G Xcode .
```

## Changes

 - The `wxDialUpManager` isn't implemented on Mac, so the `only when connected` setting for checking for updates will behave identically to the `always` setting (`gui/update_checker.cpp:134`).
 - The `wxZipInputStream(const wxString&, const wxString&)` constructor has been removed, and so seeking has to be performed manually (`util/io/package.cpp:228`).
 - The wxString `begin` and `end` functions seem to return a `wxUniCharRef` type, whcih isn't compatible with various string iterator operations. To resolve this wxStrings are converted to `std::string`s when necessary, then converted back once the operation is complete.

## Dependencies

- [Boost](http://boost.org)
- [wxWidgets](http://wxwidgets.org/)
- [Hunspell](http://hunspell.sourceforge.net/)

