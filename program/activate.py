import inspect
import sys
from os import environ, path


def my_path(x):
    return path.join(ROOT, x)


def activate():
    activator = my_path('.env/bin/activate_this.py')
    try:
        with open(activator) as f:
            code = compile(f.read(), activator, 'exec')
            exec(code, {'__file__': activator})
    except:
        raise Exception("Couldn't activate virtualenv")


ROOT = path.dirname(path.abspath(inspect.stack()[0][1]))
if ROOT not in sys.path:
    sys.path.insert(1, ROOT)
