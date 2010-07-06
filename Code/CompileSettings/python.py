import os
import os.path
import SCons.Warnings

class PythonNotFound(SCons.Warnings.Warning):
    pass
SCons.Warnings.enableWarningClass(PythonNotFound)

def generate(env):
    path = os.environ.get('OPTICKSDEPENDENCIES',None)
    if path:
       path = os.path.join(path, "python")
    if not path:
       SCons.Warnings.warn(PythonFound,"Could not detect Python")
    else:
       if env["OS"] == "windows":
           lib = "python%s" % (env["PYTHONVERSION"])
       else:
           lib = 'python%s.%s' % tuple(env['PYTHONVERSION'])
       env.AppendUnique(CXXFLAGS= ["-I%s/include/%s" % (path,env['PYTHONVERSION']), "-I%s/include/%s/%s" % (path,env['PYTHONVERSION'],env['OPTICKSPLATFORM'])],
                        LIBPATH=['%s/lib/%s' % (path,env['OPTICKSPLATFORM'])],
                        LIBS=[lib])

def exists(env):
    return env.Detect('python')
