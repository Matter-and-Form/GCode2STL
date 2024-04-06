#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <random>
#include <string>

// Define a simple structure for a 3D point.
class Point3D {
public:
    float x, y, z, e;

    Point3D() : x(0), y(0), z(0), e(0) {}
    Point3D(float x, float y, float z) : x(x), y(y), z(z) {}

    // Vector subtraction
    Point3D operator-(const Point3D& other) const {
        return Point3D(x - other.x, y - other.y, z - other.z);
    }

    // Scalar multiplication
    Point3D operator*(float scalar) const {
        return Point3D(x * scalar, y * scalar, z * scalar);
    }

    // Vector addition
    Point3D operator+(const Point3D& other) const {
        return Point3D(x + other.x, y + other.y, z + other.z);
    }

    bool operator==(const Point3D& other) const {
        return (x == other.x && y == other.y && z == other.z);
    }

    // Normalize the vector
    Point3D normalize() const {
        float len = sqrt(x * x + y * y + z * z);
        return Point3D(x / len, y / len, z / len);
    }

    // Cross product
    Point3D cross(const Point3D& other) const {
        return Point3D(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
    }
};

// Function to parse a single line of GCode and extract the point if extruding.
bool parseGCodeLine(const std::string& line, Point3D& point) {
    if (line.empty() || line[0] != 'G') return false;

    // Basic parsing to find if this is a G1 command
    if (line.find("G1") != std::string::npos || line.find("G0") != std::string::npos) {
        std::istringstream iss(line);
        char command;
        while (iss >> command) {
            switch (command) {
                case 'X': iss >> point.x; break;
                case 'Y': iss >> point.y; break;
                case 'Z': iss >> point.z; break;
                case 'E': iss >> point.e; break;
                default: iss.ignore(256, ' '); break; // Skip unknown commands/values
            }
        }
        return true;
    }
    return false;
}

bool checkG92(const std::string& line, float &currentExtrusion)
{
    if (line.empty() || line[0] != 'G') return false;


    // Basic parsing to find if this is a G1 command
    if (line.find("G92") != std::string::npos && line.find("E") != std::string::npos) {
        std::istringstream iss(line);
        char command;
        while (iss >> command) {
            switch (command) {
                case 'E': iss >> currentExtrusion; 
                return true;
                break;
                default: iss.ignore(256, ' '); break; // Skip unknown commands/values
            }
        }
    }
    return false;
}

std::vector<Point3D> createOrthogonalSquares(const std::vector<Point3D>& path, float extrusionHalfWidth) {
    std::vector<Point3D> squares;

    if(path.size() < 2) return squares; // Need at least two points to define a direction

    for(size_t i = 0; i < path.size(); ++i) {
        Point3D direction;
        if(i == 0) {
            // For the first point, the direction is towards the next point
            direction = (path[1] - path[0]).normalize();
        } 
        else if (i == path.size()-1)
        {
            direction = (path[i] - path[i - 1]).normalize();
        }
        else 
        {
            // For subsequent points, the direction is from the previous point
            direction = ((path[i] - path[i - 1]) + (path[i + 1] - path[i])).normalize();
        }

        // Create two orthogonal vectors
        Point3D up(0, 0, 1); // Assuming Z-up coordinate system
        Point3D right = direction.cross(up).normalize(); // Orthogonal to direction and 'up'

        // Generate the four points of the square
        squares.push_back(path[i] + right * extrusionHalfWidth + up * extrusionHalfWidth);
        squares.push_back(path[i] - right * extrusionHalfWidth + up * extrusionHalfWidth);
        squares.push_back(path[i] - right * extrusionHalfWidth - up * extrusionHalfWidth);
        squares.push_back(path[i] + right * extrusionHalfWidth - up * extrusionHalfWidth);
    }

    return squares;
}

std::vector<Point3D> cleanLayer(const std::vector<Point3D>& layer)
{
    std::vector<Point3D> newLayer;
    Point3D previous = layer[0] + Point3D(1,0,0);
    for(auto p : layer) {
        if (previous == p)
        {
            continue;
        }
        newLayer.push_back(p);
        previous = p;
        
    }

    return newLayer;
}

// Function to read a GCode file and extract 3D points of extrusion paths.
std::vector<std::vector<Point3D>> extract3DPointsFromGCode(const std::string& filePath) {
    std::vector<std::vector<Point3D>> layers;
    std::ifstream file(filePath);
    std::string line;
    Point3D currentPoint{0, 0, 0};
    Point3D previousPoint{0, 0, 0};
    float maxE = 0;
    bool previousNoExtrude = true;

    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return layers;
    }

