import os
import os.path
import SCons.Warnings

class PythonNotFound(SCons.Warnings.Warning):
    pass
SCons.Warnings.enableWarningClass(PythonNotFound)

def generate(env):
    path = env['OPTICKSDEPENDENCIESINCLUDE']
    if not path:
       SCons.Warnings.warn(PythonFound,"Could not detect Python")
    else:
       if env["OS"] == "windows":
           lib = "python%s" % (env["PYTHONVERSION"])
       else:
           lib = 'python%s.%s' % tuple(env['PYTHONVERSION'])
       env.AppendUnique(CXXFLAGS= ["-I%s/%s" % (path,env['PYTHONVERSION'])],
                        LIBS=[lib])

def exists(env):
    return env.Detect('python')
