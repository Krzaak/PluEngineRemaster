import os
import platform


def modify_date(path_to_file):
    """
    Zwraca Unix timestamp ostatniej modyfikacji pliku.
    """
    return os.path.getmtime(path_to_file)
