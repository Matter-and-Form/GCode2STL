#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <random>
#include <regex>
#include <string>

// Define a simple structure for a 3D point.
class Point3D
{
public:
    float x, y, z, e;

    Point3D() : x(0), y(0), z(0), e(0) {}
    Point3D(float x, float y, float z) : x(x), y(y), z(z) {}

    // Vector subtraction
    Point3D operator-(const Point3D &other) const
    {
        return Point3D(x - other.x, y - other.y, z - other.z);
    }

    // Scalar multiplication
    Point3D operator*(float scalar) const
    {
        return Point3D(x * scalar, y * scalar, z * scalar);
    }

    // Vector addition
    Point3D operator+(const Point3D &other) const
    {
        return Point3D(x + other.x, y + other.y, z + other.z);
    }

    bool operator==(const Point3D &other) const
    {
        return (x == other.x && y == other.y && z == other.z);
    }

    // Normalize the vector
    Point3D normalize() const
    {
        float len = sqrt(x * x + y * y + z * z);
        return Point3D(x / len, y / len, z / len);
    }

    // Cross product
    Point3D cross(const Point3D &other) const
    {
        return Point3D(y * other.z - z * other.y, z * other.x - x * other.z, x * other.y - y * other.x);
    }

    static double dot(const Point3D &a, const Point3D &b)
    {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }

    // Function to calculate the angle between two directions (normalized vectors)
    static double angle(const Point3D &direction1, const Point3D &direction2)
    {
        // Calculate the dot product
        double dotProd = Point3D::dot(direction1, direction2);

        // Calculate the angle in radians
        double angleRadians = acos(dotProd);

        // Return the angle
        return angleRadians;
    }
};

class PointPair
{
private:
    Point3D a;
    Point3D b;

public:
    Point3D direction;
    PointPair() : a(), b(), direction() {}
    PointPair(Point3D start, Point3D end) : a(start), b(end), direction((b - a).normalize()) {}
    Point3D start() { return a; }
    Point3D end() { return b; }
};

struct Mesh
{
    std::vector<Point3D> vertices;
    std::vector<int> indices;
};

struct Layer
{
    std::vector<std::string> gcodeLines;
    float zHeight = 0.0f;
};

// Function to parse a single line of GCode and extract the point if extruding.
bool parseGCodeLine(const std::string &line, Point3D &point)
{
    if (line.empty() || line[0] != 'G')
        return false;

    // Basic parsing to find if this is a G1 command
    if (line[1] == '1' || line[1] == '0')
    {
        std::istringstream iss(line);
        char command;
        while (iss >> command)
        {
            switch (command)
            {
            case 'X':
                iss >> point.x;
                break;
            case 'Y':
                iss >> point.y;
                break;
            case 'Z':
                iss >> point.z;
                break;
            case 'E':
                iss >> point.e;
                break;
            default:
                iss.ignore(256, ' ');
                break; // Skip unknown commands/values
            }
        }
        return true;
    }
    return false;
}

bool checkG92(const std::string &line, float &currentExtrusion)
{
    if (line.empty() || line[0] != 'G')
        return false;

    // Looking for G92, which is a reset of the extrusion position
    if (line.find("G92") != std::string::npos && line.find("E") != std::string::npos)
    {
        std::istringstream iss(line);
        char command;
        while (iss >> command)
        {
            switch (command)
            {
            case 'E':
                iss >> currentExtrusion;
                return true;
                break;
            default:
                iss.ignore(256, ' ');
                break; // Skip unknown commands/values
            }
        }
    }
    return false;
}

double calculateMitreNormalizedLength(const PointPair &a, const PointPair &b)
{
    // assume the mitre is in 2d on x/y plane
    double angle = Point3D::angle(a.direction, b.direction);
    double m = 1.0 / cos(angle / 2);
    return m;
}

