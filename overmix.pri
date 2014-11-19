# Libraries used
QT += concurrent
LIBS += -lfftw3-3 -lm -lpng -lz -llzma

# C++11 support
QMAKE_CXXFLAGS += -std=c++11

# Core logic
HEADERS += $$PWD/src/MultiPlaneIterator.hpp $$PWD/src/color.hpp $$PWD/src/Deteleciner.hpp
SOURCES += $$PWD/src/MultiPlaneIterator.cpp $$PWD/src/color.cpp $$PWD/src/Deteleciner.cpp

# Containers
HEADERS += $$PWD/src/containers/AContainer.hpp $$PWD/src/containers/ImageContainer.hpp $$PWD/src/containers/ImageGroup.hpp $$PWD/src/containers/DelegatedContainer.hpp
SOURCES += $$PWD/src/containers/AContainer.cpp $$PWD/src/containers/ImageContainer.cpp

# Container saver
HEADERS += $$PWD/src/containers/ImageContainerSaver.hpp
SOURCES += $$PWD/src/containers/ImageContainerSaver.cpp
# pugixml
HEADERS += $$PWD/src/containers/pugixml/pugixml.hpp $$PWD/src/containers/pugixml/pugiconfig.hpp
SOURCES += $$PWD/src/containers/pugixml/pugixml.cpp

# Planes
HEADERS += $$PWD/src/planes/ImageEx.hpp $$PWD/src/planes/Plane.hpp $$PWD/src/planes/FourierPlane.hpp $$PWD/src/planes/PlaneBase.hpp
SOURCES += $$PWD/src/planes/ImageEx.cpp $$PWD/src/planes/Plane.cpp $$PWD/src/planes/FourierPlane.cpp
SOURCES += $$PWD/src/planes/Plane-scaling.cpp $$PWD/src/planes/Plane-edgedetection.cpp $$PWD/src/planes/Plane-blurring.cpp $$PWD/src/planes/Plane-diff.cpp $$PWD/src/planes/Plane-pixel.cpp $$PWD/src/planes/Plane-binarize.cpp

# Aligners
HEADERS += $$PWD/src/aligners/AImageAligner.hpp $$PWD/src/aligners/AverageAligner.hpp $$PWD/src/aligners/ImageAligner.hpp $$PWD/src/aligners/LayeredAligner.hpp $$PWD/src/aligners/RecursiveAligner.hpp $$PWD/src/aligners/AnimationSeparator.hpp $$PWD/src/aligners/FrameAligner.hpp
SOURCES += $$PWD/src/aligners/AImageAligner.cpp $$PWD/src/aligners/AverageAligner.cpp $$PWD/src/aligners/ImageAligner.cpp $$PWD/src/aligners/LayeredAligner.cpp $$PWD/src/aligners/RecursiveAligner.cpp $$PWD/src/aligners/AnimationSeparator.cpp $$PWD/src/aligners/FrameAligner.cpp

# Renders
HEADERS += $$PWD/src/renders/AverageRender.hpp $$PWD/src/renders/SimpleRender.hpp $$PWD/src/renders/FloatRender.hpp $$PWD/src/renders/DiffRender.hpp $$PWD/src/renders/PlaneRender.hpp $$PWD/src/renders/StatisticsRender.hpp $$PWD/src/renders/ARender.hpp
SOURCES += $$PWD/src/renders/AverageRender.cpp $$PWD/src/renders/SimpleRender.cpp $$PWD/src/renders/FloatRender.cpp $$PWD/src/renders/DiffRender.cpp $$PWD/src/renders/PlaneRender.cpp $$PWD/src/renders/StatisticsRender.cpp

# mics
HEADERS += $$PWD/src/debug.hpp $$PWD/src/aligners/AnimationSaver.hpp $$PWD/src/planes/dump/DumpPlane.hpp $$PWD/src/ARenderPipe.hpp $$PWD/src/RenderOperations.hpp $$PWD/src/Geometry.hpp $$PWD/src/utils/utils.hpp
SOURCES += $$PWD/src/debug.cpp $$PWD/src/aligners/AnimationSaver.cpp $$PWD/src/planes/dump/DumpPlane.cpp
