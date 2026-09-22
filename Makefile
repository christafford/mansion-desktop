CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O0

all: mansion-desktop-placeholder

mansion-desktop-placeholder: src/main.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -DPLACEHOLDER

clean:
	rm -f mansion-desktop mansion-desktop-placeholder

.PHONY: clean

# Full build (will fail until dependencies are installed)
mansion-desktop: src/main.cpp src/compositor.cpp src/display.cpp src/input.cpp src/launch.cpp
	$(CXX) $(CXXFLAGS) $^ -o $@ -lwayland-server -lwayland-client -lwayland-egl -lEGL -lGL -lxkbcommon

