LaunchAndCloseFirefox by Denis GAUTIER

This console program launches Firefox and then closes it as a user would do.
The goals are:
- Create the Firefox user profile (if not alredy created): this will allow to apply some custom settings in the prefs.js file, etc.
- Trigger the "first-run-only" Firefox default pages like "Welcome to Firefox", "Firefox privacy notice", etc. so that they won't be displayed again when a real user will launch Firefox.
This tool is typically used in a new user profile customization script.

1) Firefox is launched, unless it is already running. 
The command to launch Firefox MUST be provided as the first argument. Typically: "C:\Program Files\Mozilla Firefox\Firefox.exe"

2) As long as a Firefox process is running, the first Firefox's "main window" process that we find, is activated and then {Alt}+{F4} and {ENTER} keyboard keys are sent. This is what a user can do to close Firefox, that's why it works: Killing or terminating the process could lead to an incomplete Firefox profile or the "first-run-only" Firefox default pages displayed again next time the user launches Firefox.
There may be several Firefox windows running, so this is repeated as long as we find a Firefox process with a "main window". Of course, there is a time out delay.

3) Exit Code 0 is returned in case of success. In case of error the returned exit code is 1 or higher.
Log lines are sent to stdout and sterr.

Known issue: this program may fail trying to "activate" Firefox, i.e. set Firefox as the foreground app., if another application is always reset at the foreground. The exit code will not be 0 so teh failure can be detected.
