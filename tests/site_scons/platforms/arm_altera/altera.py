# Compiler settings for running the tests on arm-altera-eabi
# Requires following packages to be installed:
# arm-altera-eabi-gcc arm-altera-eabi-g++

def set_altera_platform(env):
    env.Replace(EMBEDDED = "ALTERA")
    env.Replace(CC  = "arm-altera-eabi-gcc",
                CXX = "arm-altera-eabi-g++")
    env.Append(LINKFLAGS = "-static")
    
