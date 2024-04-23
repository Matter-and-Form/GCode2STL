# GCode to STL/PLY Converter
## Overview
This repository contains a C++ application designed to convert GCode files into 3D printable STL or PLY files. It interprets the paths in the GCode as extruded lines, which are then rendered as square tubes using specified layer height and extrusion width parameters. This tool is especially useful for visualizing GCode paths in 3D modeling software or verifying the tool paths before 3D printing.

## Features
Convert GCode to STL or PLY: Outputs either STL or PLY files based on the paths defined in a GCode file.
Extruded Lines as Square Tubes: Converts the extruded lines into square tubes, making use of the layer height and a user-definable extrusion width.
No Dependencies: This project does not rely on any external libraries, making it easy to build and run on any platform supporting C++.

## Usage
To use this tool, you need to specify the path to the input GCode file, the output file path for the STL or PLY file, and optionally the extrusion width (default is 0.4mm).

```cpp
./bin/GCode2STL <GCodeFilePath> <OutputFilePath> [extrusionWidth=0.4]
```
### Parameters
* <GCodeFilePath>: Path to the GCode file you want to convert.
* <OutputFilePath>: Path where the converted file will be saved. The extension of this file (.stl or .ply) determines the output format.
* <extrusionWidth> (optional): The width of the extrusion, defaulting to 0.4mm if not specified.

## Building the Project
This project requires a C++ compiler (such as g++ or clang). To build the project:

Use VSCode or CMake to build the project.

```
cd GCode2STL
mkdir build
cd build
cmake ..
make
```

## Contributing
Contributions are what make the open-source community such an amazing place to learn, inspire, and create. Any contributions you make are greatly appreciated.

If you would like to contribute to the project, please:

Fork the repository.
Create your Feature Branch (git checkout -b feature/AmazingFeature).
Commit your Changes (git commit -m 'Add some AmazingFeature').
Push to the Branch (git push origin feature/AmazingFeature).
Open a Pull Request.

## License
This project is open source and available under the MIT License.