    std::vector<Point3D> points;
    size_t lineNum = 0;

    while (std::getline(file, line)) {
        lineNum++;
        if (line[0] == ';')
            continue;
        if (checkG92(line, maxE))
        {
            currentPoint.e = maxE;
        }
        if (parseGCodeLine(line, currentPoint)) {
            bool isExtruding = (currentPoint.e > maxE);
            bool didMove = !(currentPoint == previousPoint);
            maxE = std::max(currentPoint.e, maxE);

            // Every move is returned G0 or G1.
            // Check to see if an extrude move is being made and the previous move was a G0
            if (isExtruding == false)
            {
                previousNoExtrude = true;
            }

            // Check if the heights change. Start a new layer if they do
            if (currentPoint.z != previousPoint.z)
            {   
                if (points.size() > 0)
                {
                    points = cleanLayer(points);
                    layers.push_back(createOrthogonalSquares(points, (currentPoint.z- previousPoint.z) / 2.0f));
                    points = std::vector<Point3D>();
                }
                previousNoExtrude = true;
            }
            else if (isExtruding && didMove)
            {
                if (previousNoExtrude)
                {
                    previousNoExtrude = false;
                    points.push_back(previousPoint);
                }
                points.push_back(currentPoint);
                
            }
            
            
            previousPoint = currentPoint;
        }
    }

    return layers;
}





void savePointsToPLY(const std::vector<std::vector<Point3D>>& layers, const std::string& filePath) {
    std::ofstream plyFile(filePath, std::ios::out);

    if (!plyFile.is_open()) {
        std::cerr << "Failed to create PLY file: " << filePath << std::endl;
        return;
    }

    size_t count = 0;
    for (auto points : layers)
    {
        count += points.size();
    }

    size_t faceCount = count -4;
    

    // Write the PLY header
    plyFile << "ply\n";
    plyFile << "format ascii 1.0\n";
    plyFile << "element vertex " << count << "\n";
    plyFile << "property float x\n";
    plyFile << "property float y\n";
    plyFile << "property float z\n";

    plyFile << "element face " << faceCount << "\n";
    plyFile << "property list uchar int vertex_index\n";
    plyFile << "end_header\n";


    
    // Write the points
    for (auto points : layers)
    {
        for (const auto& point : points) {
            plyFile << point.x << " " << point.y << " " << point.z << "\n";
        }
    }
    // Write the faces
    for (size_t i = 0; i < count - 4; i += 4)
    {
        plyFile << "4 " << i+0 << " " << i+4 << " " << i+5 << " " << i+1 << "\n";
        plyFile << "4 " << i+1 << " " << i+5 << " " << i+6 << " " << i+2 << "\n";
        plyFile << "4 " << i+2 << " " << i+6 << " " << i+7 << " " << i+3 << "\n";
        plyFile << "4 " << i+3 << " " << i+7 << " " << i+4 << " " << i+0 << "\n";
        
    }

    plyFile.close();
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <GCodeFilePath> <PLYFilePath>" << std::endl;
        return 1;
    }
    std::string filePath = argv[1];
    std::string plyFilePath = argv[2];
    auto layers = extract3DPointsFromGCode(filePath);

    savePointsToPLY(layers, plyFilePath);

    std::cout << "Saved " << layers.size() << " layers to PLY file: " << plyFilePath << std::endl;

    return 0;
}
