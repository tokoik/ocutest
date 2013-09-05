LDLIBS = -L/usr/X11R6/lib -lglut -lGLU -lGL -L/opt/local/lib -lopencv_core -lopencv_highgui -lm
CXXFLAGS	= -I/usr/X11R6/include -I/opt/local/include -DX11 -Wall -g
LDLIBS	= libglfw_linux.a -L/usr/X11R6/lib -lX11 -lGL -L/opt/local/lib -lopencv_core -lopencv_highgui -lm -lXrandr -lrt -lpthread -lm
OBJECTS	= $(patsubst %.cpp,%.o,$(wildcard *.cpp))
TARGET	= ocutest

.PHONY: clean depend

$(TARGET): $(OBJECTS)
	$(LINK.cc) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	-$(RM) $(TARGET) *.o *~ .*~ core

depend:
	$(CXX) $(CXXFLAGS) -MM *.cpp > $(TARGET).dep

-include $(wildcard *.dep)
