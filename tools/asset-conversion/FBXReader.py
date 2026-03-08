# SPDX-License-Identifier: GPL-3.0-or-later
from enum import Enum 

class Axis(Enum):
    X = 0
    Y = 1
    Z = 2

class AxisSign(Enum):
    Negative = 0
    Positive = 1

class AxisSpecification:
    UpAxis = 0
    UpAxisSign = 0
    FrontAxis = 0
    FrontAxisSign = 0

class Vector3D:
    X = 0.0
    Y = 0.0
    Z = 0.0

class Triangle:
    A = 0
    B = 1
    C = 2

class ObjectParameters:
    Position = Vector3D()
    Rotation = Vector3D()
    Scale    = Vector3D()

class Object3D:
    ObjectParams = ObjectParameters()

    VertexCount = 0
    TriangleCount = 0
    Vertices = []
    Triangles = []

class ShapeKey:
    Name = ""
    IndexCount = 0
    VertexCount = 0
    Indexes = []
    Vertices = []

class MorphObject:
    name = ""
    baseMesh = Object3D()
    shapeKeys = []

#GlobalSettings:  {
#	Version: 1000
#	Properties70:  {
#		P: "UpAxis", "int", "Integer", "",1
#		P: "UpAxisSign", "int", "Integer", "",1
#		P: "FrontAxis", "int", "Integer", "",2
#		P: "FrontAxisSign", "int", "Integer", "",1
def GetGlobalSettings(dataString):
    lineNumber = 0
    lines = dataString.splitlines()
    axisSpecs = AxisSpecification()

    for line in lines:
        if line.find("GlobalSettings:  {") >= 0:
            axisSpecs.UpAxis        = int(lines[lineNumber + 3].split(",")[4])
            axisSpecs.UpAxisSign    = int(lines[lineNumber + 4].split(",")[4])
            axisSpecs.FrontAxis     = int(lines[lineNumber + 5].split(",")[4])
            axisSpecs.FrontAxisSign = int(lines[lineNumber + 6].split(",")[4])
            break
        
        lineNumber += 1

    print("Up Axis: ", axisSpecs.UpAxis, " Sign: ", axisSpecs.UpAxisSign)
    print("Front Axis: ", axisSpecs.FrontAxis, " Sign: ", axisSpecs.FrontAxisSign)

    return axisSpecs

# ObjectType: "Geometry" {
# Count: 9
# PropertyTemplate: "FbxMesh" {
#	Properties70:  {
def GetMeshShapeCount(dataString):
    lineNumber = 0
    lines = dataString.splitlines()
    meshShapes = 0

    for line in lines:
        if line.find("ObjectType: \"Geometry\" {") >= 0:
            meshShapes = int(lines[lineNumber + 1].split(": ")[1])
            break
        
        lineNumber += 1
    
    print("Contains ", meshShapes, " geometries/shape keys.")

    return meshShapes