Mesh createOrthogonalSquares(const std::vector<PointPair> &path, float height, float width)
{
    Mesh layerMesh;
    float halfWidth = width / 2.0f;
    float halfHeight = height / 2.0f;
    const float maxMiterLength = 10.0f;
    if (path.size() == 1)
        throw "size is 1";
    // return layerMesh; // Need at least two points to define a direction

    for (size_t i = 0; i < path.size(); ++i)
    {
        PointPair pair = path[i];

        Point3D startDir = pair.direction;
        Point3D endDir = pair.direction;
        float startLength = 1.0f;
        float endLength = 1.0f;

        // if there is a path before it
        if (i > 0)
        {
            // Check for nan's
            auto newDir = (startDir + path[i - 1].direction).normalize();
            if (newDir == newDir)
                startDir = newDir;
            startLength = calculateMitreNormalizedLength(path[i], path[i - 1]);
            if (startLength != startLength)
            {
                startLength = 1.0f;
            }
            if (startLength > maxMiterLength)
            {
                startLength = maxMiterLength;
            }
        }
        if (i < path.size() - 1)
        {
            // check for nans
            auto newDir = (endDir + path[i + 1].direction).normalize();
            if (newDir == newDir)
                endDir = newDir;
            endLength = calculateMitreNormalizedLength(path[i], path[i + 1]);
            if (endLength != endLength)
                endLength = 1.0f;
            if (endLength > maxMiterLength)
                endLength = maxMiterLength;
        }

        // Create two orthogonal vectors
        Point3D up(0, 0, 1);                                               // Assuming Z-up coordinate system
        Point3D rightStart = startDir.cross(up).normalize() * startLength; // Orthogonal to direction and 'up'
        Point3D rightend = endDir.cross(up).normalize() * endLength;       // Orthogonal to direction and 'up'
        // Generate the four points of the square
        int startIndex = layerMesh.vertices.size();
        layerMesh.vertices.push_back(pair.start() + rightStart * halfWidth + up * halfHeight);
        layerMesh.vertices.push_back(pair.start() - rightStart * halfWidth + up * halfHeight);
        layerMesh.vertices.push_back(pair.start() - rightStart * halfWidth - up * halfHeight);
        layerMesh.vertices.push_back(pair.start() + rightStart * halfWidth - up * halfHeight);
        layerMesh.vertices.push_back(pair.end() + rightend * halfWidth + up * halfHeight);
        layerMesh.vertices.push_back(pair.end() - rightend * halfWidth + up * halfHeight);
        layerMesh.vertices.push_back(pair.end() - rightend * halfWidth - up * halfHeight);
        layerMesh.vertices.push_back(pair.end() + rightend * halfWidth - up * halfHeight);

        layerMesh.indices.push_back(startIndex + 1);
        layerMesh.indices.push_back(startIndex + 0);
        layerMesh.indices.push_back(startIndex + 4);
        layerMesh.indices.push_back(startIndex + 5);

        layerMesh.indices.push_back(startIndex + 2);
        layerMesh.indices.push_back(startIndex + 1);
        layerMesh.indices.push_back(startIndex + 5);
        layerMesh.indices.push_back(startIndex + 6);

        layerMesh.indices.push_back(startIndex + 3);
        layerMesh.indices.push_back(startIndex + 2);
        layerMesh.indices.push_back(startIndex + 6);
        layerMesh.indices.push_back(startIndex + 7);

        layerMesh.indices.push_back(startIndex + 0);
        layerMesh.indices.push_back(startIndex + 3);
        layerMesh.indices.push_back(startIndex + 7);
        layerMesh.indices.push_back(startIndex + 4);
    }

    return layerMesh;
}

float extractNumber(const char letter, const std::string &input)
{
    size_t pos = input.find(letter); // Find the position of letter
    if (pos == std::string::npos)
        throw("No letter found");

    // Extract the substring starting from letter position
    std::string fromLetter = input.substr(pos + 1);

    // Convert the substring to a float
    char *endPtr;
    float number = std::strtof(fromLetter.c_str(), &endPtr);

    return number;
}

bool layerContainsECommand(const Layer &gcodeVector)
{
    std::regex EJumpRegix("^(G0|G1).*E");
    for (const auto &cmd : gcodeVector.gcodeLines)
    {
        if (std::regex_search(cmd, EJumpRegix))
        {
            return true;
        }
    }
    return false;
}

