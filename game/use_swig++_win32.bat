%SWIG% -lua -c++ ego.i
del ego_wrap.cpp
ren ego_wrap.cxx ego_wrap.cpp