#Objects:  {
#	Geometry: 3038010852992, "Geometry::Mesh", "Mesh" {
#		Vertices: *114 {
#			a: 10.7975749969482,13.6265239715576,0.0234996974468231,10.1619930267334,14.4739675521851,0.0234996974468231,7.51373052597046,12.7578935623169,0.0234996974468231,13.3844385147095,14.0652685165405,0.0234996974468231,11.4705438613892,12.6948261260986,0.0234996974468231,14.2046508789063,8.18757438659668,0.0234996974468231,10.1157102584839,9.61395645141602,0.0203892663121223,12.3156242370605,8.29102516174316,0.0234996974468231,13.3564224243164,10.2925596237183,0.0234996974468231,8.72986507415771,7.07394504547119,0.0234996974468231,9.285813331604,8.54938697814941,0.0234996974468231,11.7018203735352,10.3192472457886,0.0234996974468231,10.9674043655396,9.01348304748535,0.0234996974468231,12.9356555938721,9.28118133544922,0.0234996974468231,14.6750869750977,9.52291011810303,0.0234996974468231,10.4341812133789,7.98412322998047,0.0234996974468231,16.0408496856689,7.56885528564453,0.0234996974468231,3.10627031326294,1.94792711734772,0.0234996974468231,5.58724498748779,4.02721977233887,0.0234996974468231,5.96529817581177,3.34199833869934,0.0234996974468231,9.42035675048828,1.49372184276581,0.0234996974468231,9.35679817199707,2.29146933555603,0.0234996974468231,11.3582448959351,2.49418663978577,0.0234996974468231,12.8883981704712,2.98757338523865,0.0104711800813675,14.6132659912109,3.34199833869934,0.0234996974468231,14.0934429168701,3.79093670845032,0.0170471575111151,1.87759721279144,2.20783877372742,0.0234996974468231,15.2512311935425,4.64155626296997,0.0234996974468231,2.63234901428223,2.85647964477539,0.0234996974468231,1.25599443912506,3.4539577960968,0.0234996974468231,10.903618812561,3.24748516082764,0.0163857694715261,13.0774250030518,2.30235195159912,0.0234996974468231,1.26371049880981,5.2012882232666,0.0234996974468231,1.89929342269897,6.6843147277832,0.0234996974468231,2.45013189315796,6.17584848403931,0.0234996974468231,3.76366972923279,7.40464210510254,0.0234996974468231,4.20857763290405,6.59957075119019,0.0234996974468231,5.89985752105713,6.52065992355347,0.0234996974468231
#		}
#		PolygonVertexIndex: *99 {
#			a: 1,2,-1,0,3,-2,4,0,-3,4,3,-1,12,13,-12,13,14,-9,7,13,-13,6,12,-12,11,13,-9,6,10,-13,10,15,-13,10,9,-16,7,5,-14,15,7,-13,13,5,-15,5,16,-15,19,18,-18,19,20,-19,20,21,-19,20,22,-22,21,22,-31,25,23,-25,22,23,-31,18,28,-18,24,27,-26,28,26,-18,28,29,-27,22,31,-24,23,31,-25,34,33,-33,34,35,-34,36,35,-35,36,37,-36
#		}
def GetBaseMesh(dataString, scale):
    lineNumber = 0
    lines = dataString.splitlines()
    
    baseMesh = Object3D()

    for i, line in enumerate(lines):
        if line.find("\"Geometry::") >= 0 and line.find("\"Mesh\" {") >= 0 :
            vertexString = ""

            for l in range(2, 20):
                if lines[i + l].find("}") >= 0:
                    break
                else:
                    vertexString += lines[i + l]
                    print(lines[i + l])

            vertexString.replace("\n", "")
            
            vertexCount = int(lines[lineNumber + 1].split(": *")[1].split(" ")[0])
            verticesXYZ = [float(i) for i in filter(None, vertexString.split(": ")[1].split(","))]

            polygonVertexIndex = 0
            triangleVertexIndex = []
            offset = 4

            #variable amount of spacing based on the amount of vertices given
            for l in range(4, 15):
                if lines[i + l].find("PolygonVertexIndex:") >= 0:
                    offset = l
                    break
            
            triangleVertexString = ""
                
            for l in range(offset + 1, offset + 10):
                if lines[i + l].find("}") >= 0:
                    break
                else:
                    triangleVertexString += lines[i + l]

            triangleVertexString.replace("\n", "")

            polygonVertexIndex = int(lines[lineNumber + offset].split(": *")[1].split(" ")[0])
            triangleVertexIndex = [int(i) for i in filter(None, triangleVertexString.split(": ")[1].split(","))]
            
            vertices = []
            triangles = []
            triangleCount = 0

            for i in range(0, int(vertexCount), 3):
                vertex = Vector3D()

                vertex.X = verticesXYZ[i] * scale
                vertex.Y = verticesXYZ[i + 1] * scale
                vertex.Z = verticesXYZ[i + 2] * scale

                print("Vertex: [", vertex.X, ",", vertex.Y, ",", vertex.Z, "]")

                vertices.append(vertex)

            
            for i in range(0, polygonVertexIndex, 3):
                triangle = Triangle()

                triangle.A = triangleVertexIndex[i]
                triangle.B = triangleVertexIndex[i + 1]
                triangle.C = (triangleVertexIndex[i + 2] * -1) - 1 #negate value and subtract one

                print("Triangle Vertex Indexes: [", triangle.A, ",", triangle.B, ",", triangle.C, "]")
                
                triangleCount += 1

                triangles.append(triangle)

            baseMesh.ObjectParams = GetMeshParameters(dataString, scale)
            baseMesh.TriangleCount = triangleCount
            baseMesh.Triangles = triangles
            baseMesh.VertexCount = vertexCount
            baseMesh.Vertices = vertices

            print("Primary mesh contains ", vertexCount, " vertices and ", triangleCount, "triangles.")
            break
        
        lineNumber += 1
    
    print("Created primary mesh.")

    return baseMesh