std::vector<Layer> extractStringLayers(const std::string &filePath)
{
    std::vector<Layer> stringLayers;
    std::ifstream file(filePath);

    if (!file.is_open())
    {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return stringLayers;
    }

    size_t lineNum = 0;
    std::string line;

    {
        Layer layer;

        // Look for G1 or G0 calls with a z command
        std::regex ZJumpRegix("^(G0|G1).*Z");
        while (std::getline(file, line))
        {
            lineNum++;

            auto position = line.find("Z");
            if (std::regex_search(line, ZJumpRegix))
            {

                stringLayers.push_back(layer);
                layer = Layer();
                layer.zHeight = extractNumber('Z', line);
            }
            layer.gcodeLines.push_back(line);
        }

        stringLayers.push_back(layer);
    }

    // Now concat layers have no extrusion or are at the same height
    std::vector<Layer> concatLayers;

    auto previousLayer = stringLayers[0];
    for (int i = 1; i < stringLayers.size(); i++)
    {
        auto layer = stringLayers[i];
        if ((layerContainsECommand(layer) == false) || (layer.zHeight == previousLayer.zHeight))
        {
            previousLayer.gcodeLines.insert(previousLayer.gcodeLines.end(), layer.gcodeLines.begin(), layer.gcodeLines.end());
        }
        else
        {
            concatLayers.push_back(previousLayer);
            previousLayer = layer;
        }
    }
    concatLayers.push_back(previousLayer);

    return concatLayers;
}

// Function to read a GCode file and extract 3D points of extrusion paths.
std::vector<Mesh> extract3DPointsFromLayers(const std::vector<Layer> &layers)
{

    std::vector<Mesh> pointLayers;
    Point3D currentPoint{0, 0, 0};
    Point3D previousPoint{0, 0, 0};
    float maxE = 0;

    int lineNum = 0;

    for (int i = 0; i < layers.size(); i++)
    {
        auto layer = layers[i];

        std::vector<PointPair> points;
        bool previousNoExtrude = true;

        for (auto line : layers[i].gcodeLines)
        {

            lineNum++;
            // skip comments
            if (line[0] == ';' || line == "\r")
                continue;
            if (checkG92(line, maxE))
            {
                currentPoint.e = maxE;
            }
            if (parseGCodeLine(line, currentPoint))
            {
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
                if (isExtruding && didMove)
                {
                    points.push_back(PointPair(previousPoint, currentPoint));
                }

                previousPoint = currentPoint;
            }
        }
        if (points.size() > 0)
        {
            float previousZ = i == 0 ? 0 : layers[i - 1].zHeight;
            float currentZ = layers[i].zHeight;
            if (currentZ < previousZ)
            {
                previousZ = 0;
            }
            pointLayers.push_back(createOrthogonalSquares(points, (currentZ - previousZ), 0.4f));
        }
    }

    return pointLayers;
}

void savePointsToPLY(const std::vector<Mesh> &layers, const std::string &filePath)
{
    std::ofstream plyFile(filePath, std::ios::out);

    if (!plyFile.is_open())
    {
        std::cerr << "Failed to create PLY file: " << filePath << std::endl;
        return;
    }

    size_t vertCount = 0;
    size_t faceCount = 0;
    for (auto mesh : layers)
    {
        vertCount += mesh.vertices.size();
        faceCount += mesh.indices.size() / 4;
    }

    // Write the PLY header
    plyFile << "ply\n";
    plyFile << "format ascii 1.0\n";
    plyFile << "element vertex " << vertCount << "\n";
    plyFile << "property float x\n";
    plyFile << "property float y\n";
    plyFile << "property float z\n";

    plyFile << "element face " << faceCount << "\n";
    plyFile << "property list uchar int vertex_index\n";
    plyFile << "end_header\n";

    // Write the points
    for (auto mesh : layers)
    {
        for (const auto &point : mesh.vertices)
        {
            plyFile << point.x << " " << point.y << " " << point.z << "\n";
        }
    }
    // Write the faces
    int indiceIndex = 0;
    for (auto mesh : layers)
    {
        for (size_t i = 0; i < mesh.indices.size(); i += 4)
        {
            {
                plyFile << "4";
                for (int x = 0; x < 4; x++)
                {
                    plyFile << " " << mesh.indices[i + x] + indiceIndex;
                }
                plyFile << "\n";
            }
        }
        indiceIndex += mesh.vertices.size();
    }
    plyFile.close();
}

int main(int argc, char *argv[])
{
    if (argc != 3)
    {
        std::cerr << "Usage: " << argv[0] << " <GCodeFilePath> <PLYFilePath>" << std::endl;
        return 1;
    }

    std::string filePath = argv[1];
    std::string plyFilePath = argv[2];
    auto stringLayers = extractStringLayers(filePath);
    std::cout << stringLayers.size() << std::endl;

    auto layers = extract3DPointsFromLayers(stringLayers);

    savePointsToPLY(layers, plyFilePath);
    std::cout << "Saved " << layers.size() << " layers to PLY file: " << plyFilePath << std::endl;

    return 0;
}
