CXX = g++-13
CXXFLAGS = -D _DEBUG -ggdb3 -std=c++17 -O0 -Wall -Wextra -Weffc++ -Waggressive-loop-optimizations	  \
		   -Wc++14-compat -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts 		  \
		   -Wconditionally-supported -Wconversion -Wctor-dtor-privacy -Wempty-body -Wfloat-equal      \
		   -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op \
		   -Wnon-virtual-dtor -Wopenmp-simd -Woverloaded-virtual -Wpacked -Wpointer-arith -Winit-self \
		   -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel 		  \
		   -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods 				  \
		   -Wsuggest-final-types -Wsuggest-override -Wswitch-default -Wswitch-enum -Wsync-nand 		  \
		   -Wundef -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-literal-suffix   \
		   -Wno-missing-field-initializers -Wno-narrowing -Wno-old-style-cast -Wno-varargs 			  \
		   -Wstack-protector -fcheck-new -fsized-deallocation -fstack-protector -fstrict-overflow 	  \
		   -flto-odr-type-merging -fno-omit-frame-pointer -Wlarger-than=8192 -Wstack-usage=8192 -pie  \
		   -fPIE -Werror=vla -I/Library/Frameworks/SDL2.framework/Headers -F/Library/Frameworks	  	  \
		   -framework SDL2

HOME = $(shell pwd)
CXXFLAGS += -I $(HOME)

TARGET = Differentiator/differentiator.exe
DOXYFILE = Others/Doxyfile

HEADERS  = Differentiator/MathExpressionsMain.h 	Differentiator/MathExpressionCalculations.h	\
		   Differentiator/MathExpressionInOut.h Differentiator/MathExpressionGnuPlot.h \
		   Differentiator/MathExpressionTexDump.h	Differentiator/DSL.h 				\
		   Differentiator/MathExpressionEquationRead.h 	\
		   Vector/ArrayFuncs.h Vector/HashFuncs.h Vector/Vector.h  Vector/Types.h \
		   Common/Log.h Common/Errors.h Common/Colors.h Common/StringFuncs.h Common/DoubleFuncs.h 	\
		   FastInput/InputOutput.h 	FastInput/StringFuncs.h

FILESCPP = Differentiator/MathExpressionsMain.cpp 	Differentiator/main.cpp \
		   Differentiator/MathExpressionCalculations.cpp Differentiator/MathExpressionInOut.cpp \
		   Differentiator/MathExpressionGnuPlot.cpp  Differentiator/MathExpressionTexDump.cpp 	\
		   Differentiator/DSL.cpp  Differentiator/MathExpressionEquationRead.cpp 	\
		   Vector/ArrayFuncs.cpp Vector/HashFuncs.cpp Vector/Vector.cpp \
		   Common/Log.cpp Common/Errors.cpp Common/StringFuncs.cpp Common/DoubleFuncs.cpp 	\
		   FastInput/InputOutput.cpp	FastInput/StringFuncs.cpp

objects = $(FILESCPP:%.cpp=%.o)

.PHONY: all docs clean buildDirs

all: $(TARGET)

$(TARGET): $(objects) 
	$(CXX) $^ -o $(TARGET) $(CXXFLAGS)

%.o : %.cpp $(HEADERS)
	$(CXX) -c $< -o $@ $(CXXFLAGS) 

docs: 
	doxygen $(DOXYFILE)

clean:
	rm -rf *.o
	rm -rf FastInput/*.o
	rm -rf Differentiator/*.o
	rm -rf Common/*.o
	rm -rf Vector/*.o


buildDirs:
	mkdir $(OBJECTDIR)
	mkdir $(PROGRAMDIR)