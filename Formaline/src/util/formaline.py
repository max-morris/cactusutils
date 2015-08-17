# gdb extension script to extract formaline tarballs from Cactus executable
# Copyright (C) 2013 Roland Haas
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
""" Usage: gdb -P formaline.py exe/cactus_sim or gdb -x formaline.py exe/cactus_sim """
import io
import sys

if not gdb.inferiors(): # RedHat and Debian differ in how scripts are used 
  # for some reason gdb passes an empty string in argv[0] if no argument is given
  if(not sys.argv[0]):
    raise ValueError("needexactly one argument: gdb -P formaline.py exe/cactus_sim")
  # open executable and get handle to FOrmalin'e top-level source object
  gdb.execute("file %s" % sys.argv[0])

inferior = gdb.inferiors()[0]
cactus_source = gdb.parse_and_eval("cactus_source")

# replicate logic from output_source.c
# struct datainfo {
#   unsigned char const *data;
#   size_t length;
#   struct datainfo const *next;
# };
# 
# struct sourceinfo {
#   struct datainfo const *first;
#   char const *arrangement;
#   char const *thorn;
# };
count = 0
while(cactus_source[count] != 0):
  sourceinfo = cactus_source[count].dereference()
  fn = "Cactus-source-%s.tar.gz" % sourceinfo['thorn'].string()
  fh = io.open(fn, mode='wb')
  datainfo = sourceinfo['first']
  while(datainfo != 0):
    data = inferior.read_memory(datainfo['data'], datainfo['length'])
    fh.write(data)
    datainfo = datainfo['next']
  fh.close()
  count += 1
