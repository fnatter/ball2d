VPATH = .:../incl

%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(MY_CXXFLAGS) $(CXXFLAGS) -c $< -o $@

%.o: %.asm
	$(NASM) -f coff $< -o $@

# automatic generation of dependencies
# below is the unix-version, but it doesn't seem to work with cygwin 20.1b
# the uncommented version below doesn't use unix-tools,
# the same thing is achieved with a c-program (../util/chdep.c)
# look at ../utils/chdep.c and the GNU make manual,
#  page 30, for more information

#%.d: %.cpp
#	$(BASH) -ec '$(CXX) $(CPPFLAGS) -I$(ROOT)/incl -MM $< \
#	| sed '\''s/\($*\)\.o[ :]*/\1.o $@ : /g'\'' > $@; \
#	[ -s $@ ] || rm -f $@'

%.d: %.cpp
	$(CXX) $(CPPFLAGS) -MM -I$(INCLDIR) $< > $@; \
	..\util\chdep.exe $@ dummy

%.d: %.asm # this generates a simple line: sth.o sth.d: sth.asm
	echo $* $@: $< > $@ 