#Geometry: 3038010853408, "Geometry::AddEyebrow", "Shape" {
#   Version: 100
#   Indexes: *17 {
#       a: 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16
#   } 
#   Vertices: *51 {
#       a: 2.37597942352295,-2.99088287353516,0,2.52437686920166,-3.26673984527588,0,3.14269971847534,-2.70812797546387,0,1.77199268341064,-3.13370132446289,0,2.21885299682617,-2.68759822845459,0,-0.354424476623535,-1.27592945098877,0,-0.354424476623535,-1.27592945098877,0,-0.354424476623535,-1.27592945098877,0,-0.354424476623535,-1.27592945098877,0,-0.354424476623535,-1.27592945098877,0,-0.354424476623535,-1.27592945098877,0,-0.354424476623535,-1.27592945098877,0,-0.354424476623535,-1.27592945098877,0,-0.354424476623535,-1.27592945098877,0,-0.354424476623535,-1.27592945098877,0,-0.354424476623535,-1.27592945098877,0,-0.354424476623535,-1.27592945098877,0
#   }
def GetMeshShapeKeys(dataString, scale):
    lines = dataString.splitlines()
    
    shapeKeys = []

    for i, line in enumerate(lines):
        if line.find("\"Shape\" {") >= 0:
            shapeKey = ShapeKey()
            
            name = lines[i].split("\"Geometry::")[1].split("\"")[0]
            indexCount = int(lines[i + 2].split(": *")[1].split(" ")[0])
            indexes = [int(i) for i in filter(None, lines[i + 3].split(": ")[1].split(","))]
            
            vertexCount = int(lines[i + 5].split(": *")[1].split(" ")[0])

            offset = 6
            vertexString = ""
                
            for l in range(offset, offset + 10):
                if lines[i + l].find("}") >= 0:
                    break
                else:
                    vertexString += lines[i + l]


            verticesXYZ = [float(i) for i in filter(None, vertexString.split(": ")[1].split(","))]
            
            vertices = []

            for i in range(0, vertexCount, 3):
                vertex = Vector3D()

                vertex.X = verticesXYZ[i] * scale
                vertex.Y = verticesXYZ[i + 1] * scale
                vertex.Z = verticesXYZ[i + 2] * scale

                print("Vertex: [", vertex.X, ",", vertex.Y, ",", vertex.Z, "]")

                vertices.append(vertex)

            shapeKey.Name = name
            shapeKey.IndexCount = indexCount
            shapeKey.Indexes = indexes
            shapeKey.VertexCount = vertexCount
            shapeKey.Vertices = vertices

            shapeKeys.append(shapeKey)

            print("Created shape key ", name, " with ", indexCount, " modified indices and ", vertexCount, "modified vertex values.")
    
    print("Created ", len(shapeKeys), " shape keys.")

    return shapeKeys

#Model: 3038010866496, "Model::Face", "Mesh" {
#    Version: 232
#    Properties70:  {
#        P: "InheritType", "enum", "", "",1
#        P: "DefaultAttributeIndex", "int", "Integer", "",0
#        P: "Lcl Translation", "Lcl Translation", "", "A",-0.349570542573929,-0.286012262105942,0
#        P: "Lcl Rotation", "Lcl Rotation", "", "A",9.33466682838942e-06,-0,0
#    }
def GetMeshParameters(dataString, scale):
    lines = dataString.splitlines()

    objParam = ObjectParameters()

    for line in lines:
        if line.find("Lcl Translation") >= 0:
            splitTranslation = line.split(",")
            objParam.Position.X = float(splitTranslation[4]) * scale
            objParam.Position.Y = float(splitTranslation[5]) * scale
            objParam.Position.Z = float(splitTranslation[6]) * scale
            
        if line.find("Lcl Rotation") >= 0:
            splitRotation = line.split(",")
            objParam.Rotation.X = float(splitRotation[4])
            objParam.Rotation.Y = float(splitRotation[5])
            objParam.Rotation.Z = float(splitRotation[6])

    print("Object Position: [", objParam.Position.X, ", ", objParam.Position.Y, ", ", objParam.Position.Z, "]")
    print("Object Rotation: [", objParam.Rotation.X, ", ", objParam.Rotation.Y, ", ", objParam.Rotation.Z, "]")

    return objParam

def GetMorphObject(dataString, name, scale):
    morph = MorphObject()

    morph.name = name
    morph.baseMesh = GetBaseMesh(dataString, scale)
    morph.shapeKeys = GetMeshShapeKeys(dataString, scale)

    return morph