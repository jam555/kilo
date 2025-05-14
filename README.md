Mila
===

Kilo is a small text editor in less than 1K lines of code (counted with cloc),
written by Salvatore Sanfilippo aka antirez and is released under the BSD 2
clause license. It doesn't depend on any library beyond the standard Unixy libc,
implementing terminal interfacing manually.
Mila is a derivative of Kilo, still under development, that seeks to take it in...
DISSIMILAR interface directions. "mila" is from the Basque language, where it
means "one thousand", making it a handy reference to Kilo. Unfortunately, I
didn't do enough research before choosing it as a name, so I'll likely be
renaming this project, maybe to "thou"; hopefully I'll manage to keep a reference
to Kilo in the name.


A screencast of Kilo is available here:
	https://asciinema.org/a/90r2i9bq8po03nazhqtsifksb

Usage: kilo `<filename>` `\[--no-alt-screen\]`

Keys:

    CTRL-S: Save
    CTRL-Q: Quit
    CTRL-F: Find string in file (ESC to exit search, arrows to navigate)

Mila adds formatting improvements to Kilo, both by using the alternate screen
available on many ANSI-terminals, and by putting in a bit more effort when the
alternate-screen is explicitly commanded against.
