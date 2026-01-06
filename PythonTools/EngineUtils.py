# Source - https://stackoverflow.com/a
# Posted by Mark Amery, modified by community. See post 'Timeline' for change history
# Retrieved 2026-01-05, License - CC BY-SA 4.0

import os
import platform
from statx import statx

def creation_date(path_to_file):
    """
    Try to get the Unix timestamp that a file was created, falling back to when
    it was last modified if that isn't possible.
    See http://stackoverflow.com/a/39501288/1709587 for explanation.
    """
    if platform.system() == 'Windows':
        return os.path.getctime(path_to_file)
    else:
        stat = os.stat(path_to_file)
        try:
            return stat.st_birthtime
        except AttributeError:
            # We're probably on Linux. Hopefully, we are on a recent enough
            # version that we can use the statx syscall. (If we are not, btime
            # below will be `None`.)
            btime = statx(path_to_file).btime
            if btime:
                return btime

    # If we've made it this far, all our efforts have failed. Fall back to
    # returning last-modified time as the closest available alternative:
    return os.path.getmtime(path_to_file)
